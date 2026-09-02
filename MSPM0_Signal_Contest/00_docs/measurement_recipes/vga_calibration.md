# VGA / PGA Calibration：可变增益校准

## 输入与输出

- 输入：控制 code/电压、已校准的输入/输出幅值、测试频率、量程与温度。
- 输出：`control → gain_db` 正向表、`target_gain_db → control` 反向表/插值、偏置与频响残差。

## 逻辑链

```text
固定已知单音输入 → 选择频率与安全幅度
→ 遍历VGA控制点
→ 每点等待建立 → 同步测Ain/Aout → Gain Recipe
→ 检查削顶/噪声底/单调性 → 保存 control,gain_db
→ 独立控制点验证
→ Application请求目标增益：查找包围区间 → 线性插值control
→ 设置后重新测量并闭环微调（需要时）
```

若 VGA 增益随频率变化，应建立二维关系 `gain_db(control, frequency)`，或用“控制表 + 每档频响补偿表”，不能只在 1 kHz 标定后跨 MHz 使用。

## 为什么需要正向表和反向表

正向 `control→gain_db` 用来验证单调性、残差和频率依赖；运行时 AGC 需要的是 `target_gain_db→control` 反查。先保留同一组实测点再做区间反插值，可避免为正反方向维护两份彼此不一致的标定数据。

## 推荐、备选与失效

| 特性 | 默认 | 备选 | 失效条件 |
|---|---|---|---|
| 单调近线性 dB/V | 稀疏 LUT+插值 | affine 模型 | 表内残差超指标 |
| 非线性但单调 | 密一些的分段 LUT | 低阶拟合 | 平台/拐点导致反解不唯一 |
| 有迟滞 | 升/降方向分别标定 | 闭环 AGC | 用一张表描述双值关系 |
| 频率相关 | 多频表/频响补偿 | 关键频段分档 | 只校准单频 |

## 采样、抗噪声与精度

- 每个控制点至少 2～3 帧；输入幅值必须高于噪声且输出不削顶。
- 优先 LockIn/SineFit3 测目标单音幅值，输入输出同方法。
- 切换控制后等待建立；控制 DAC 自身先校准。
- 温度/供电变化明显时把条件写入表头，必要时按温度建立多张表并插值。

## MCU / RAM 与 Primitive

- 标定过程主要 O(K*N)；比赛运行时 LUT 反查 O(log K)，RAM O(K) 或 O(KF)。
- 复用 Gain、DAC Calibration、ADC Calibration、LockIn/SineFit3、Clipping Detect。
- 通用 1D LUT 插值应由同一 `calibration_lut_1d` Primitive 服务 ADC/DAC/VGA/传感器；二维频率表先留 Recipe/离线工具。

Recipe 状态：`DRAFT`。

## 20 项执行契约

| # | 字段 | 本 Recipe 的约束 |
|---|---|---|
| 1 | 用途 | VGA/PGA control↔gain dB 正向/反向标定 |
| 2 | 输入 | control、校准 Ain/Aout、频率、量程/温度 |
| 3 | 输出 | control→gain dB 表、反查控制、残差/频率范围 |
| 4 | 完整逻辑链 | 逐控制点 settle/双通道测量/Gain/单调检查/独立验证 |
| 5 | 步骤原因 | 实测表替代理想 dB/V 假设；反解前必须保证单调 |
| 6 | 默认算法 | 单调 1D LUT + 线性插值 |
| 7 | 可选增强 | affine、密表、分频表/二维表、闭环微调 |
| 8 | 适用条件 | control→gain 单调且条件稳定 |
| 9 | 不适用条件 | 平台/迟滞导致多值、单频表跨宽频无补偿 |
| 10 | 采样率 | 每校准频率按 Gain Recipe 设置，覆盖目标频带 |
| 11 | 点数/周期数 | 每 control 2～3 帧、每帧 5～20 周期；留独立控制点 |
| 12 | 抗噪 | LockIn/SineFit3、残差门、多帧 Median/MAD |
| 13 | 精度增强 | DAC/ADC 先校准、升降分表、频率/温度多表 |
| 14 | 计算量 | 标定 O(KN)，运行查询 O(log K) |
| 15 | RAM | 1D O(K)，多频 O(KF) |
| 16 | Primitive | Gain、LockIn/SineFit3、ADC/DAC Calibration、Clipping |
| 17 | 仓库路径 | 本 Recipe、`gain.md`、`05_precision/{lock_in,sine_fit_3param}` |
| 18 | 伪代码 | `for control: set/settle/measure gain/check -> table -> inverse interpolate -> remeasure` |
| 19 | MCU 调用 | 硬件 set API 必须从 exact VGA/PGA driver `.h` 获取；本 Recipe 不命名它 |
| 20 | 失败排查 | 查控制方向、单调/迟滞、settling、Ain/Aout 削顶/噪声、频率和温度 |

```c
/* APPLICATION PSEUDOCODE：先由真实 driver 设置 control，再调用真实测量 API。 */
SignalLockIn_Process(vin_v, N, &cfg, &in_result);
SignalLockIn_Process(vout_v, N, &cfg, &out_result);
gain_db = 20.0f * log10f(out_result.amplitude_peak_v / in_result.amplitude_peak_v);
```
