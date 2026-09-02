# Frequency Response：幅频/相频响应

## 输入与输出

- 输入：频点表 `f[k]`，每个频点稳定的输入/输出双通道波形或幅相测量结果，settling 条件和校准表。
- 输出：每点 `frequency_hz`、`gain`、`gain_db`、`phase_deg`、质量指标；可选保存原始输入/输出幅值。

## 完整逻辑链

```text
选择频点（常用对数间隔）
→ DDS/DAC设置频率与幅值
→ 等待源、DUT、VGA/量程和滤波器建立
→ 同步采输入与输出
→ 两路校准/削顶检查
→ 已知频率LockIn或SineFit3测 Ain/Aout/phase
→ Gain Recipe：Aout/Ain、20log10
→ thru/fixture频响校准：dB相减、phase相减
→ 重复/质量门限 → 保存点
→ 下一频点
→ 扫描结束后再做phase unwrap、截止点/峰值/带宽后处理
```

### 每一步为什么存在

- 同时测输入和输出能抵消 DDS 随频率幅值变化；只相信设定幅值会把信号源频响误认为 DUT。
- 改频后必须等待；前一点的滤波器/AGC状态会污染当前点。
- 已知频率测量优先 LockIn/SineFit3，不必每点做全 FFT；FFT 适合同时检查谐波/杂散。
- fixture/thru 校准把电缆、调理、ADC 和通道频响从 DUT 响应中剥离。
- 曲线后处理与单点采集分离，便于重测坏点和保持 Application 状态机简单。

## 适用、备选和失效

| 方法 | 使用条件 | 备选 | 失效条件 |
|---|---|---|---|
| 双通道 LockIn | 激励频率已知且参考相干 | SineFit3 | 源频率漂移/参考不同步 |
| 双通道 SineFit3 | 频率已知但记录不严格相干 | FFT | 多音/强失真、频率错误 |
| FFT transfer | 还需谐波或一次多音激励 | 多正弦拟合（当前无正式模块） | 频点互相泄漏、动态范围不足 |

## 采样、频点与记录

- 每点建议 5～20 周期；低频观测时间会变长，高频由模拟带宽和 Fs 决定。
- 对数扫频适合跨多个 decade；在截止点、峰值和相位快速变化处自适应加密。
- 每个频点至少测 2～3 帧以判断稳定性；自动量程/VGA 改变后额外丢弃建立帧。
- FFT 测到谐波时保证最高关注频率低于 Nyquist 并有抗混叠余量。

## 抗噪声、校准与精度

- 弱输出使用同步检测并延长积分；输入/输出量程可不同，但各自校准。
- 多帧 gain/phase 先按质量门限筛选，phase 做圆周平均。
- fixture 校准用复数关系 `H_dut=H_meas/H_thru`；实现上 `gain_db` 相减、phase unwrap 后相减。
- 校准表用按频率的 LUT + interpolation，表外禁止静默外推。

## MCU / RAM 与现有 Primitive

- 每点 LockIn/SineFit3 为 O(N)、O(1)；只保存结果时总 RAM O(K)，可边测边 UART 输出降为 O(1)。
- FFT 每点 O(N log N)，资源见 DSP 预算。
- 复用 LockIn、SineFit3、Gain/Phase Recipe、ADC Calibration、Channel Delay Calibration；Application 参考 `08_applications/sweep_analyzer` 和 `sweep_measurement`。

## 缺口

曲线点的统一质量结构、phase unwrap、复数 thru correction、按频率 LUT 插值尚无正式 Primitive。它们跨频响、VGA、传感器补偿重复，是 P1 候选。Recipe 状态：`DRAFT`。

## 20 项执行契约

| # | 字段 | 本 Recipe 的约束 |
|---|---|---|
| 1 | 用途 | DUT 幅频/相频响应和后续带宽/峰值分析 |
| 2 | 输入 | 频点表、同步 Vin/Vout、settling、校准/fixture 表 |
| 3 | 输出 | 每点 frequency/gain/gain dB/phase/quality |
| 4 | 完整逻辑链 | 见“完整逻辑链”，DDS 设置和 ADC 采集属于 Application |
| 5 | 步骤原因 | 见“每一步为什么存在” |
| 6 | 默认算法 | 对数扫频 + 同步双通道 LockIn/SineFit3 + thru 补偿 |
| 7 | 可选增强 | FFT transfer、自适应加密、曲线流式输出 |
| 8 | 适用条件 | 激励已知、DUT 每点达到稳态、输入输出可同步测量 |
| 9 | 不适用条件 | AGC/量程仍变化、源幅值只凭设定、未等待建立、fixture 未定义 |
| 10 | 采样率 | 每点覆盖目标/谐波频带并留抗混叠余量 |
| 11 | 点数/周期数 | 每点 5～20 周期、至少 2～3 帧；变化快处加密频点 |
| 12 | 抗噪 | 弱输出 LockIn、帧质量门、gain Median/MAD、phase 圆周平均 |
| 13 | 精度增强 | 同时测 Vin/Vout、thru 校准、通道 delay、频率表内插值 |
| 14 | 计算量 | 每点 LockIn/SineFit O(N)，FFT O(N log N) |
| 15 | RAM | 处理 O(1) 或 FFT 工作区；曲线 O(K)，可 UART 流式降 RAM |
| 16 | Primitive | LockIn、SineFit3、Gain/Phase、ADC/ChannelDelay Calibration |
| 17 | 仓库路径 | `05_precision/{lock_in,sine_fit_3param,channel_delay_calibration}`；Application 参考 `MSPM0_Signal_Contest/08_applications/sweep_analyzer` |
| 18 | 伪代码 | `set f -> settle -> acquire A/B -> calibrate -> measure complex ratio -> quality -> store -> next` |
| 19 | MCU 调用 | 下方只展示真实算法 API；DDS/ADC API 必须从目标模块 README/.h 读取 |
| 20 | 失败排查 | 查 DDS 实际输出、settling、同步、削顶、f0、输入下限、通道/fixture 校准 |

```c
SignalLockIn_Process(vin_v, N, &lock_cfg, &in_result);
SignalLockIn_Process(vout_v, N, &lock_cfg, &out_result);
/* 两次均 OK 后，应用层计算幅值比和 B-A 相位。 */
```
