# Module README Assembly Guide Audit

> **口径更新：** 本审计最初按统一 18 项模板检查。当前总入口已改为 [CONTEST_IMPLEMENTATION_GUIDE.md](CONTEST_IMPLEMENTATION_GUIDE.md)：简单硬件 README 必须先提示 Direct DriverLib，复杂硬件继续保留完整装配项，纯算法 README 从输入 buffer 开始且不再复制 Pin/SysConfig/Platform 模板章节。

审计日期：2026-08-11；2026-08-13 canonical 合并后口径更新。当前模块数量与路径以 92 条 Canonical Registry 为准，硬件、算法和 Adapter 均位于同一 `MSPM0_Signal_Contest/` 根。

## 18 项判定规则

1. 30 秒路线；
2. 加入哪些文件；
3. COPY/LINK/GENERATED/REFERENCE ONLY；
4. CCS/projectspec 实际加入方式；
5. SysConfig 需求或明确不需要；
6. Pin 配置或明确不占 Pin；
7. Platform 绑定或明确不需要；
8. 题目参数对应关系；
9. 参数修改位置与同步项；
10. main 顶部 include/config/buffer/result；
11. SYSCFG_DL_init 前后与初始化顺序；
12. runtime/while 调用位置，或明确单次调用后不重复；
13. 输出变量、类型、单位和含义；
14. 2～3 条变量级下游连接；
15. Clean/Build 与常见装配错误；
16. 最小验证；
17. 从母版到结果的完整例子；
18. 示例与当前 .h/已验证 Application/Platform 一致。

复杂硬件模块只有必要装配项完整才标 ASSEMBLY_READY。纯算法以输入、文件、参数、workspace/result、API、输出和最小验证为准，不要求建立独立 SysConfig/Pin/Platform 章节。简单 MSPM0G3507 动作采用 SysConfig + DriverLib 时，旧 callback 源必须标为 REFERENCE ONLY。

## 本轮结论

| 状态 | 数量 | 含义 |
|---|---:|---|
| ASSEMBLY_READY | 30 | 优先入口覆盖 18 项，可按 README 从母版拼到结果 |
| ASSEMBLY_BLOCKED | 2 | OPA/GPAMP 软件 helper 可用，真实硬件缺 Profile/Platform/Pin 证据 |
| DRAFT | 62 | 有百科/API README，但尚未完成 18 项拼装验收 |
| 合计 | 94 | 与 Module Catalog 正式口径一致 |

## 第一批优先模块

