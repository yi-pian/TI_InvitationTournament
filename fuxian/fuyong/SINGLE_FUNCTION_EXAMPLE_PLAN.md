# MSPM0 主题式单功能教学工程计划

本目录是比赛现场的代码参考库，不是新的软件框架。每个工程从 `template_original/signal_contest_template_final` 独立复制；教学组合逻辑只在该工程的 `main.c`，已有模块 `.c/.h` 只复制、不修改。

| 主题工程 | 主要数据链 | 子功能 / COPY 区 | 代码依据 | SysConfig 来源 | 状态 |
|---|---|---|---|---|---|
| `01_adc_basic` | ADC polling → code | `ADC_BASIC` | `02_acquisition/adc_basic` README/.h | `PROFILE_07_BASIC_IO` | 已生成；Generate blocked |
| `02_adc_dma` | ADC DMA → `adc_samples` | `ADC_DMA` | `adc_dma` README/.h | `PROFILE_01_ADC_CAPTURE` | 已审计；Generate blocked |
| `03_adc_pingpong_dma` | ping-pong DMA → 连续帧 | `PINGPONG_DMA` | `adc_pingpong_dma` README/.h | 待确认的原 profile | 审计中 |
| `04_dual_adc_dma` | sync ADC DMA → 双通道 | `DUAL_ADC_DMA` | `adc_dual_sync` README/.h | restored example04 | 已审计；syntax pass |
| `10_timer_frequency` | comparator → timer capture | `TIMER_CAPTURE` | `timer_capture` README/.h | `PROFILE_05_FREQUENCY` | 已生成；syntax pass |
| `11_zero_cross_frequency` | ADC → voltage → centered → crossings | `ZERO_CROSS_COMMON` / `ZERO_CROSS_FREQUENCY` / `ZERO_CROSS_INTERPOLATION` | example04 + zero-cross headers | restored example04 | 已生成；syntax pass |
| `20_fft_analysis` | ADC → voltage → centered → window → FFT → magnitude | `FFT_COMMON` / `FFT_FREQUENCY` / `FFT_PEAK_INTERPOLATION` / `FFT_HARMONICS` / `FFT_THD` / `FFT_SNR_SFDR` / `FFT_SPECTRUM_PLOT` | example04 + module headers | restored example04 | 已生成；syntax pass |
| `21_time_domain_waveform` | ADC → screen mapping → TFT | `TIME_DOMAIN_PLOT` | example04 TFT driver API | restored example04 | 已生成；syntax pass |
| `30_basic_measurement` | ADC → voltage → basic statistics | `BASIC_MEASUREMENT` | example04 CMSIS recipe | restored example04 | 已生成；syntax pass |
| `40_dual_channel_measurement` | dual ADC → phase | `PHASE` | `signal_dual_adc_phase.h` + example04 | restored example04 | 已生成；syntax pass |
| `50_robust_measurement` | ADC → voltage → filtering/statistics | `MEDIAN_FILTER` / `HAMPEL_FILTER` / `MAD` / `ROBUST_VPP` / `ROBUST_RMS` | example04 + robust headers | restored example04 | 已生成；syntax pass |
| `60_precision_measurement` | ADC → voltage → sine fitting | `SINE_FIT_3PARAM` / `SINE_FIT_4PARAM` | example04 + sine-fit headers | restored example04 | 已生成；syntax pass |
| `61_lock_in` | ADC → voltage → I/Q | `LOCK_IN` | example04 + lock-in header | restored example04 | 已生成；syntax pass |
| `70_keypad_usage` | keypad scan → UI state | `KEY_READ` / `PAGE_SWITCH` / `NUMBER_INPUT` / `PARAMETER_ADJUST` | example04 + keypad header | restored example04 | 已生成；syntax pass |
| `80_tft_usage` | TFT init → text/value/pages | `TFT_TEXT` / `TFT_VARIABLE` / `TFT_LIVE_VALUE` / `TFT_TWO_PAGES` | example04 + ST7789 headers | restored example04 | 已生成；syntax pass |
| `90_dds_usage` | waveform table → DDS → DAC DMA | `DDS_INIT` / `DDS_FIXED_FREQUENCY` / `DDS_SET_FREQUENCY` / `DDS_KEY_ADJUST` | example04 + wave output header | restored example04 | 已生成；syntax pass |
| `91_dac_usage` | DAC DC → PA15 | `DAC_DC` / `DAC_WAVEFORM` | `dac_dc` README | `PROFILE_07_BASIC_IO` | 已生成；syntax pass |

`01_adc_basic`、`03_adc_pingpong_dma`、`91_dac_usage` 只有在 README、真实头文件和可复用 SysConfig 三项均确认后才创建；缺一项即记入 `UNSUPPORTED_OR_NOT_READY.md`，不会猜测接口或引脚。

`20_fft_analysis` 只在 `FFT_COMMON` 中做一次 FFT；其余 COPY 区复用 `fft_magnitude`。工程之间允许教学代码重复，禁止为去重新增 Core、Feature、Context、Cache 或其他框架文件。
