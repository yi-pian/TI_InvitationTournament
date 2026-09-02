# Module Composability / Platform Closure Audit

> 历史闭环快照。本文的 92→93→94 只描述 2026-08-09 至 2026-08-11 的迁移过程，不是当前总数。2026-08-13 当前机器真相为 Registry 101 条、FORMAL 81 条、selectable 71 条；当前可拼装状态需同时看 Module Card、SysConfig contract 和最新构建矩阵。

> 历史闭环说明：本文记录“旧 callback 架构是否有真实 MSPM0G3507 落地点”，不是当前比赛推荐层级。2026-08-10 起，简单硬件动作已改为 SysConfig + Direct DriverLib；最新结论以 [MODULE_ABSTRACTION_AUDIT.md](MODULE_ABSTRACTION_AUDIT.md) 和 [WHEN_TO_USE_DRIVERLIB_OR_MODULE.md](WHEN_TO_USE_DRIVERLIB_OR_MODULE.md) 为准。旧 Platform 代码仅兼容保留。

> 2026-08-11 增量：`adc_fifo_dma` 作为第 94 个正式模块加入并完成独立 SysConfig/compile/full-link；它不属于本文 2026-08-09 的 93 模块旧 callback 闭环统计，当前总数以 Module Catalog 为准。

审计日期：2026-08-09  
目标芯片：MSPM0G3507  
SDK/工具证据：MSPM0 SDK 2.11.00.07、SysConfig 1.28.0、TI Arm Clang 5.1.1.LTS

## 1. 结论

本次按真实 `.c/.h/README/MODULE_CARD` 扫描了：

- `MSPM0_Signal_Contest/01_bsp`、`02_acquisition`、`06_generator`、`07_signal_frontend`；
- `08_applications/common`，包括新建立的唯一 MSPM0G3507 平台层；
- 独立 `MSPM0_Signal_Contest/03_measurement`、`04_dsp`、`05_precision`。

正式模块由 92 个变为 **93 个**：不是新增功能算法，而是把重复/缺失的硬件 glue 收敛为 1 个正式 `MSPM0G3507 Platform Adapter` 模块。审计结果：

| 状态 | 数量 | 说明 |
|---|---:|---|
| READY | 86 | 纯软件可直接调用，或硬件链已经到正式 adapter/DriverLib/SysConfig |
| API_GAP | 7 | OPA/GPAMP 抽象无法表达真实 MSPM0G3507 离散配置 |
| DOC_GAP | 0 | 本轮核心 callback README 已补完整闭环 |
| PLATFORM_GAP | 0 | 已识别的通用 callback 均有正式平台实现；OPA/GPAMP 是 API 问题而非少一层胶水 |
| SYSCONFIG_GAP | 0 | READY 硬件模块均给出具体 Profile/示例路径 |
| EXAMPLE_GAP | 0 | 核心硬件链已有最小代码或完整工程；10 条代表链完成 full link |

这里的 `READY` 只回答“能否按 README 拼装”，不等于 `BOARD_VERIFIED`。本轮没有开发板实测。

## 2. 全部正式模块审计表

### A. BSP / Hardware（15）