| 类别 | 模块 | 正式路径 | 入口形态 | 结果 | 状态 |
|---|---|---|---|---:|---|
| A | ADC | 01_bsp/adc/ | 单点 Direct；N 点转 ADC DMA | 18/18 | ASSEMBLY_READY |
| A | Comparator | 01_bsp/comparator/ | 固定配置 Direct；测频转 Capture | 18/18 | ASSEMBLY_READY |
| A | DAC | 01_bsp/dac/ | 固定 code Direct；连续波转 DAC DMA | 18/18 | ASSEMBLY_READY |
| A | DMA | 01_bsp/dma/ | Support only；从 ADC/DAC DMA 进入 | 18/18 | ASSEMBLY_READY |
| A | GPAMP | 01_bsp/gpamp/ | 软件 Validate；硬件 API_GAP | 15/18，缺真实 Pin/Platform/硬件完整例 | ASSEMBLY_BLOCKED |
| A | GPIO | 01_bsp/gpio/ | Direct DriverLib | 18/18 | ASSEMBLY_READY |
| A | OPA | 01_bsp/opa/ | 软件 Gain helper；硬件 API_GAP | 15/18，缺真实 Pin/Platform/硬件完整例 | ASSEMBLY_BLOCKED |
| A | Timer | 01_bsp/timer/ | Direct；复杂链用功能模块 | 18/18 | ASSEMBLY_READY |
| A | UART | 01_bsp/uart/ | Direct blocking DriverLib | 18/18 | ASSEMBLY_READY |
| A | VREF | 01_bsp/vref/ | calibration helper；硬件另配 SysConfig | 18/18 | ASSEMBLY_READY |
| B | ADC Basic | 02_acquisition/adc_basic/ | Direct single conversion | 18/18 | ASSEMBLY_READY |
| B | ADC DMA | 02_acquisition/adc_dma/ | 正式复杂模块 | 18/18 | ASSEMBLY_READY |
| B | ADC FIFO DMA | 02_acquisition/adc_fifo_dma/ | FIFO + 32-bit DMA 满吞吐率单帧 | 18/18 | ASSEMBLY_READY |
| B | Dual ADC Sync | 02_acquisition/adc_dual_sync/ | dual platform + 可选 deinterleave | 18/18 | ASSEMBLY_READY |
| B | ADC Timer Trigger | 02_acquisition/adc_timer_trigger/ | 兼容入口；新工程默认 ADC DMA | 18/18 | ASSEMBLY_READY |
| B | Timer Capture | 02_acquisition/timer_capture/ | capture platform + 计算模块 | 18/18 | ASSEMBLY_READY |
| C | ADC To Voltage | ALG/03_measurement/adc_to_voltage/ | 纯算法 | 18/18 | ASSEMBLY_READY |
| C | VPP | ALG/03_measurement/vpp/ | 纯算法 | 18/18 | ASSEMBLY_READY |
| C | RMS | ALG/03_measurement/rms/ | 纯算法 | 18/18 | ASSEMBLY_READY |
| C | AC RMS | ALG/03_measurement/ac_rms/ | 纯算法 | 18/18 | ASSEMBLY_READY |
| C | Phase | ALG/03_measurement/phase/ | 纯算法；同步由上游保证 | 18/18 | ASSEMBLY_READY |
| D | Remove DC | ALG/04_dsp/remove_dc/ | 纯算法 | 18/18 | ASSEMBLY_READY |
| D | Window Dispatcher | ALG/04_dsp/window/ | 统一入口 + 四个 linked 子实现 | 18/18 | ASSEMBLY_READY |
| E | FFT | ALG/04_dsp/fft/ | 纯算法；Backend 由 verified projectspec 选择 | 18/18 | ASSEMBLY_READY |
| E | FFT Magnitude | ALG/04_dsp/fft_magnitude/ | 纯算法 | 18/18 | ASSEMBLY_READY |
| E | Peak Detect | ALG/04_dsp/peak_detect/ | 纯算法 | 18/18 | ASSEMBLY_READY |
| F | Harmonic | ALG/04_dsp/harmonic/ | 纯算法 | 18/18 | ASSEMBLY_READY |
| F | THD | ALG/04_dsp/thd/ | 纯算法，输入是 Harmonic result | 18/18 | ASSEMBLY_READY |
| G | FFT Parabolic Interpolation | ALG/05_precision/fft_parabolic_interpolation/ | 纯算法 | 18/18 | ASSEMBLY_READY |
| H | DAC DC | 06_generator/dac_dc/ | Direct DriverLib | 18/18 | ASSEMBLY_READY |
| H | DAC DMA | 06_generator/dac_dma/ | module + DAC platform | 18/18 | ASSEMBLY_READY |
| H | DDS | 06_generator/dds/ | 算法层；真实输出接 DAC DMA/platform | 18/18 | ASSEMBLY_READY |

## 其余 62 个正式模块

下列模块保留现有百科/API 说明，但本轮未达到 18/18，不能写 ASSEMBLY_READY。共同缺口主要是母版视角的 30 秒路线、projectspec linked-source 动作、变量级 main/下游和完整闭环例子。

