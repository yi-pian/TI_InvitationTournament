# Reuse Test 01 — Signal Analyzer

来源 COPY 区：`02/ADC_DMA`、`30/BASIC_MEASUREMENT`、`20/FFT_COMMON_HELPER + FFT_COMMON + FFT_FREQUENCY + FFT_PEAK_INTERPOLATION + FFT_HARMONICS + FFT_THD + FFT_SNR_SFDR`、`21/TIME_DOMAIN_PLOT`、`80/TFT_TEXT + TFT_VARIABLE + TFT_LIVE_VALUE`。

为容纳 ADC DMA 与 TFT，使用 `PROFILE_06_FULL_SIGNAL` 的既有硬件资源，并仅把 ADC0/Timer/DMA 的逻辑实例名改为 02 模块实际需要的 `SIGNAL_ADC`、`SIGNAL_SAMPLE_TIMER`、`SIGNAL_ADC_DMA`；没有改引脚、DMA 通道或外围设备。

统一变量无重命名：`adc_samples`、`SAMPLE_COUNT`、`sample_rate_hz`、`voltage_samples`、`centered_samples`、`fft_magnitude`、`frequency_hz`、`mean_v`、`vpp_v`、`rms_v`、`ac_rms_v`、`thd_percent`、`snr_db`、`sfdr_db`。