| Module | 类型 | 依赖 Platform | 正式 Platform 实现 | README 说明 | 仅靠 README 可拼装 | 状态 |
|---|---|---:|---|---:|---:|---|
| ADC (`01_bsp/adc`) | Hardware abstraction | 是 | `mspm0g3507/signal_mspm0g3507_platform.c` | 是 | 是（software trigger） | READY |
| Button (`01_bsp/button`) | Hardware + debounce | 是 | `SignalMSPM0G3507_GPIO_ReadActive` | 是 | 是 | READY |
| Comparator (`01_bsp/comparator`) | Hardware abstraction | 是 | `SignalMSPM0G3507_Comparator_*` | 是 | 是 | READY |
| DAC (`01_bsp/dac`) | Hardware abstraction | 是 | `SignalMSPM0G3507_DAC_*` | 是 | 是 | READY |
| DMA (`01_bsp/dma`) | Hardware abstraction | 是 | `SignalMSPM0G3507_DMA_*` | 是 | 是；trigger/mode 按 Profile | READY |
| GPAMP (`01_bsp/gpamp`) | Hardware abstraction | 是 | 无；API 字段不足 | 是（明确 Gap） | 否 | API_GAP |
| GPIO (`01_bsp/gpio`) | Hardware abstraction | 是 | `SignalMSPM0G3507_GPIO_*` | 是 | 是 | READY |
| Latching Button Switch (`01_bsp/latching_button_switch`) | Hardware + debounce | 是 | `SignalMSPM0G3507_GPIO_ReadActive` | 是 | 是 | READY |
| Matrix Keypad 4x4 (`01_bsp/matrix_keypad_4x4`) | Hardware + scan | 是 | `SignalMSPM0G3507_Keypad*` | 是 | 是 | READY |
| OPA (`01_bsp/opa`) | Hardware abstraction | 是 | 无；API 字段不足 | 是（明确 Gap） | 否 | API_GAP |
| System Clock (`01_bsp/system_clock`) | Pure software | 否 | N/A | 是 | 是 | READY |
| Timer (`01_bsp/timer`) | Hardware abstraction | 是 | `SignalMSPM0G3507_Timer*` | 是 | 是 | READY |
| UART (`01_bsp/uart`) | Hardware abstraction | 是 | `SignalMSPM0G3507_UART_*` | 是 | 是 | READY |
| TFT ILI9341 (`01_bsp/tft_ili9341`) | Hardware display | 是 | `signal_mspm0g3507_tft_platform.c` | 是 | 是 | READY |
| VREF (`01_bsp/vref`) | Pure calibration helper | 否 | N/A；不直接配置 VREF hardware | 是 | 是 | READY |

### B. Acquisition（9）

| Module | 类型 | 依赖 Platform | 正式 Platform 实现 | README 说明 | 仅靠 README 可拼装 | 状态 |
|---|---|---:|---|---:|---:|---|
| ADC Basic (`02_acquisition/adc_basic`) | Hardware acquisition | 是 | BSP ADC + `SignalMSPM0G3507_ADC_*` | 是 | 是 | READY |
| ADC Continuous (`02_acquisition/adc_continuous`) | Software frame dispatcher | 否；callback 是业务 consumer | README 给最小 consumer | 是 | 是 | READY |
| ADC DMA (`02_acquisition/adc_dma`) | MSPM0 hardware acquisition | 内置 | 正式模块直接使用 generated macros/DriverLib | 是 | 是 | READY |
| Dual ADC Sync (`02_acquisition/adc_dual_sync`) | Pure data adapter | 否 | 双 ADC 硬件由 `signal_dual_adc_platform.c` | 是 | 是 | READY |
| ADC Ping-Pong DMA (`02_acquisition/adc_pingpong_dma`) | Pure buffer/state manager | 否 | 上游 DMA 由 ADC DMA/平台提供 | 是 | 是 | READY |
| ADC Ring Buffer (`02_acquisition/adc_ring_buffer`) | Pure buffer | 否 | N/A | 是 | 是 | READY |
| ADC Timer Trigger (`02_acquisition/adc_timer_trigger`) | Hardware orchestrator | 是 | BSP Timer/ADC + unified adapter | 是 | 是 | READY |
| Timer Capture (`02_acquisition/timer_capture`) | Software ticks→Hz | 间接 | `signal_mspm0g3507_capture_platform.c` 采 timestamp | 是 | 是 | READY |
| Trigger Capture (`02_acquisition/trigger_capture`) | Pure raw segment extraction | 否 | N/A | 是 | 是 | READY |

### C. Measurement（canonical `03_measurement/`）

