# 20_fft_analysis

## 推荐复制函数

- FFT 测频：`PrepareSignal() + RunFFTCommon() + MeasureFFTFrequency()`。
- RAM 不足时：用 `FFT_COMMON_HELPER_LOW_RAM` 的 `RunQ15FFTLowRam()` 替换原
  `RunQ15FFT()`；1024 点可节省 2048 B 临时 Q15 幅值数组。
- Q15 幅度标定：`arm_cmplx_mag_q15` 输出为 Q2.14，恢复浮点量纲必须以
  `16384.0f` 为 1.0；Hann 多 bin RSS 再乘
  `coherent_gain / sqrt(power_gain)` 才是正弦峰值幅度。
- 双通道但 RAM 只够一套 FFT 工作区时：复制
  `FFT_COMMON_LOW_RAM_PARAMETERIZED`，对两个通道依次调用；每次调用后先保存
  本通道测量结果，再处理另一通道。
- 双通道 H1～Hn/THD：复制 `FFT_CHANNEL_HARMONICS_MEASURE`；它对单个输入
  通道完成完整 FFT 链并立即把结果复制到调用者输出数组。两个通道依次调用即可。
- 双通道调用点也希望复用时，继续复制
  `FFT_DUAL_CHANNEL_HARMONICS_MEASURE`；它内部依次调用前者，共用 FFT 工作区，
  返回值 bit0/bit1 分别表示 CH1/CH2 有效，一路无信号不会拖累另一路。
- 插值测频：额外复制 `RefineFFTFrequency()`。
- THD：额外复制 `AnalyzeHarmonicsAndTHD()`；需要自定最高谐波阶数时复制
  `AnalyzeHarmonicsAndTHDToOrder(last_order)`；SNR/SFDR：额外复制
  `AnalyzeSNRAndSFDR()`。
- 显示谱线：复制 `DrawFFTSpectrum()`。

`RunFFTCommon()` 是唯一执行 FFT 的函数；其余函数只复用 `fft_magnitude[]`。

## 1. 这个工程干什么

在一次 ADC 帧上执行一次 FFT，展示测频、插值、谐波、THD、SNR、SFDR 和频谱绘图数据。

## 2. 这个工程包含哪些子功能

| 我要做什么 | COPY 区域 |
|---|---|
| FFT 直接测频 | `FFT_COMMON_HELPER + FFT_COMMON + FFT_FREQUENCY` |
| FFT 插值测频 | `FFT_COMMON_HELPER + FFT_COMMON + FFT_FREQUENCY + FFT_PEAK_INTERPOLATION` |
| 谐波 | `FFT_COMMON_HELPER + FFT_COMMON + FFT_FREQUENCY + FFT_HARMONICS` |
| THD | `FFT_COMMON_HELPER + FFT_COMMON + FFT_FREQUENCY + FFT_HARMONICS + FFT_THD` |
| SNR/SFDR | `FFT_COMMON_HELPER + FFT_COMMON + FFT_FREQUENCY + FFT_SNR_SFDR` |
| 画频谱 | `FFT_COMMON_HELPER + FFT_COMMON + FFT_SPECTRUM_PLOT` |

## 3. 输入

`adc_samples`：`uint16_t` ADC code；`SAMPLE_COUNT`：已验证为 512 或 1024 的 CMSIS Q15 FFT 长度；`sample_rate_hz`：Hz。

## 4. 输出

`voltage_samples`、`centered_samples`、`fft_magnitude`、`peak_bin`、`interpolated_bin`、`frequency_hz`、`thd_percent`、`snr_db`、`sfdr_db`。

## 5. 公共数据链

`ADC → voltage → remove DC → Hann → Q15 FFT → magnitude`；只在 `FFT_COMMON` 运行一次。

## 6. 功能与 COPY 区对应表

见第 2 节。`FFT_COMMON_HELPER` 是 `FFT_COMMON` 所调用的 CMSIS Q15 helper；所有后续区都依赖同一 `fft_magnitude`，不可另行重复 FFT。

## 7. 使用的模块

`signal_dual_adc_mspm0g3507`、`signal_window`、`signal_window_gain_correction`、`signal_fft_parabolic_interpolation`、`signal_harmonic`、`signal_thd`、`signal_snr`、`signal_sfdr`、CMSIS-DSP。依据：restored example04 `App_Spectrum`、`App_RecipeCMSISSpectrumQ15` 和模块真实头文件。

独立复制时，FFT 分析算法文件必须成组带入：`signal_algorithm_status.h`、`signal_window.c/.h`、`signal_window_gain_correction.c/.h`、`signal_fft_parabolic_interpolation.c/.h`、`signal_multi_bin_energy.c/.h`、`signal_harmonic.c/.h`、`signal_thd.c/.h`、`signal_snr.c/.h`、`signal_sfdr.c/.h`。若还要显示结果，另带入 `signal_status.h` 及完整 TFT 三件组：`signal_tft_st7789.c/.h`、`signal_tft_st7789_mspm0g3507.c/.h`、`signal_tft_st7789_font.c/.h` 和 `signal_tft_st7789_font_data.inc`。ADC 采集模块按所选采集工程复制；复用测试使用 `02_adc_dma` 的 `signal_adc_dma.c/.h`。

## 8. SysConfig / 引脚

复制 restored example04 的完整 ADC/DMA profile；未新分配引脚。

## 9. main.c 流程

获取一帧后先运行 `FFT_COMMON`，再按需要启用其他 COPY 区。

## 10. 每个 COPY 区说明

`FFT_COMMON` 输出所有公共数组；测频给整数 bin；插值精修 bin；谐波/THD 和指标只消费谱数组。

## 11. 如何复制到新工程

复制实际 `modules` 文件、所需 COPY 区和 CMSIS-DSP 链接配置。已有 ADC DMA 时可直接将 `adc_samples` 接到 `FFT_COMMON`。

## 12. 可调参数

`SIGNAL_SAMPLE_COUNT`、`SIGNAL_SAMPLE_RATE_HZ`、Hann 窗、谐波最高阶和谱显示区域。

## 13. 常见错误

FFT 长度不是 Q15 支持值、`peak_bin` 位于边界、采样率不是真实值、或未跳过 DC bin。

## 14. 本工程没有做什么

不创建新的 FFT 包装模块；显示区域仅给出以 `fft_magnitude` 绘图的 COPY 接口。

## 15. Build 状态

SysConfig 1.28 Generate、TI Arm Clang 5.1 Compile/Link 已通过；实板为 `NOT_RUN`。
