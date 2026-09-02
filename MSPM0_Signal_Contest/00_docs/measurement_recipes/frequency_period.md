# Frequency / Period：频率与周期测量

## 输入与输出

- 输入：单通道 `voltage_v[N]` 与 `Fs`，或比较器/Timer Capture 边沿时间戳。
- 输出：`frequency_hz`、`period_s`、参与平均的周期数，以及可选的稳定性/相关系数。

## 默认决策链

```text
先判断波形和硬件条件
├─ 已有干净数字边沿 → Timer Capture时间戳 → 相邻同向边沿差 → 多周期平均
├─ 高SNR正弦 → ADC转电压 → 去DC/中心阈值 → 滞回过零
│              → crossing线性插值 → 同向边沿配对 → 多周期平均
├─ 未知频率/有谐波或中等噪声 → 去DC → Hann → FFT → Magnitude
│              → 排除DC后找峰 → 峰值插值 → f=(fractional_bin)Fs/N
└─ 严重失真但周期重复 → 去DC → 限定lag自相关 → 周期峰 → 可选峰顶插值
```

### 每一步为什么存在

1. **去 DC 或估计中心阈值**：固定用 0 V 阈值会让带偏置波形漏检；但测 DC 本身时不能先去 DC。
2. **滞回过零**：避免阈值附近噪声产生多次假边沿。
3. **线性插值**：使用跨阈值两点计算小数样本位置，通常比整数索引高一档；只在两点之间局部近似直线时成立。
4. **只配同方向边沿**：上升和下降交替相差半周期，混用会把频率算成两倍。
5. **跨多个周期求首末差**：随机边沿误差只主要落在首末两端，不会逐周期累加。
6. **FFT 窗和峰值插值**：非相干记录先抑制泄漏，再从整数 bin 细化频率；插值不能分辨本来未分开的两条谱线。
7. **限定自相关 lag**：排除 lag=0 宽峰和不可能的周期，降低 O(NL) 计算量。

## 推荐、备选与选择条件

| 方法 | 推荐使用条件 | 优点 | 失效条件 |
|---|---|---|---|
| Timer Capture | 方波/比较器边沿干净，频率在 Timer 量程内 | CPU/RAM 最低，时间分辨率由 Timer 决定 | 阈值噪声、边沿抖动、计数器溢出未处理 |
| ZeroCross + interpolation | 单音、SNR 高、每周期点数足够 | O(N)，结果直观 | 多音、强谐波、噪声假过零、每周期点数太少 |
| Hann FFT + peak interpolation | 未知频率、中等噪声、还需频谱 | 对单次假边沿不敏感 | N 非 2 次幂、记录太短、近邻强音、严重 off-bin 偏差 |
| Autocorrelation | 非正弦但重复、基波在频谱中不突出 | 不依赖固定阈值 | lag 范围过大很慢；次谐波/倍周期峰歧义 |
| SineFit4 | 已有可靠粗频率且仅单音 | 同时估计频率/幅值/相位/DC | 搜索跨多个局部极小、强噪声/多音；当前仅窄范围 PC 验证 |

## 采样与记录要求

- 过零法：理论最低每周期 2 点不代表可测；比赛建议目标频率上限仍有约 20 点/周期，快速但干净信号可在板上验证后降低。至少 3 个同向 crossing 才有 2 个周期，建议 5～20 个周期。
- FFT 法：`N` 为 2 次幂；观测时间 `Tobs=N/Fs`，整数 bin 分辨率 `Δf=Fs/N`。建议至少覆盖 3～10 个低频周期；需要测 H5 时还必须满足 `5*f0 < Fs/2` 并留抗混叠余量。
- 自相关：`min_lag≈Fs/fmax`、`max_lag≈Fs/fmin`，记录至少覆盖 3 个最长候选周期。
- Timer Capture：观测多个周期；低频要处理计数器溢出，高频要确认输入同步器和比较器传播延迟。

## 抗噪声与精度增强

- 过零阈值用稳健上下电平中点，滞回宽度可由 MAD 噪声估计设置；不要先 Hampel 真实方波边沿。
- 对每周期结果先用 Median/MAD 删除少量明显错误周期，再对保留周期求平均；频率快速变化时改用短窗。
- FFT 优先 Hann；相干采样时 Rectangular 才能发挥窄主瓣。幅值需 coherent gain，频率只需一致的 magnitude 标度。
- Fs 的系统误差会按比例进入频率；高精度必须校准 Timer/采样时钟，而不是只增加 N。

## MCU / RAM

