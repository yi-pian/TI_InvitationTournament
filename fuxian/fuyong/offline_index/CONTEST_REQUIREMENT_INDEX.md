# 按电赛需求选择方案

## 需求：频率测量

| 现场条件 | 推荐 | 原因 | 避免 |
|---|---|---|---|
| 有比较器/方波边沿，需实时频率和占空比 | `10_timer_frequency` / `MeasureTimerFrequency()` | 硬件 Capture 直接测周期，CPU 负担低。 | 将 ADC 数组硬套为 Capture。 |
| ADC 采到干净低中频正弦/周期波 | `11_zero_cross_frequency` / `MeasureFrequencyZeroCross()` | 多周期过零插值简单、直观。 | 未去 DC、crossing 数不足仍输出旧频率。 |
| 波形未知、存在谐波或还要频谱 | `20_fft_analysis` / `RunFFTCommon()` + peak 函数 | 可同时给频率、频谱、THD/SNR/SFDR。 | 每个频谱指标重新 FFT。 |

## 需求：幅值、DC、RMS、Vpp

- 普通信号：`30_basic_measurement` / `MeasureBasicParameters()`。
- 有尖峰、偶发离群点：`50_robust_measurement` / `ApplySelectedFilter()` + `AnalyzeRobustStatistics()`。
- 接入实际电压前，确认 ADC `VREF`、前端比例与校准含义；教学工程默认线性换算不等于所有题目实际校准。

## 需求：相位差、时间延迟

- 推荐：`04_dual_adc_dma` + `40_dual_channel_measurement`。
- 必要条件：双 ADC 公共硬件触发；`delay_s` 使用已知或另行测得的基波频率换算。
- 不推荐：两次独立 ADC 采样后直接比较相位。

## 需求：谐波、THD、SNR、SFDR

- 推荐：`20_fft_analysis` 的 `RunFFTCommon()` 后依次运行峰值、插值、谐波/THD、SNR/SFDR。
- 前提：采样率满足 Nyquist，`SIGNAL_SAMPLE_COUNT` 与 CMSIS Q15 FFT 支持的 N 一致，窗口/窗增益修正保留。
- 注意：同一帧相同 N/Fs/window 只保留一套 FFT 工作区和一次 FFT。

## 需求：弱信号或已知频率响应

- 推荐：`61_lock_in` / `RunLockIn()`。
- 前提：有可靠的 `reference_frequency_hz`；DDS 激励时应让参考频率与 DDS 请求保持一致。
- 若参考频率未知，先用 Timer/Zero-Cross/FFT 得到频率，再决定是否可作 Lock-In。

## 需求：高精度正弦参数

- 推荐：`60_precision_measurement`。
- 已知频率：3 参数拟合；初频不确定：FFT 粗测/插值后输入 4 参数拟合。
- 保留 `initial_frequency_hz` 的物理含义，不能把 mean、幅值等变量临时塞入其中。

## 需求：波形显示和人机交互

- 波形：`21_time_domain_waveform`；数值/页面：`80_tft_usage`；键盘：`70_keypad_usage`。
- 将测量函数和 `UpdateLiveValue()` 分开：显示只读取结果，不重复采集或分析。

## 需求：产生激励

- 可调连续正弦：`90_dds_usage`。
- 固定 DC 偏置/控制电压：`91_dac_usage`。
- 组合测量时，DDS 产生函数不应触发第二条 ADC 数据链；ADC 仍只由一个采集入口产生。