| 类别 | 模块 | 正式路径 | 本轮结果 | 状态 |
|---|---|---|---|---|
| A | Button | 01_bsp/button/ | 未达 18/18 | DRAFT |
| A | Latching Button Switch | 01_bsp/latching_button_switch/ | 未达 18/18 | DRAFT |
| A | Matrix Keypad 4×4 | 01_bsp/matrix_keypad_4x4/ | 未达 18/18 | DRAFT |
| A | System Clock | 01_bsp/system_clock/ | 未达 18/18 | DRAFT |
| A | TFT ILI9341 | 01_bsp/tft_ili9341/ | 未达 18/18 | DRAFT |
| B | ADC Continuous | 02_acquisition/adc_continuous/ | 未达 18/18 | DRAFT |
| B | ADC Ping-Pong DMA | 02_acquisition/adc_pingpong_dma/ | 未达 18/18 | DRAFT |
| B | ADC Ring Buffer | 02_acquisition/adc_ring_buffer/ | 未达 18/18 | DRAFT |
| B | Trigger Capture | 02_acquisition/trigger_capture/ | 未达 18/18 | DRAFT |
| C | Mean | ALG/03_measurement/mean/ | 未达 18/18 | DRAFT |
| C | MinMax | ALG/03_measurement/minmax/ | 未达 18/18 | DRAFT |
| C | Statistics | ALG/03_measurement/statistics/ | 未达 18/18 | DRAFT |
| C | Zero Cross | ALG/03_measurement/frequency_zero_cross/ | 未达 18/18 | DRAFT |
| D | Hann Window | ALG/04_dsp/window/hann/ | 子实现；统一 Window 为入口 | DRAFT |
| D | Hamming Window | ALG/04_dsp/window/hamming/ | 子实现；统一 Window 为入口 | DRAFT |
| D | Blackman Window | ALG/04_dsp/window/blackman/ | 子实现；统一 Window 为入口 | DRAFT |
| D | Rectangular Window | ALG/04_dsp/window/rectangular/ | 子实现；统一 Window 为入口 | DRAFT |
| D | FIR | ALG/04_dsp/fir/ | 未达 18/18 | DRAFT |
| D | IIR Biquad | ALG/04_dsp/iir_biquad/ | 未达 18/18 | DRAFT |
| D | Moving Average | ALG/04_dsp/moving_average/ | 未达 18/18 | DRAFT |
| D | Median Filter | ALG/04_dsp/median_filter/ | 未达 18/18 | DRAFT |
| D | Hampel Filter | ALG/04_dsp/hampel_filter/ | 未达 18/18 | DRAFT |
| D | MAD | ALG/04_dsp/mad/ | 未达 18/18 | DRAFT |
| D | Correlation | ALG/04_dsp/correlation/ | 未达 18/18 | DRAFT |
| D | Autocorrelation | ALG/04_dsp/autocorrelation/ | 未达 18/18 | DRAFT |
| F | Spectral SNR | ALG/04_dsp/snr/ | 未达 18/18 | DRAFT |
| F | SFDR | ALG/04_dsp/sfdr/ | 未达 18/18 | DRAFT |
| F | Clipping Detect | ALG/04_dsp/clipping_detect/ | 未达 18/18 | DRAFT |
| G | ADC Gain/Offset Calibration | ALG/05_precision/adc_gain_offset_calibration/ | 未达 18/18 | DRAFT |
| G | Channel Delay Calibration | ALG/05_precision/channel_delay_calibration/ | 未达 18/18 | DRAFT |
| G | Zero Cross Interpolation | ALG/05_precision/zero_cross_interpolation/ | 未达 18/18 | DRAFT |
| G | Multi Cycle Average | ALG/05_precision/multi_cycle_average/ | 未达 18/18 | DRAFT |
| G | Log Parabolic Interpolation | ALG/05_precision/log_parabolic_interpolation/ | 未达 18/18 | DRAFT |
| G | Window Gain Correction | ALG/05_precision/window_gain_correction/ | 未达 18/18 | DRAFT |
| G | Multi-bin Energy | ALG/05_precision/multi_bin_energy/ | 未达 18/18 | DRAFT |
| G | Lock-In | ALG/05_precision/lock_in/ | 未达 18/18 | DRAFT |
| G | Robust VPP | ALG/05_precision/robust_peak_to_peak/ | 未达 18/18 | DRAFT |
| G | Robust RMS | ALG/05_precision/robust_rms/ | 未达 18/18 | DRAFT |
| G | Sine Fit 3-Parameter | ALG/05_precision/sine_fit_3param/ | 未达 18/18 | DRAFT |
| G | Sine Fit 4-Parameter | ALG/05_precision/sine_fit_4param/ | 未达 18/18 | DRAFT |
| H | DAC Wave Table | 06_generator/dac_wave_table/ | 未达 18/18 | DRAFT |
| H | Sine | 06_generator/sine/ | 未达 18/18 | DRAFT |
| H | Square | 06_generator/square/ | 未达 18/18 | DRAFT |
| H | Triangle | 06_generator/triangle/ | 未达 18/18 | DRAFT |
| H | Sawtooth | 06_generator/sawtooth/ | 未达 18/18 | DRAFT |
| H | Arbitrary Wave | 06_generator/arbitrary_wave/ | 未达 18/18 | DRAFT |
| H | AM Modulation | 06_generator/am_modulation/ | 未达 18/18 | DRAFT |
| H | Frequency Sweep | 06_generator/frequency_sweep/ | 未达 18/18 | DRAFT |
| I | Comparator Threshold | 07_signal_frontend/comparator_threshold/ | 未达 18/18 | DRAFT |
| I | Comparator Zero Cross | 07_signal_frontend/comparator_zero_cross/ | 未达 18/18 | DRAFT |
| I | GPAMP Buffer | 07_signal_frontend/gpamp_buffer/ | 继承 GPAMP hardware API_GAP | DRAFT |
| I | GPAMP Gain | 07_signal_frontend/gpamp_gain/ | 继承 GPAMP hardware API_GAP | DRAFT |
| I | OPA Buffer | 07_signal_frontend/opa_buffer/ | 继承 OPA hardware API_GAP | DRAFT |
| I | OPA DAC Bias | 07_signal_frontend/opa_dac_bias/ | 未达 18/18 | DRAFT |
| I | OPA Inverting | 07_signal_frontend/opa_inverting/ | 未达 18/18 | DRAFT |
| I | OPA Non-inverting PGA | 07_signal_frontend/opa_noninverting_pga/ | 未达 18/18 | DRAFT |
| I | OPA To ADC | 07_signal_frontend/opa_to_adc/ | 未达 18/18 | DRAFT |
| J | Backend Adapter | ALG/04_dsp/backend_adapter/ | 未达 18/18 | DRAFT |
| J | Integration Glue | 08_applications/common/signal_integration.* | 未达 18/18 | DRAFT |
| J | Dual ADC Platform Adapter | 08_applications/common/signal_dual_adc_platform.* | 未达 18/18 | DRAFT |
| J | DAC DMA Platform Adapter | 08_applications/common/signal_dac_dma_platform.* | 未达 18/18 | DRAFT |
| J | MSPM0G3507 Platform Adapter | 08_applications/common/mspm0g3507/ | 专用/兼容实现尚未形成单一 18 项 README | DRAFT |