- 过零：O(N)+O(E)，事件约十余字节/个、位置 4E 字节。
- FFT：O(N log N)，典型 `complex[N]=8N` 字节、`magnitude[N/2+1]≈2N` 字节，另有时域数组。
- 自相关：O(NL)，输出 `4(L+1)` 字节；先缩小 lag 范围。
- Timer Capture：O(E)，几乎无帧 RAM。

## 复用的现有 Primitive

- `SignalZeroCross_Process`
- `SignalZeroCrossInterpolation_Process`
- `recipe_multi_cycle_average`（直接 Recipe；旧 `SignalMultiCycleAverage_Process` 仅在 legacy compatibility 区维护）
- `SignalWindow_Apply`、`SignalFFT_ForwardReal`、`SignalFFTMagnitude_Process`
- `recipe_peak_detect`、`SignalFFTParabolicInterpolation_Process` 或 `SignalLogParabolicInterpolation_Process`
- `SignalAutocorrelation_Process`、`SignalAutocorrelation_FindPeriod`
- 可选 `SignalMAD_Process`、`SignalSineFit4Param_Process`

周期最终由 `period_s = 1.0f / frequency_hz` 或 `average_period_samples/Fs` 得到，这个标量公式留在 Recipe，不新建模块。

## 已知缺口

自相关峰目前只输出整数 lag；“相关峰三点抛物线亚样本插值”会在时间延迟与自相关周期中重复使用，是正式 Primitive 的候选。Recipe 状态：`DRAFT`。

## 20 项执行契约

| # | 字段 | 本 Recipe 的约束 |
|---|---|---|
| 1 | 用途 | 频率、周期、高精度单音测频、重复波形周期 |
| 2 | 输入 | `voltage_v[N]`+真实 `Fs`，或同向 Timer Capture 时间戳 |
| 3 | 输出 | `frequency_hz`、`period_s`、周期数、质量指标 |
| 4 | 完整逻辑链 | 见“默认决策链”；不得省略数据有效性、去 DC/阈值、特征与换算 |
| 5 | 步骤原因 | 见“每一步为什么存在” |
| 6 | 默认算法 | 数字边沿用 Capture；高 SNR 单音用 crossing 插值；一般未知单音用 Hann FFT 插值 |
| 7 | 可选增强 | 多周期平均、MAD、自相关、窄范围 SineFit4 |
| 8 | 适用条件 | 稳态或慢变重复信号，Fs/N/时钟来源已知 |
| 9 | 不适用条件 | 削顶、帧内快速变频、近邻强音未分离、Timer 溢出未处理 |
| 10 | 采样率 | crossing 建议约 20 点/周期；FFT/谐波同时满足 Nyquist 与抗混叠余量 |
| 11 | 点数/周期数 | 至少 3 个同向 crossing，建议 5～20 周期；FFT N 为 2 次幂 |
| 12 | 抗噪 | 滞回 crossing、Hann、多周期结果 Median/MAD |
| 13 | 精度增强 | crossing/FFT 三点插值、延长观测时间、校准采样时钟 |
| 14 | 计算量 | crossing O(N)，FFT O(N log N)，相关 O(NL) |
| 15 | RAM | crossing O(E)；FFT 约 `8N+2N` 字节外加时域帧；相关输出 `4(2L+1)` |
| 16 | Primitive | ZeroCross、ZeroCrossInterpolation、MultiCycleAverage、Window、FFT、Magnitude、Peak、FFT interpolation、Autocorrelation |
| 17 | 仓库路径 | `03_measurement/frequency_zero_cross`、`04_dsp/{fft,fft_magnitude,peak_detect,autocorrelation}`、`05_precision/{zero_cross_interpolation,multi_cycle_average,fft_parabolic_interpolation}` |
| 18 | 伪代码 | `validate -> choose method -> extract same-direction feature -> average/interpolate -> Hz -> 1/Hz` |
| 19 | MCU 调用 | 见下方真实 API 最小链；采集 API 由目标 Application README 决定 |
| 20 | 失败排查 | 先查 Fs/时钟、方向混用、DC/滞回、边界 bin、N、Timer 溢出，再查噪声/前端 |

```c
/* events/positions/result 均由调用者分配；每一步检查 status。 */
SignalZeroCross_Process(voltage_v, N, &zc_cfg, events, event_capacity, &zc_result);
SignalZeroCrossInterpolation_Process(voltage_v, N, zc_cfg.threshold_v,
                                     events, zc_result.event_count,
                                     positions, position_capacity, &interp_result);
SignalMultiCycleAverage_Process(positions, interp_result.position_count,
                                sample_rate_hz, &frequency_result);
```