| Module | 类型 | 依赖 Platform | 正式 Platform 实现 | README 说明 | 仅靠 README 可拼装 | 状态 |
|---|---|---:|---|---:|---:|---|
| ADC To Voltage (`03_measurement/adc_to_voltage`) | Pure software | 否 | N/A | 是 | 是 | READY |
| Mean (`03_measurement/mean`) | Pure software | 否 | N/A | 是 | 是 | READY |
| MinMax (`03_measurement/minmax`) | Pure software | 否 | N/A | 是 | 是 | READY |
| VPP (`03_measurement/vpp`) | Pure software | 否 | N/A | 是 | 是 | READY |
| RMS (`03_measurement/rms`) | Pure software | 否 | N/A | 是 | 是 | READY |
| AC RMS (`03_measurement/ac_rms`) | Pure software | 否 | N/A | 是 | 是 | READY |
| Statistics (`03_measurement/statistics`) | Pure software | 否 | N/A | 是 | 是 | READY |
| Zero Cross (`03_measurement/frequency_zero_cross`) | Pure software | 否 | N/A | 是 | 是 | READY |
| Phase (`03_measurement/phase`) | Pure software dispatcher | 否 | N/A | 是 | 是 | READY |

### D. DSP / Filtering（canonical `04_dsp/`）

| Module | 类型 | 依赖 Platform | 正式 Platform 实现 | README 说明 | 仅靠 README 可拼装 | 状态 |
|---|---|---:|---|---:|---:|---|
| Remove DC (`04_dsp/remove_dc`) | Pure software | 否 | N/A | 是 | 是 | READY |
| Window Dispatcher (`04_dsp/window`) | Pure software parent | 否 | N/A | 是 | 是 | READY |
| Hann (`04_dsp/window/hann`) | Pure software implementation | 否 | N/A | 是 | 是 | READY |
| Hamming (`04_dsp/window/hamming`) | Pure software implementation | 否 | N/A | 是 | 是 | READY |
| Blackman (`04_dsp/window/blackman`) | Pure software implementation | 否 | N/A | 是 | 是 | READY |
| Rectangular (`04_dsp/window/rectangular`) | Pure software implementation | 否 | N/A | 是 | 是 | READY |
| FIR (`04_dsp/fir`) | Pure software | 否 | N/A | 是 | 是 | READY |
| IIR Biquad (`04_dsp/iir_biquad`) | Pure software | 否 | N/A | 是 | 是 | READY |
| Moving Average (`04_dsp/moving_average`) | Pure software | 否 | N/A | 是 | 是 | READY |
| Median Filter (`04_dsp/median_filter`) | Pure software | 否 | N/A | 是 | 是 | READY |
| Hampel Filter (`04_dsp/hampel_filter`) | Pure software | 否 | N/A | 是 | 是 | READY |
| MAD (`04_dsp/mad`) | Pure software | 否 | N/A | 是 | 是 | READY |
| Correlation (`04_dsp/correlation`) | Pure software | 否 | N/A | 是 | 是 | READY |
| Autocorrelation (`04_dsp/autocorrelation`) | Pure software | 否 | N/A | 是 | 是 | READY |

### E. FFT / Spectrum（canonical `04_dsp/`）

| Module | 类型 | 依赖 Platform | 正式 Platform 实现 | README 说明 | 仅靠 README 可拼装 | 状态 |
|---|---|---:|---|---:|---:|---|
| FFT (`04_dsp/fft`) | Pure software/backend-selectable | 否 | N/A | 是 | 是 | READY |
| FFT Magnitude (`04_dsp/fft_magnitude`) | Pure software | 否 | N/A | 是 | 是 | READY |
| Peak Detect (`04_dsp/peak_detect`) | Pure software | 否 | N/A | 是 | 是 | READY |

### F. Harmonic / Quality（canonical `04_dsp/`）

| Module | 类型 | 依赖 Platform | 正式 Platform 实现 | README 说明 | 仅靠 README 可拼装 | 状态 |
|---|---|---:|---|---:|---:|---|
| Harmonic (`04_dsp/harmonic`) | Pure software | 否 | N/A | 是 | 是 | READY |
| THD (`04_dsp/thd`) | Pure software | 否 | N/A | 是 | 是 | READY |
| Spectral SNR (`04_dsp/snr`) | Pure software | 否 | N/A | 是 | 是 | READY |
| SFDR (`04_dsp/sfdr`) | Pure software | 否 | N/A | 是 | 是 | READY |
| Clipping Detect (`04_dsp/clipping_detect`) | Pure software | 否 | N/A | 是 | 是 | READY |

