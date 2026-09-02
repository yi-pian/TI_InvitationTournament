# example04 模块复制清单

复制原则与 22_X 相同：只复制当前正式模块；普通测量和 FFT 基元按
`MSPM0_Signal_Contest/00_docs/recipes/` 直接写入 `main.c`，不复制冻结兼容
`.c/.h`。正式库来源文件没有修改；本题新增内容仅为 ST7789 字库和应用组合代码。

| 功能组 | 正式库来源目录 | 复制到 `modules/` 的文件 | SysConfig | 本题用途 |
|---|---|---|---|---|
| 双 ADC DMA | `02_acquisition/adc_dual_sync` | `signal_dual_adc_mspm0g3507.c/.h` | 是 | CH1/CH2 同步采集 |
| 单次捕获复现组合 | `02_acquisition/single_capture_replay` | `signal_single_capture_replay.c/.h` | 沿用依赖配置 | 连续触发、裁剪、三槽存储、安全绘图和 DAC 周期复现 |
| 触发捕获 | `02_acquisition/trigger_capture` | `signal_trigger_capture.c/.h` | ADC DMA 触发链 | 单次波形捕获 |
| 定时器捕获 | `02_acquisition/timer_capture` | `signal_timer_capture.c/.h`, `signal_timer_capture_mspm0g3507.c/.h` | 是 | Comparator 硬件测频 |
| 统一波形输出 | `06_generator/wave_output` | `signal_wave_output_mspm0g3507.c/.h` | 沿用 DAC DMA | `SignalWaveOutput_*WithOffset(frequency_hz, vpp_v, offset_v)` 四种三参数入口 |
| 波表与 DDS | `06_generator/dac_wave_table`, `06_generator/dds` | `signal_dac_wave_table.c/.h`, `signal_dds.c/.h` | 否 | 统一封装的内部依赖 |
| DAC DMA | `06_generator/dac_dma` | `signal_dac_dma_mspm0g3507.c/.h` | 是 | OUT 连续输出 |
| 波形发生 | `06_generator/sine`, `square`, `triangle`, `sawtooth` | 四组 `signal_sine/square/triangle/sawtooth.c/.h` | 否 | 四种可编程波形 |
| 任意波 | `06_generator/arbitrary_wave` | `signal_arbitrary_wave.c/.h` | 否 | 捕获数据重采样复现 |
| ADC 电压 | `00_docs/recipes/adc_to_voltage.md` | 无 `.c/.h`；`main.c` 的 `App_RecipeADCToVoltage` | 否 | ADC code 转电压 |
| 基本测量 | `00_docs/recipes/{mean,minmax,vpp,rms,ac_rms}.md` | 无 `.c/.h`；CMSIS-DSP 直调 | 否 | DC、最大最小、Vpp、RMS、AC RMS |
| 标准差 | 应用层 Welford 单遍计算 | 无 `.c/.h` | 否 | 保持原总体标准差显示语义 |
| 过零测频 | `03_measurement/frequency_zero_cross`, `05_precision/zero_cross_interpolation` | `signal_zero_cross*.c/.h` | 否 | ADC 时域过零与亚采样位置 |
| 去直流 | `00_docs/recipes/remove_dc.md` | 无 `.c/.h`；CMSIS `arm_mean_f32` + `arm_offset_f32` | 否 | FFT/过零预处理 |
| FFT | `00_docs/recipes/cmsis_fft_spectrum.md`、`05_precision/fft_parabolic_interpolation` | 无 FFT 基元 `.c/.h`；Q15 CMSIS 直调 + `signal_fft_parabolic_interpolation.c/.h` | CMSIS-DSP | 频率、幅值、插值 |
| 窗函数 | `04_dsp/window`, `05_precision/window_gain_correction` | `signal_window.c/.h`, `signal_window_gain_correction.c/.h` | 否 | Rect/Hann/Hamming/Blackman |
| 频谱指标 | `00_docs/recipes/peak_detect.md`、`04_dsp/harmonic`, `thd`, `snr`, `sfdr`, `04_dsp/multi_bin_energy` | Peak 无 `.c/.h`；其余对应 `signal_*.c/.h` | 否 | 基波、H2/H3、THD、SNR、SFDR |
| 削顶 | `00_docs/recipes/clipping_detect.md` | 无 `.c/.h`；`main.c` 直计数 | 否 | 超量程提示 |
| 抗毛刺 | `04_dsp/mad`, `median_filter`, `hampel_filter`, `05_precision/robust_peak_to_peak`, `robust_rms` | 对应 `signal_*.c/.h` | 否 | RAW/Median/Hampel、Robust Vpp/RMS |
| 精密正弦 | `05_precision/sine_fit_3param`, `sine_fit_4param` | 两组 `signal_sine_fit*.c/.h` | 否 | 精修频率、幅值、相位 |
| Lock-In | `05_precision/lock_in` | `signal_lock_in.c/.h` | 否 | 已知 DDS 参考下的微弱信号 |
| 校准 | `05_precision/adc_gain_offset_calibration`, `channel_delay_calibration` | 两组 `signal_*calibration.c/.h` | 否 | ADC 两点校准、双通道延迟 |
| Comparator | `07_signal_frontend/comparator_zero_cross`, `07_signal_frontend/comparator` | `signal_comparator_zero_cross.c/.h`, `signal_comparator.c/.h` | 是 | CH1 整形和捕获输入 |
| 矩阵键盘 | `01_bsp/matrix_keypad_4x4` | `signal_matrix_keypad_4x4.c/.h` | GPIO 是 | 参数、页面、滤波和窗选择 |
| ST7789 | `12_external_devices/display/st7789` | `signal_tft_st7789.c/.h`, `signal_tft_st7789_mspm0g3507.c/.h` | SPI/GPIO 是 | 屏幕驱动 |
| ST7789 字库 | `22_X` ILI9341 字库资源移植 | `signal_tft_st7789_font.c/.h`、`signal_tft_st7789_font_data.inc` | 否 | 与 ILI9341 相同的 6×12 / 8×16 / 12×24 / 16×32 ASCII 字体与两个示例汉字 |
| ST7789 旧文字辅助 | `12_external_devices/display/st7789_text_3x5` | `signal_tft_st7789_text.c/.h` | 否 | 保留兼容；本题界面已改用 ST7789 字库 |

