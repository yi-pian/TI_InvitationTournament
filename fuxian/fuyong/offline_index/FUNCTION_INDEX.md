# 可复制教学函数索引

以下名称、输入输出均以当前 `fuyong/*/main.c` 为准。复制一个函数时，也要复制它明确依赖的全局数组、配置常量和模块；不要只复制函数体。

## ReadADCOnce()

- 来源：`01_adc_basic`；COPY：`ADC_BASIC`。
- 功能：软件触发一次 ADC0 转换。
- 输入：ADC 模拟输入、SysConfig 的 ADC/MEM0 宏。
- 输出：`adc_samples[0]`，`uint16_t` ADC code。
- 依赖：`SYSCFG_DL_init()`；无帧 DMA。

## InitADC() / AcquireADCFrame()

- 来源：`02_adc_dma`；COPY：`ADC_DMA`。
- 功能：初始化单路 ADC/Timer/DMA，并取得一帧 DMA 数据。
- 输入：`s_adc_config`、`SAMPLE_COUNT`。
- 输出：`adc_samples[]`（ADC code）、`sample_rate_hz`（Hz）、`adc_frame_ready`。
- 依赖：`signal_adc_dma` 与对应 SysConfig；后续 Basic、FFT、绘图共享此帧。

## InitDualADC() / AcquireDualADCFrame()

- 来源：`04_dual_adc_dma`；COPY：`DUAL_ADC_DMA`。
- 功能：初始化并采集公共 Timer 触发的同步双通道帧。
- 输入：双 ADC/DMA SysConfig、`SAMPLE_COUNT`。
- 输出：`adc_ch1_samples[]`、`adc_ch2_samples[]`（ADC code）、`sample_rate_hz`（Hz）。
- 依赖：`signal_dual_adc_mspm0g3507`；Phase、双通道题目只能使用这一类同步帧。

## InitTimerFrequencyMeasurement() / MeasureTimerFrequency()

- 来源：`10_timer_frequency`；COPY：`TIMER_CAPTURE`。
- 功能：初始化比较器/TIMG Capture 测量，并读取硬件频率与占空比。
- 输入：Capture 事件、定时器时钟、load value。
- 输出：`frequency_hz`（Hz）、`duty_cycle_percent`（%）。
- 依赖：`signal_timer_capture_mspm0g3507` 与 Capture SysConfig；不依赖 ADC。

## AcquireADCFrame()、PrepareSignal()、MeasureFrequencyZeroCross()

- 来源：`11_zero_cross_frequency`；COPY：`ZERO_CROSS_PREPARE`、`ZERO_CROSS_MEASURE`。
- 功能：采一帧 → ADC code 转 V → 计算 `mean_v` → 得到 `centered_samples[]` → 上升过零/线性插值/多周期平均。
- 输入：`adc_samples[]`、`SIGNAL_SAMPLE_COUNT`、`sample_rate_hz`。
- 输出：`voltage_samples[]`（V）、`mean_v`（V）、`centered_samples[]`（V）、`frequency_hz`（Hz）。
- 依赖：双 ADC 驱动、CMSIS-DSP、`signal_zero_cross`、`signal_zero_cross_interpolation`。
- 复制前置：若新项目已有有效 ADC 帧，可省 `AcquireADCFrame()`，但必须保留同格式数组与真实 Fs。

## PrepareSignal()（FFT） / RunQ15FFT() / RunFFTCommon()

- 来源：`20_fft_analysis`；COPY：`FFT_PREPARE`、`FFT_COMMON`。
- 功能：FFT 版 `PrepareSignal()` 完成 ADC code→V、`mean_v`、去 DC；`RunFFTCommon()` 做 Hann、Q15 FFT 和窗增益修正。
- 输入：`adc_samples[]`、`centered_samples[]`、`sample_rate_hz`。
- 输出：`voltage_samples[]`（V；加窗后用作临时输入）、`fft_magnitude[]`（单边幅度）。
- 依赖：`RunQ15FFT()`、`fft_q15[]`、`fft_magnitude_q15[]`、CMSIS Q15 FFT、Window/Gain-Correction 模块。
- 重要：`RunQ15FFT()` 是 `RunFFTCommon()` 的内部 helper，不单独作为主流程入口；同一帧只能调用一次 `RunFFTCommon()`。