### G. Precision（canonical `05_precision/`）

| Module | 类型 | 依赖 Platform | 正式 Platform 实现 | README 说明 | 仅靠 README 可拼装 | 状态 |
|---|---|---:|---|---:|---:|---|
| ADC Gain/Offset Calibration (`05_precision/adc_gain_offset_calibration`) | Pure software | 否 | N/A | 是 | 是 | READY |
| Channel Delay Calibration (`05_precision/channel_delay_calibration`) | Pure software | 否 | N/A | 是 | 是 | READY |
| Zero Cross Interpolation (`05_precision/zero_cross_interpolation`) | Pure software | 否 | N/A | 是 | 是 | READY |
| Multi Cycle Average (`05_precision/multi_cycle_average`) | Pure software | 否 | N/A | 是 | 是 | READY |
| FFT Parabolic Interpolation (`05_precision/fft_parabolic_interpolation`) | Pure software | 否 | N/A | 是 | 是 | READY |
| Log Parabolic Interpolation (`05_precision/log_parabolic_interpolation`) | Pure software | 否 | N/A | 是 | 是 | READY |
| Window Gain Correction (`05_precision/window_gain_correction`) | Pure software | 否 | N/A | 是 | 是 | READY |
| Multi-bin Energy (`05_precision/multi_bin_energy`) | Pure software | 否 | N/A | 是 | 是 | READY |
| Lock-In (`05_precision/lock_in`) | Pure software | 否 | N/A | 是 | 是 | READY |
| Robust VPP (`05_precision/robust_peak_to_peak`) | Pure software | 否 | N/A | 是 | 是 | READY |
| Robust RMS (`05_precision/robust_rms`) | Pure software | 否 | N/A | 是 | 是 | READY |
| Sine Fit 3-Parameter (`05_precision/sine_fit_3param`) | Pure software | 否 | N/A | 是 | 是 | READY |
| Sine Fit 4-Parameter (`05_precision/sine_fit_4param`) | Pure software | 否 | N/A | 是 | 是 | READY |

### H. Generator（11）

| Module | 类型 | 依赖 Platform | 正式 Platform 实现 | README 说明 | 仅靠 README 可拼装 | 状态 |
|---|---|---:|---|---:|---:|---|
| DAC DC (`06_generator/dac_dc`) | Hardware output | 是 | BSP DAC + unified adapter | 是 | 是 | READY |
| DAC Wave Table (`06_generator/dac_wave_table`) | Pure code conversion | 否 | N/A | 是 | 是 | READY |
| DAC DMA (`06_generator/dac_dma`) | Hardware output wrapper | 是 | `signal_dac_dma_platform.c` | 是 | 是 | READY |
| DDS (`06_generator/dds`) | Pure wave indexing | 否 | 输出交给 DAC DMA | 是 | 是 | READY |
| Sine (`06_generator/sine`) | Pure table generator | 否 | N/A | 是 | 是 | READY |
| Square (`06_generator/square`) | Pure table generator | 否 | N/A | 是 | 是 | READY |
| Triangle (`06_generator/triangle`) | Pure table generator | 否 | N/A | 是 | 是 | READY |
| Sawtooth (`06_generator/sawtooth`) | Pure table generator | 否 | N/A | 是 | 是 | READY |
| Arbitrary Wave (`06_generator/arbitrary_wave`) | Pure resampler | 否 | N/A | 是 | 是 | READY |
| AM Modulation (`06_generator/am_modulation`) | Pure software | 否 | N/A | 是 | 是 | READY |
| Frequency Sweep (`06_generator/frequency_sweep`) | Pure sequence generator | 否 | N/A | 是 | 是 | READY |

### I. Analog Frontend（9）

