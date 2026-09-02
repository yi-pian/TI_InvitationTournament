# Slew Rate：压摆率测量

## 输入与输出

- 输入：被测输出经已知调理比例送入的 `voltage_v[N]`、`Fs`、阈值比例（常用 10/90 或 20/80）、ADC 到 DUT 输出的幅值反算参数。
- 输出：`rise_slew_rate_v_per_us`、`fall_slew_rate_v_per_us`、对应 rise/fall time、实际 DUT 高低平台与有效边沿数。

## 逻辑链

```text
足够快且足够大的输入阶跃 → DUT输出 → 已校准模拟调理 → ADC帧
→ 削顶/平台存在检查
→ 稳健上下平台 L/H（不能用单个max/min）
→ 反算 DUT 实际平台与 ΔVdut
→ 20%/80%（或10%/90%）阈值
→ 上升/下降crossing线性插值
→ 分方向计算每条边沿时间
→ Median/MAD剔除坏边沿 → 多边沿平均
→ SRrise=(βH-βL)ΔVdut/trise
   SRfall=(βH-βL)ΔVdut/tfall
```

### 每一步为什么存在

- 输入阶跃必须使 DUT 进入压摆率限制；小信号带宽限制得到的斜率不是大信号 SR。
- 用 ADC 实测并校准后的输出跨度，不把 DDS/发生器设定幅度直接代入 DUT 输出公式。
- 稳健平台让阈值不被少量过冲/错误码拉偏，但过冲仍应作为独立质量指标保留。
- 上升和下降必须分开，因为正负输出级能力可能不同。
- 多边沿平均降低 ADC 量化和触发相位造成的随机误差。

## 推荐与备选

| 方法 | 使用条件 | 优点 | 不适用 |
|---|---|---|---|
| ADC 20/80 crossing + interpolation | ADC 带宽/Fs 足够，边沿区有多个点 | 与波形同帧，可同时显示平台和过冲 | 区间只有约 1 点、前端已限速 |
| 局部线性回归斜率 | slew 区近似直线且有 5 点以上 | 利用更多点，抗单点噪声 | 弯曲边沿、自动选择区间不可靠 |
| 两比较器阈值 + Timer Capture | ADC 时间分辨率不足 | 时间分辨率高、RAM 小 | 比较器阈值/传播延迟未校准 |

## 采样率、周期与测试频率

- 理想上 20%～80% 区间应有至少 5 个 ADC 点；最低 2 个点只能做脆弱插值。
- 方波半周期必须大于最慢预期边沿时间并留出稳定平台。高测试频率会让输出尚未到平台就换向，从而平台和 SR 都失真。
- 至少采 3 条完整上升和 3 条完整下降边沿，建议 5～20 条；帧首尾残缺边沿丢弃。
- 采样前端带宽应明显高于由预期 rise time 推出的带宽；否则测到的是测量链的 SR。

## 抗噪声、插值与校准

- crossing 插值可提高亚采样点精度，但不能替代足够 Fs；阈值附近有振铃或量化台阶时要返回低质量状态。
- 对 per-edge 结果使用 Median/MAD，不对原始边沿做 Hampel/Median。
- 校准 ADC gain/offset、调理电路增益和频率响应；上升/下降路径若不对称要分别校准。
- 用示波器以相同 20/80 定义交叉验证，不能拿 10/90 时间直接比较。

## MCU / RAM 与现有 Primitive

- O(N+E)，主要 RAM 是帧、边沿事件和少量 per-edge 结果。
- 复用 Robust VPP、MAD，以及 ZeroCross/Interpolation 的基础能力。
- `SR=(βH-βL)ΔV/t` 是 Recipe-local 标量公式，不建单独模块。
- 完整多阈值边沿提取依赖拟议的 P0 `edge_timing` Primitive。

## 失效条件

ADC 或调理削顶、没有高低平台、DUT 未进入 slew limit、输入阶跃自身太慢、阈值区不足两点、强振铃多次跨阈值、自动量程在采样中改变增益，均应拒绝结果。Recipe 状态：`DRAFT`。

## 20 项执行契约

| # | 字段 | 本 Recipe 的约束 |
|---|---|---|
| 1 | 用途 | DUT 正/负压摆率和相应 rise/fall time |
| 2 | 输入 | 校准 `voltage_v[N]`、`Fs`、阈值比例、DUT 幅值反算参数 |
| 3 | 输出 | `SRrise/SRfall`、rise/fall time、平台、有效边沿数 |
| 4 | 完整逻辑链 | 见“逻辑链”，必须确认 DUT 真正进入大信号 slew limit |
| 5 | 步骤原因 | 见“每一步为什么存在” |
| 6 | 默认算法 | 20/80 crossing + 相邻点插值 + 分方向多边沿平均 |
| 7 | 可选增强 | 局部线性回归、双比较器 Timer Capture |
| 8 | 适用条件 | 输入阶跃足够快/大，ADC/前端比 DUT 更快，存在稳定平台 |
| 9 | 不适用条件 | 小信号带宽限制、削顶、无平台、强振铃多 crossing |
| 10 | 采样率 | 20%～80% 区间建议至少 5 点；ADC/前端带宽高于被测边沿 |
| 11 | 点数/周期数 | 至少 3 上升+3 下降，建议各 5～20 条有效边沿 |
| 12 | 抗噪 | 对 per-edge 时间/斜率做 Median/MAD；不平滑真实边沿 |
| 13 | 精度增强 | 插值/局部回归、同定义示波器交叉验证、幅值/通道校准 |
| 14 | 计算量 | O(N+E) |
| 15 | RAM | 帧 + O(E) 事件/结果；无 FFT workspace |
| 16 | Primitive | RobustPeakToPeak、MAD、ZeroCross/Interpolation；完整 `edge_timing` 缺失 |
| 17 | 仓库路径 | `05_precision/{robust_peak_to_peak,zero_cross_interpolation}`、`04_dsp/mad` |
| 18 | 伪代码 | `validate step -> robust L/H -> thresholds -> interpolate crossings -> reject edges -> ΔV/Δt` |
| 19 | MCU 调用 | 当前保持应用 Recipe；下方公式片段不是新 API |
| 20 | 失败排查 | 查输入阶跃、平台、测量链带宽、点数、阈值定义、方向、量程切换 |

```c
/* APPLICATION RECIPE：t20/t80 已由真实 crossing API/局部逻辑得到。 */
float rise_time_s = (t80_rise_samples - t20_rise_samples) / sample_rate_hz;
float sr_rise_v_per_s = (0.80f - 0.20f) * dut_delta_v / rise_time_s;
```