## MeasureFFTFrequency() / RefineFFTFrequency()

- 来源：`20_fft_analysis`；COPY：`FFT_FREQUENCY`、`FFT_PEAK_INTERPOLATION`。
- 功能：找非 DC 最大谱线，再以三点抛物线精修频率。
- 输入：`fft_magnitude[]`、`sample_rate_hz`。
- 输出：`peak_bin`、`peak_value`、`interpolated_bin`、`frequency_hz`。
- 依赖：`RunFFTCommon()`；精修函数还依赖 `signal_fft_parabolic_interpolation`。
- 适用：FFT 测频、频谱分析、为 Sine Fit 提供 `initial_frequency_hz`。

## AnalyzeHarmonicsAndTHD() / AnalyzeSNRAndSFDR() / DrawFFTSpectrum()

- 来源：`20_fft_analysis`；COPY：`FFT_HARMONICS_THD`、`FFT_SNR_SFDR`、`FFT_SPECTRUM_PLOT`。
- 功能：从同一 `fft_magnitude[]` 提取 H1~H3/THD、SNR/SFDR，或画出谱线。
- 输入：`fft_magnitude[]`；谐波函数还需要 `frequency_hz`、`sample_rate_hz`；显示还需要 `tft`。
- 输出：`harmonics`、`thd_percent`、`snr_db`、`sfdr_db` 或 TFT 图形。
- 依赖：`RunFFTCommon()`；谐波分析应在 `RefineFFTFrequency()` 后调用；三者绝不再跑 FFT。

## AcquireADCFrame()、PrepareDisplaySamples()、DrawTimeDomainWaveform()

- 来源：`21_time_domain_waveform`；COPY：`TIME_DOMAIN_PREPARE`、`TIME_DOMAIN_PLOT`。
- 功能：采样、可选 ADC→V 统一数据准备、把 ADC code 映射为 TFT 时域折线。
- 输入：`adc_samples[]`、`SIGNAL_SAMPLE_COUNT`、已初始化 `tft`。
- 输出：`voltage_samples[]`（V）与 TFT 波形。
- 依赖：双 ADC、TFT ST7789；绘图的现有 Y 映射使用 ADC code 满量程，不把 code 当 V。

## ConvertADCToVoltage() / CalculatePopulationStdDev() / MeasureBasicParameters()

- 来源：`30_basic_measurement`；COPY：`BASIC_CONVERT`、`BASIC_MEASUREMENT`。
- 功能：ADC code 转 V；以 Welford 法算总体标准差；一次输出 Basic 全部量。
- 输入：`adc_samples[]` 或 `voltage_samples[]`，`SIGNAL_SAMPLE_COUNT`。
- 输出：`mean_v`、`minimum_v`、`maximum_v`、`vpp_v`、`rms_v`、`ac_rms_v`、`population_stddev_v`、`clipping`。
- 依赖：CMSIS-DSP；`MeasureBasicParameters()` 内部依赖 `CalculatePopulationStdDev()` 和 `centered_samples[]`。
- 注意：若已使用其他函数生成了同一帧 `voltage_samples[]`，只复制 `MeasureBasicParameters()`，不要再次复制转换函数。

## MeasurePhase() / CalculateDelayFromPhase()

- 来源：`40_dual_channel_measurement`；COPY：`PHASE_MEASURE`、`PHASE_DELAY`。
- 功能：由同步双 ADC frame 得相位，再用已知基波频率换算延迟。
- 输入：`adc_ch1_samples[]`、`adc_ch2_samples[]`、`sample_rate_hz`、`reference_frequency_hz`。
- 输出：`phase_deg`（deg）、`delay_s`（s）。
- 依赖：`AcquireDualADCFrame()`、`signal_dual_adc_phase`。
- 注意：模块直接输出的是相位；`delay_s = phase_deg / (360 × reference_frequency_hz)` 是应用层推导。

## ConvertADCToVoltage()、ApplyMedianFilter()、ApplyHampelFilter()、ApplySelectedFilter()、AnalyzeRobustStatistics()