| Module | 类型 | 依赖 Platform | 正式 Platform 实现 | README 说明 | 仅靠 README 可拼装 | 状态 |
|---|---|---:|---|---:|---:|---|
| Comparator Threshold (`07_signal_frontend/comparator_threshold`) | Config builder + hardware apply | 是 | BSP Comparator + unified adapter | 是 | 是 | READY |
| Comparator Zero Cross (`07_signal_frontend/comparator_zero_cross`) | Config builder + hardware apply | 是 | BSP Comparator + unified adapter | 是 | 是 | READY |
| GPAMP Buffer (`07_signal_frontend/gpamp_buffer`) | Hardware config builder | 是 | 无；继承 BSP GPAMP Gap | 是（明确 Gap） | 否 | API_GAP |
| GPAMP Gain (`07_signal_frontend/gpamp_gain`) | Hardware config builder | 是 | 无；继承 BSP GPAMP Gap | 是（明确 Gap） | 否 | API_GAP |
| OPA Buffer (`07_signal_frontend/opa_buffer`) | Hardware config builder | 是 | 无；继承 BSP OPA Gap | 是（明确 Gap） | 否 | API_GAP |
| OPA DAC Bias (`07_signal_frontend/opa_dac_bias`) | Pure voltage-budget helper | 否 | N/A | 是 | 是 | READY |
| OPA Inverting (`07_signal_frontend/opa_inverting`) | Hardware config builder | 是 | 无；继承 BSP OPA Gap | 是（明确 Gap） | 否 | API_GAP |
| OPA Non-inverting PGA (`07_signal_frontend/opa_noninverting_pga`) | Hardware config builder | 是 | 无；继承 BSP OPA Gap | 是（明确 Gap） | 否 | API_GAP |
| OPA To ADC (`07_signal_frontend/opa_to_adc`) | Pure range-budget helper | 否 | N/A | 是 | 是 | READY |

### J. Adapter（5）

| Module | 类型 | 依赖 Platform | 正式 Platform 实现 | README 说明 | 仅靠 README 可拼装 | 状态 |
|---|---|---:|---|---:|---:|---|
| Backend Adapter (`ALG/04_dsp/backend_adapter`) | Pure data/backend adapter | 否 | N/A | 是 | 是 | READY |
| Integration Glue (`08_applications/common/signal_integration.*`) | Pure pipeline glue | 否 | N/A | 是 | 是 | READY |
| Dual ADC Platform Adapter (`08_applications/common/signal_dual_adc_platform.*`) | MSPM0 platform | 内置 | 正式源码自身 | 是 | 是 | READY |
| DAC DMA Platform Adapter (`08_applications/common/signal_dac_dma_platform.*`) | MSPM0 platform | 内置 | 正式源码自身 | 是 | 是 | READY |
| MSPM0G3507 Platform Adapter (`08_applications/common/mspm0g3507`) | MSPM0 platform | 内置 | 正式目录自身 | 是 | 是；OPA/GPAMP 除外 | READY |

## 3. Callback 逐项闭环

