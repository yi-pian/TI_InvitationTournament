# COPY 区速查

| 功能 | 工程 | 函数 / COPY 区 | 必须同时复制 | 输入 | 输出 |
|---|---|---|---|---|---|
| ADC DMA | 02 | `AcquireADCFrame()` / `ADC_DMA` | `InitADC()`、SysConfig + ADC module | 模拟输入 | `adc_samples` |
| 双 ADC DMA | 04 | `AcquireDualADCFrame()` / `DUAL_ADC_DMA` | `InitDualADC()`、SysConfig + dual ADC | 模拟输入 | `adc_ch1_samples/ch2` |
| 过零插值频率 | 11 | `PrepareSignal() + MeasureFrequencyZeroCross()` | 两个完整函数 | ADC code/V/Fs | `frequency_hz` |
| FFT 频率 | 20 | `PrepareSignal() + RunFFTCommon() + MeasureFFTFrequency()` | `RunQ15FFT()` | ADC code/Fs | `frequency_hz` |
| FFT 插值 | 20 | `RefineFFTFrequency()` / `FFT_PEAK_INTERPOLATION` | PREPARE + COMMON + FREQUENCY | magnitude/peak | `interpolated_bin` |
| 谐波/THD | 20 | `AnalyzeHarmonicsAndTHD()` / `FFT_HARMONICS_THD` | PREPARE + COMMON + 插值 | magnitude | harmonics/`thd_percent` |
| SNR/SFDR | 20 | `AnalyzeSNRAndSFDR()` / `FFT_SNR_SFDR` | PREPARE + COMMON + FREQUENCY | magnitude | dB values |
| 时域图 | 21 | `PrepareDisplaySamples() + DrawTimeDomainWaveform()` | ADC acquire + TFT init | `adc_samples` | TFT 折线 |
| 基础统计 | 30 | `ConvertADCToVoltage() + MeasureBasicParameters()` | ADC acquire | `adc_samples` | Vpp/RMS/DC |
| 相位 | 40 | `MeasurePhase() + CalculateDelayFromPhase()` | dual ADC acquisition | 两路 code/Fs | phase/delay |
| 抗异常值 | 50 | `ApplyMedianFilter()`、`ApplyHampelFilter()`、`AnalyzeRobustStatistics()` | `ConvertADCToVoltage()` | V samples | 过滤/统计 |
| 正弦拟合 | 60 | `RunSineFit3Param()`/`RunSineFit4Param()` | `ConvertADCToVoltage()` | V/Fs/初频 | 幅相/DC/频率 |
| Lock-In | 61 | `RunLockIn()` / `LOCK_IN` | `ConvertADCToVoltage()` | V/参考频率/Fs | 幅相 |
| 键盘 | 70 | `ReadKeypad()`、`HandlePageSwitch()` 等 | key module | keypad | UI 状态 |
| TFT | 80 | `DrawStaticText()`、`UpdateLiveValue()`、`DrawPage()` | `InitTFTDemo()` | variable/page | screen |
| DDS | 90 | `InitDDSOutput()`、`SetDDSFrequency()` | DDS init | frequency | DAC waveform |