## 本工程硬件底座

`signal_contest_template.syscfg` 以正式库 `09_examples/integration_profiles/PROFILE_06_FULL_SIGNAL/profile.syscfg` 为底座，追加 ST7789 SPI1（PB9/PB8/PB6）、DC PB15、背光 PB12，以及固定矩阵键盘行 PB16/PB0/PB7/PB17、列 PB18/PB13/PB20/PB4。ADC0/PA25 为 CH1，ADC1/PA17 为 CH2，DAC DMA 使用 DMA_CH1，ADC 使用 DMA_CH0/DMA_CH2，Comparator 使用 COMP0/PA27，TIMG7 做 Timer Capture。

## 修改边界

- 正式模块 `.c/.h`：未修改。
- `signal_tft_st7789_font.c/.h`、`signal_tft_st7789_font_data.inc`：新增字库模块；点阵数据原样来自 22_X 的 ILI9341 字库，绘制接口改为 ST7789。
- `signal_tft_st7789_text.c/.h`：旧的 5×7 辅助模块保留兼容，不属于对正式驱动的修改。
- `main.c`、`signal_config.h`、本工程 `.syscfg`、文档：按本题编写；main 的捕获链只保留组合模块初始化、COMP 回调、中断通知、按键和页面逻辑。普通测量、去直流、FFT 和削顶采用 Recipe，不保留兼容模块。
- 当前状态：`BOARD NOT RUN`；尚未执行 CCS 全量 Generate/Build 和实板验证。