| Callback | 为什么存在 | MSPM0G3507 正式实现 / 文件 | 用户自己写？ | 关键真实调用/说明 |
|---|---|---|---:|---|
| ADC `read/enable/disable` | 不把 ADC instance/MEM 写死进 BSP | `SignalMSPM0G3507_ADC_*` / `signal_mspm0g3507_platform.c` | 否 | software trigger、raw interrupt polling、`DL_ADC12_getMemResult` |
| DAC `write` | 电压换算与 DAC instance 解耦 | `SignalMSPM0G3507_DAC_Write` / 同上 | 否 | 12-bit 0..4095 → `DL_DAC12_output12` |
| GPIO `write/read/toggle` | UI 模块不绑定 GPIOA/B | `SignalMSPM0G3507_GPIO_*` / 同上 | 否 | `pin` 是 bit mask |
| Button `read_pressed` | 电气电平转逻辑按下 | `SignalMSPM0G3507_GPIO_ReadActive` / 同上 | 否 | active-low/high 由 context 指定 |
| Latching `read_on` | 电气电平转逻辑 ON | 同上 | 否 | 与普通按钮共用唯一输入 adapter |
| Keypad row/column/delay | 4×4 扫描不写死八个 pin | `SignalMSPM0G3507_Keypad*` / 同上 | 否 | active-low rows、pull-up columns |
| UART `write/read` | 字节流与 UART instance 解耦 | `SignalMSPM0G3507_UART_*` / 同上 | 否 | blocking TX、bounded non-blocking RX |
| Timer set/start/stop/read | Hz/tick 与 TimerG 解耦 | `SignalMSPM0G3507_Timer*` / 同上 | 否 | period count → LOAD=count-1 |
| DMA configure/start/stop | transfer 与 channel/trigger 解耦 | `SignalMSPM0G3507_DMA_*` / 同上 | 否 | trigger/mode 仍由 SysConfig 确定 |
| Comparator apply | V 配置与 DAC8/迟滞枚举解耦 | `SignalMSPM0G3507_Comparator_*` / 同上 | 否 | DAC8 量化，迟滞量化为 0/10/20/30 mV |
| Timer Capture ISR/state/copy | 把硬件 capture IRQ、down-count 转换与算法 period 计算分离 | `SignalMSPM0G3507_Capture_*` / `signal_mspm0g3507_capture_platform.c/.h` | 否 | ISR 保存正向 tick；Application 再交给 `SignalTimerCapture_MeanPeriod` |
| ADC Timer Trigger arm/disarm | 保证 ADC 先 arm、Timer 后 start | 直接传 `SignalMSPM0G3507_ADC_Enable/Disable` | 否 | Profile 必须使用 Event trigger |
| ADC Continuous frame callback | 通知业务层消费已经完成的 frame | Application 的处理函数 | 是，但只写业务处理 | README 给最小真实签名；不是硬件 glue |
| DAC DMA start/stop | 状态机与 Timer/Event/DMA/DAC 分离 | `signal_dac_dma_platform.c/.h` | 否 | PROFILE_03 |
| TFT SPI/GPIO/delay | 屏幕核心不绑定 SPI/pin | `signal_mspm0g3507_tft_platform.c/.h` | 否 | 已从 TFT example private glue 提升 |
| OPA apply | 期望抽象模拟拓扑 | 无 | 当前也不应写 | API 无法表达真实 MUX/离散 gain，API_GAP |
| GPAMP apply | 期望抽象 GPAMP 配置 | 无 | 当前也不应写 | API 无法表达真实 DriverLib config，API_GAP |

## 4. Application 隐藏 Glue 扫描

| 发现 | 原位置 | 处理 |
|---|---|---|
| Comparator capture ISR、timestamp/timeout 状态 | `08_applications/frequency_meter/main.c` | 提升到 `signal_mspm0g3507_capture_platform.c`；Frequency Meter A 改为正式 API |
| TFT SPI write、DC/BLK GPIO、delay | `09_examples/tft_ili9341_lp_mspm0g3507/main.c` | 提升到 `signal_mspm0g3507_tft_platform.c`；示例改为 Bind |
| DAC DMA Timer/Event/DMA/DAC glue | 多个 generator application 已共用 | 保留唯一 `signal_dac_dma_platform.c`，没有再造第二份 |
| Dual ADC/DMA glue | Phase application 已共用 | 保留唯一 `signal_dual_adc_platform.c` |
| `static adc_read/dac_write/gpio_set/platform_start` 其他重复 | 全部 `08_applications/*/main.c` | 未发现另一套通用实现 |

## 5. 数据单位闭环