- 来源：`50_robust_measurement`；COPY：`ROBUST_CONVERT`、`MEDIAN_FILTER`、`HAMPEL_FILTER`、`ROBUST_STATISTICS`。
- 功能：转换电压，按 RAW/Median/Hampel 选择一条滤波链，再计算 MAD、鲁棒 Vpp、鲁棒 RMS。
- 输入：`voltage_samples[]`（V）、`s_filter_mode`、`workspace[]`。
- 输出：`filtered_samples[]`（V）、`outlier_count`、`mad_v`、`robust_vpp_v`、`robust_rms_v`。
- 依赖：Median/Hampel/MAD/Robust Vpp/Robust RMS 原子模块。
- 注意：选一种滤波链；不要将 Median 与 Hampel 无意地都当成最终输入。

## ConvertADCToVoltage() / RunSineFit3Param() / RunSineFit4Param()

- 来源：`60_precision_measurement`；COPY：`SINE_FIT_CONVERT`、`SINE_FIT_3PARAM`、`SINE_FIT_4PARAM`。
- 功能：电压换算后作 3P（已知频率）或 4P（搜索频率）正弦拟合。
- 输入：`voltage_samples[]`（V）、`sample_rate_hz`、`initial_frequency_hz`。
- 输出：`amplitude_v`（V peak）、`phase_deg`（deg）、`mean_v`（V）、`frequency_hz`（Hz）。
- 依赖：Sine Fit 3P/4P 模块；4P 初频优先来自 FFT 插值结果。

## ConvertADCToVoltage() / RunLockIn()

- 来源：`61_lock_in`；COPY：`LOCK_IN_CONVERT`、`LOCK_IN`。
- 功能：把输入电压与已知参考频率作 I/Q 同步检测。
- 输入：`voltage_samples[]`（V）、`reference_frequency_hz`（Hz）、`sample_rate_hz`（Hz）。
- 输出：`amplitude_v`（V peak）、`phase_deg`（deg）、`lock_in_result`（含 I/Q）。
- 依赖：`signal_lock_in`；参考频率需由 DDS、题目已知值或 FFT 提供。

## ReadKeypad() / HandlePageSwitch() / HandleNumberInput() / HandleParameterAdjust()

- 来源：`70_keypad_usage`；COPY：`KEY_READ`、`PAGE_SWITCH`、`NUMBER_INPUT`、`PARAMETER_ADJUST`。
- 功能：读取一次新按键、切页、维护数字文本和调节参数。
- 输入：4×4 键盘事件 `key`。
- 输出：`current_page`、`input_text[]/input_length`、`adjustable_value`。
- 依赖：`signal_matrix_keypad_4x4`；与 ADC/FFT 数据链独立。

## InitTFTDemo() / DrawStaticText() / DisplayVariable() / UpdateLiveValue() / DrawPage()

- 来源：`80_tft_usage`；COPY：`TFT_TEXT`、`TFT_VARIABLE`、`TFT_LIVE_VALUE`、`TFT_TWO_PAGES`。
- 功能：初始化 ST7789、绘制固定文字、变量、局部刷新和页面。
- 输入：`tft`、`frequency_hz`（Hz）、`current_page`。
- 输出：屏幕内容。
- 依赖：ST7789 核心/平台/字库模块；显示函数不产生或修改测量数据。

## InitDDSOutput() / SetDDSFrequency() / HandleDDSFrequencyAdjust()

- 来源：`90_dds_usage`；COPY：`DDS_INIT`、`DDS_SET_FREQUENCY`、`DDS_KEY_ADJUST`。
- 功能：初始化波表和 DAC DMA，以 `frequency_hz` 更新带幅值/偏置的连续正弦。
- 输入：`wave_table[]`、`dac_output[]`、`frequency_hz`（Hz）。
- 输出：DAC DMA 波形、`dds_result`。
- 依赖：Wave Output/DAC DMA 模块和对应 SysConfig；键盘/串口输入只负责给出新频率。

## SetDACDC() / ExplainContinuousWaveformEntry()

- 来源：`91_dac_usage`；COPY：`DAC_DC`、`DAC_WAVEFORM`。
- 功能：输出固定 12 bit DAC 直流；说明连续波形必须转用 90 DDS。
- 输入：`dac_code`（0..4095）。
- 输出：DAC0/PA15 直流电压。
- 依赖：本工程 DAC SysConfig；`ExplainContinuousWaveformEntry()` 是教学说明函数，不生成波形。
