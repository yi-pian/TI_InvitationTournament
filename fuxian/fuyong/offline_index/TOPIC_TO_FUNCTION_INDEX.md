# 按“我要做什么”查函数

## 我要采 ADC 数据

- 单点、最小 DriverLib 示例：`01_adc_basic` → `ReadADCOnce()`。
- 单路一帧 DMA：`02_adc_dma` → `InitADC()`、`AcquireADCFrame()`。
- 两路同步 DMA：`04_dual_adc_dma` → `InitDualADC()`、`AcquireDualADCFrame()`。

选择标准：只测一个信号优先单路；需要相位、两路对比或同步关系必须双路。

## 我要测频

- 硬件边沿、方波/比较器输出：`10_timer_frequency` → `MeasureTimerFrequency()`。
- ADC 中低频平滑周期波：`11_zero_cross_frequency` → `PrepareSignal()`、`MeasureFrequencyZeroCross()`。
- 未知/复杂波、还要频谱指标：`20_fft_analysis` → `PrepareSignal()`、`RunFFTCommon()`、`MeasureFFTFrequency()`、`RefineFFTFrequency()`。

不要把三种方法当作无条件可互换：Timer 需要硬件 Capture 边沿；过零需要足够 crossing；FFT 需要 N、Fs、窗口与 Nyquist 条件正确。

## 我要测普通幅值

`30_basic_measurement`：`ConvertADCToVoltage()`、`MeasureBasicParameters()`。

直接输出：`mean_v`、`minimum_v`、`maximum_v`、`vpp_v`、`rms_v`、`ac_rms_v`、`population_stddev_v`、`clipping`。

## 我要在干扰/尖峰下测幅值

`50_robust_measurement`：`ConvertADCToVoltage()`、`ApplySelectedFilter()`、`AnalyzeRobustStatistics()`。

按现场情况选择 RAW、Median 或 Hampel；输出 `mad_v`、`robust_vpp_v`、`robust_rms_v`、`outlier_count`。

## 我要测相位差或延迟

`40_dual_channel_measurement`：`AcquireDualADCFrame()`、`MeasurePhase()`、`CalculateDelayFromPhase()`。

输出 `phase_deg`，再结合外部已知/已测的 `reference_frequency_hz` 得 `delay_s`。

## 我要 FFT、谐波、THD、SNR、SFDR

`20_fft_analysis`：

1. `PrepareSignal()`；
2. `RunFFTCommon()`；
3. `MeasureFFTFrequency()`；
4. 可选 `RefineFFTFrequency()`；
5. `AnalyzeHarmonicsAndTHD()`；
6. `AnalyzeSNRAndSFDR()`。

所有步骤共用唯一 `fft_magnitude[]`。

## 我要显示时域波形或数值

- 时域波形：`21_time_domain_waveform` → `DrawTimeDomainWaveform()`。
- TFT 基础文字/变量/页面：`80_tft_usage` → `InitTFTDemo()`、`DrawStaticText()`、`UpdateLiveValue()`、`DrawPage()`。

## 我要做正弦精密测量

`60_precision_measurement`：`RunSineFit3Param()`、`RunSineFit4Param()`。

3P 适用于已知频率；4P 需要可信的 `initial_frequency_hz`，可由 20 的 FFT 插值结果提供。

## 我要做 Lock-In

`61_lock_in`：`ConvertADCToVoltage()`、`RunLockIn()`。

输入必须有正确的 `reference_frequency_hz`；若由 MCU 自己输出激励，参考可与 DDS 频率保持同源。

## 我要接键盘、切页、输数字

`70_keypad_usage`：`ReadKeypad()`、`HandlePageSwitch()`、`HandleNumberInput()`、`HandleParameterAdjust()`。

## 我要输出正弦或固定直流

- DDS/DAC DMA 连续正弦：`90_dds_usage` → `InitDDSOutput()`、`SetDDSFrequency()`。
- 固定 DAC DC：`91_dac_usage` → `SetDACDC()`。

连续波形不要从 `91_dac_usage` 的固定 code 循环拼出来。