| 上游输出 | 下游输入 | 单位/格式 | 是否直接接 | 说明 |
|---|---|---|---:|---|
| ADC Basic / ADC DMA | ADC To Voltage | `uint16_t` raw code | 是 | 同步 bits 与 VREF |
| ADC To Voltage | Measurement/DSP | `float` V | 是 | scale/offset 后仍为 V |
| Backend Adapter | Q15/Q30 algorithm backend | Q15/Q30 fixed-point | 按 adapter 接 | 不能把 raw ADC 直接当 Q15 电压 |
| Timer Capture platform | Timer Capture math | `uint32_t` tick | 是 | `timer_hz` 用 Hz，`counter_modulus` 用 tick |
| Timer Capture math | Application | `float frequency_hz` | 是 | Hz |
| FFT | FFT Magnitude | `signal_complex_f32_t[N]` | 是 | FFT complex 未归一化，不是 V |
| Magnitude | Peak/Interpolation/Harmonic | `float magnitude[0..N/2]` | 是 | bin 无量纲索引，频率换算用 Fs/N |
| Phase | Application | degrees（公开 result 字段） | 是 | 不与 radian API 混用 |
| DAC Wave/DDS | DAC DMA | `uint16_t` 12-bit code | 是 | 0..4095，不是 V |
| DAC DC | BSP DAC | target V → raw code | 是 | reference V 必须和 SysConfig/实测一致 |

## 6. SysConfig 闭环

| 功能 | 必需 Peripheral/Event | 正式参考 |
|---|---|---|
| ADC Basic | ADC0 MEM0, software trigger, PA25, VDDA | `PROFILE_07_BASIC_IO` |
| DAC DC | DAC0, 12-bit, amplifier/output enable, PA15 | `PROFILE_07_BASIC_IO` |
| UART | UART0 PA10/PA11, 115200, FIFO, loopback off | `PROFILE_07_BASIC_IO` |
| ADC DMA | TIMG0 publisher 1 → ADC0 → DMA0 | `PROFILE_01_ADC_CAPTURE` |
| DAC DMA | TIMG6 publisher 3 → DAC0 FIFO/DMA1 | `PROFILE_03_DAC_GENERATOR` |
| Comparator Capture | COMP0 publisher 4 → TIMG6 subscriber 4, IRQ | `PROFILE_05_FREQUENCY` |
| Dual ADC | Timer event → ADC0/ADC1 → DMA0/DMA1 | `PROFILE_02_DUAL_ADC` |
| TFT | SPI1 + hardware CS + DC/BLK GPIO | `09_examples/tft_ili9341_lp_mspm0g3507/tft_ili9341.syscfg` |

`PROFILE_01..06` 的 UART 均已显式设置 `enableInternalLoopback=false`，防止“编译通过但外部串口不出数据”。

## 7. 真实 Build/Link 证据

命令：`tools/build_platform_closure.ps1`。每个目标都执行 SysConfig → 全部 translation units compile (`-Wall -Werror`) → final application link。

| Target | SysConfig | Compile | Link | Flash B | SRAM B（含 512 B stack） | Board |
|---|---|---|---|---:|---:|---|
| DAC fixed code direct | PASS | PASS | PASS | 1480 | 514 | NOT_RUN |
| ADC single result direct | PASS | PASS | PASS | 1232 | 514 | NOT_RUN |
| UART blocking TX direct | PASS | PASS | PASS | 1312 | 512 | NOT_RUN |
| GPIO set direct | PASS | PASS | PASS | 1064 | 512 | NOT_RUN |
| ADC Timer Trigger minimum | PASS | PASS | PASS | 2936 | 520 | NOT_RUN |
| ADC Continuous minimum | PASS | PASS | PASS | 1400 | 524 | NOT_RUN |
| ADC DMA minimum | PASS | PASS | PASS | 2664 | 668 | NOT_RUN |
| DAC DMA minimum | PASS | PASS | PASS | 2224 | 683 | NOT_RUN |
| Comparator + Timer Capture minimum | PASS | PASS | PASS | 3576 | 776 | NOT_RUN |
| TFT ILI9341 minimum | PASS | PASS | PASS | 21064 | 637 | NOT_RUN |

回归结果：7 个 Integration Profile 全部 SysConfig/compile/link PASS；Round 1 的 8 个 Application 全部 full link PASS，其中迁移后的 Frequency Meter A 为 Flash 2368 B、SRAM 780 B。

机器可读证据：`10_tests/platform_closure/build/platform_closure_build_results.json`、`10_tests/peripheral_profiles/build/profile_build_results.json`、`10_tests/integration/round1_build_closure/round1_build_results.json`。