## API 真值与验证边界

- 算法示例以当前 canonical 目录的 public `.h` 为准，不使用迁移前旧算法 API。
- ADC DMA、ADC FIFO DMA、Dual ADC、Timer Capture、DAC DMA、DDS 的调用顺序来自现有隔离 COPY TEST 或 full-link Application/Platform Closure example。
- 简单 GPIO/UART/ADC/DAC/Timer/Comparator 调用来自当前 SDK DriverLib 与 P01/P05/P07 生成宏。
- 本轮新增 ADC FIFO DMA 的正式 `.c/.h`、P08 SysConfig 和 README；没有修改算法 Backend。
- README/编译通过不自动升级为 BOARD_VERIFIED；物理 Pin、电压、频率、相位仍需按硬件验证边界执行。

## 本轮自动验证结果

| 检查 | 结果 |
|---|---|
| Documentation/API consistency | PASS：99 README、103 header，0 issue |
| Formal module integrity | 当前由 Canonical Path Checker、Module Card Checker 与 API Checker统一覆盖 |
| Projectspec path policy | PASS：17/17 |
| Platform Closure SysConfig/compile/full link | PASS：10/10 |
| Round 1 Application SysConfig/compile/full link | PASS：8/8 |

Platform Closure 覆盖 ADC Basic、ADC DMA、ADC Timer Trigger、DAC DC、DAC DMA、GPIO、UART、Timer Capture 等 README 直接引用的当前示例；Round 1 覆盖 Meter、三种 Frequency、Spectrum、THD、Phase 与 DDS 组合链。均只表示构建/链接闭环，不冒充本轮新增实板证据。
