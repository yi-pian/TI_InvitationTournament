# MSPM0 比赛现场教学工程索引

## 按我要做什么查找

| 我要做什么 | 主题工程 | 推荐复制函数 / COPY 区 |
|---|---|---|
| 单点读 ADC code | `01_adc_basic` | `ReadADCOnce()` / `ADC_BASIC` |
| 取得单路 ADC DMA 帧 | `02_adc_dma` | `InitADC() + AcquireADCFrame()` / `ADC_DMA` |
| 取得同步双 ADC 帧 | `04_dual_adc_dma` | `InitDualADC() + AcquireDualADCFrame()` / `DUAL_ADC_DMA` |
| Timer/比较器硬件测频 | `10_timer_frequency` | `InitTimerFrequencyMeasurement() + MeasureTimerFrequency()` / `TIMER_CAPTURE` |
| ADC 过零测频 | `11_zero_cross_frequency` | `PrepareSignal() + MeasureFrequencyZeroCross()` / `ZERO_CROSS_PREPARE + ZERO_CROSS_MEASURE` |
| 单帧滤波：均值/中值/Hampel/FIR/IIR | `15_filter_processing` | `MOVING_AVERAGE`、`MEDIAN_HAMPEL`、`FIR_IIR` |
| FFT 直接/插值测频 | `20_fft_analysis` | `PrepareSignal() + RunFFTCommon() + MeasureFFTFrequency() (+ RefineFFTFrequency())` |
| THD、SNR、SFDR、谐波 | `20_fft_analysis` | 见该 README 对照表 |
| TFT 时域波形 | `21_time_domain_waveform` | `PrepareDisplaySamples() + DrawTimeDomainWaveform()` |
| 双通道/XY 波形显示 | `21_waveform_display` | `WAVEFORM_PREPARE + WAVEFORM_DRAW` |
| FFT 频谱显示（不做 FFT） | `22_spectrum_display` | `SPECTRUM_DISPLAY` |
| 软件触发、预触发、后触发提取 | `23_trigger_capture` | `TRIGGER_CAPTURE` |
| 显示自动量程/硬件增益建议 | `24_auto_range` | `AUTO_RANGE` |
| ADC 两点与通道延迟校准 | `25_calibration` | `ADC_GAIN_OFFSET_CALIBRATION + CHANNEL_DELAY_CALIBRATION` |
| DC、Vpp、RMS、AC RMS | `30_basic_measurement` | `ConvertADCToVoltage() + MeasureBasicParameters()` |
| 双通道相位/延迟 | `40_dual_channel_measurement` | `MeasurePhase() + CalculateDelayFromPhase()` |
| Median/Hampel/MAD/鲁棒 Vpp/RMS | `50_robust_measurement` | 对应同名 COPY 区 |
| 正弦拟合 | `60_precision_measurement` | `SINE_FIT_3PARAM` / `SINE_FIT_4PARAM` |
| Lock-In | `61_lock_in` | `ConvertADCToVoltage() + RunLockIn()` |
| 键盘读取、翻页、参数调整 | `70_keypad_usage` | `ReadKeypad()`、`HandlePageSwitch()`、`HandleNumberInput()`、`HandleParameterAdjust()` |
| TFT 文本/变量/两页 | `80_tft_usage` | `DrawStaticText()`、`UpdateLiveValue()`、`DrawPage()` |
| DDS/DAC DMA 波形 | `90_dds_usage` | `InitDDSOutput() + SetDDSFrequency()` |
| DAC 固定 DC | `91_dac_usage` | `SetDACDC()` |

## 统一接口速查

- 单路 ADC 原始码：`adc_samples`；双路：`adc_ch1_samples`、`adc_ch2_samples`。
- 点数：`SAMPLE_COUNT`；实际采样率：`sample_rate_hz`；DMA 完成：`adc_frame_ready`。
- 电压：`voltage_samples`；去 DC：`centered_samples`；频谱：`fft_magnitude`。
- 最终量：`frequency_hz`、`mean_v`、`vpp_v`、`rms_v`、`phase_deg`、`delay_s`、`thd_percent`、`snr_db`、`sfdr_db`。

完整约定见 [SINGLE_FUNCTION_INTERFACE_STANDARD.md](SINGLE_FUNCTION_INTERFACE_STANDARD.md)，逐块复制关系见 [COPY_BLOCK_INDEX.md](COPY_BLOCK_INDEX.md)。

## 推荐学习顺序

`01 → 02 → 04 → 80 → 70 → 30 → 10/11 → 15 → 20 → 21 → 21_display/22/23/24/25 → 40 → 50 → 60/61 → 90/91`。
