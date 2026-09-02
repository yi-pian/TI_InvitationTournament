# 一页速查表

| 需求 | 教学工程 | 优先复制函数 | 主要输入 | 最终输出 | 同帧注意 |
|---|---|---|---|---|---|
| 单点 ADC 读数 | 01_adc_basic | `ReadADCOnce()` | 模拟输入 | `adc_samples[0]` code | 不是 DMA 帧。 |
| 单路 ADC DMA | 02_adc_dma | `InitADC()`、`AcquireADCFrame()` | ADC/DMA | `adc_samples[]`、`sample_rate_hz` | 每帧一次。 |
| 同步双 ADC | 04_dual_adc_dma | `InitDualADC()`、`AcquireDualADCFrame()` | 双 ADC/DMA | `adc_ch1_samples[]`、`adc_ch2_samples[]` | 每帧一次。 |
| Timer 测频/占空比 | 10_timer_frequency | `MeasureTimerFrequency()` | Capture 边沿 | `frequency_hz`、`duty_cycle_percent` | 不用 ADC。 |
| 过零插值测频 | 11_zero_cross_frequency | `PrepareSignal()`、`MeasureFrequencyZeroCross()` | `adc_samples[]`、Fs | `frequency_hz` | 去 DC 一次。 |
| FFT 测频 | 20_fft_analysis | `PrepareSignal()`、`RunFFTCommon()`、`MeasureFFTFrequency()` | ADC code、Fs | `frequency_hz` | FFT 一次。 |
| FFT 插值测频 | 20_fft_analysis | 额外 `RefineFFTFrequency()` | `fft_magnitude[]` | 精修 `frequency_hz` | 不重做 FFT。 |
| THD/SNR/SFDR | 20_fft_analysis | `AnalyzeHarmonicsAndTHD()`、`AnalyzeSNRAndSFDR()` | `fft_magnitude[]` | `%`、dB | 不重做 FFT。 |
| 时域波形 | 21_time_domain_waveform | `DrawTimeDomainWaveform()` | `adc_samples[]` | TFT 折线 | 与 FFT 可共用 ADC 帧。 |
| DC/Vpp/RMS | 30_basic_measurement | `ConvertADCToVoltage()`、`MeasureBasicParameters()` | ADC code | `mean_v`、`vpp_v`、`rms_v` | 电压转换一次。 |
| 双通道相位 | 40_dual_channel_measurement | `MeasurePhase()`、`CalculateDelayFromPhase()` | 同步双 ADC、Fs | `phase_deg`、`delay_s` | 双 ADC 帧一次。 |
| 抗尖峰幅值 | 50_robust_measurement | `ApplySelectedFilter()`、`AnalyzeRobustStatistics()` | `voltage_samples[]` | `robust_vpp_v`、`robust_rms_v` | 共享 filter 输出。 |
| 正弦精密拟合 | 60_precision_measurement | `RunSineFit3Param()`、`RunSineFit4Param()` | 电压、Fs、初频 | 幅值/相位/DC/频率 | 初频可来自 FFT。 |
| 弱信号 Lock-In | 61_lock_in | `RunLockIn()` | 电压、Fs、参考频率 | `amplitude_v`、`phase_deg` | 不负责盲测频。 |
| 键盘 | 70_keypad_usage | `ReadKeypad()`、`Handle*()` | 4×4 键盘 | UI 状态 | 与测量数据链独立。 |
| TFT | 80_tft_usage | `InitTFTDemo()`、`UpdateLiveValue()` | 结果变量 | 屏幕 | 不修改算法结果。 |
| DDS/DAC DMA | 90_dds_usage | `InitDDSOutput()`、`SetDDSFrequency()` | `frequency_hz` | 连续 DAC 波形 | 与 ADC 数据链独立。 |
| DAC DC | 91_dac_usage | `SetDACDC()` | `dac_code` | DC 电压 | 不适用于连续波。 |
