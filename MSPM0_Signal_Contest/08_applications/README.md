# Applications：我应该参考哪个完整组合？

> **2026-08-13 比赛使用策略：** 本目录现有完整 Application 用于查看组合与当前 build baseline。新赛题默认复制 `signal_contest_template/`，再按模块 README 把所列 `.c/.h/.inc` 冻结复制到 `modules/`。新母版不需要 `MSPM0_SIGNAL_LIBRARY_ROOT`，也不以 Linked Source / Platform Adapter 作为主路线。

> **CMSIS-DSP 默认依赖：** 所有正式 Application projectspec 和共享 Profile 已按 SDK 官方方式加入 CMSIS-DSP。普通 RMS/Vpp/FFT/FIR/IIR 不再需要额外配置 CMSIS；直接查 `00_docs/CMSIS_DSP_CONTEST_COOKBOOK.md`。

本目录放的是已经拼好的系统参考工程和比赛模板，不是所有功能的默认入口。先用 [比赛功能实现与拼装指南](../00_docs/CONTEST_IMPLEMENTATION_GUIDE.md) 判断每个功能属于 Direct DriverLib、复杂模块、算法或外部驱动；需要参考完整组合时再从下表选择 Application。

> 状态说明：表中的 `BUILD_VERIFIED` 只表示当前版本已完成 SysConfig、编译和完整链接；除非工程 README 明确写有板测证据，否则不能当作已经实板验证。

## 30 秒快速选择

| 我想做什么 | 推荐先看哪个 Application | 它已经包含什么 | 适合拿来做什么 |
|---|---|---|---|
| 测 DC、Min、Max、Vpp、RMS、AC RMS | **signal_meter【简单参考】** | 单通道 ADC DMA、Raw→Voltage、基础测量、过零测频 | 最简单的单通道测量起点 |
| 测频率，但还不确定方法 | **frequency_meter【常用参考】** | Capture、过零插值、FFT 插值三种后端 | 比较 A/B/C 后保留最合适的一条链 |
| 用比较器和 Timer 硬件测频 | **frequency_meter A** | COMP0、Event、Timer Capture、周期平均 | 边沿干净、只关心频率、希望少占 CPU/RAM |
| 用 ADC 波形做时域测频 | **frequency_meter B** | ADC DMA、Remove DC、Zero Cross、线性插值、多周期平均 | 周期波形清楚且需要从采样波形得到频率 |
| 用 FFT 测频 | **frequency_meter C** | ADC DMA、Window、FFT、Magnitude、Peak、抛物线插值 | 多频、噪声或同时需要频谱的场景 |
| 做完整频谱、找主峰和多个峰 | **spectrum_analyzer【常用参考】** | Remove DC、Hann、FFT、幅值和窗增益修正、Peak | 频谱显示、主频和主要谱峰提取 |
| 测 H1～H5 和 THD | **harmonic_thd_analyzer【常用参考】** | 频谱链、基波插值、多 bin 谐波能量、THD | 正弦质量、谐波失真分析 |
| 测双通道相位 | **dual_channel_phase_meter【常用参考】** | 双 ADC、两路换算/去 DC、FFT Phase、Correlation Phase | 比较两路同频信号的 B−A 相位 |
| 产生软件可调正弦 | **dds_generator【简单参考】** | Sine Wave Table、DDS、DAC DMA、DAC 平台适配 | 改频率、幅值、偏置和初相位后持续输出 |
| 测 DUT 的增益/相位随频率变化 | **sweep_analyzer【综合参考】** | DDS/DAC 激励、ADC 采集、Lock-in 幅相、扫频点结果 | 放大器、滤波器等外部模拟 DUT 的扫频准备 |
| 捕获一个周期并从 DAC 重放 | **waveform_capture_replay【综合参考】** | ADC DMA、Ring Buffer、Trigger、周期段、重采样、DAC DMA | 周期波形捕获、归一化形状重放 |
| 在一个工程里按 Profile 选择多种分析功能 | **signal_analyzer【综合参考】** | Basic/Frequency/Spectrum/THD/Phase 五种编译期 Profile | 从完整分析器删减成功能专用工程 |
| 已拆好功能和实现层级，要从零组合 | **signal_contest_template【混合实现模板】** | Direct/复杂模块/算法/外部驱动分区、集中配置、Pipeline 空间 | 比赛拿到题后复制并按各路线接入 |
| 只想学习硬件/算法回调骨架 | **peripheral_system_template** | Acquire/Algorithm/Output 三个 Hook | 自己设计一条很薄的应用层调用链 |

## 最推荐的四个起点

- **signal_meter【简单参考】**：第一次看完整 `ADC DMA → 算法 → 结果`。
- **spectrum_analyzer【常用参考】**：第一次拼 FFT 全链。
- **signal_analyzer【综合参考】**：已有多个功能，适合删减和比较 Profile。
- **signal_contest_template【混合实现模板】**：已经拆好功能并判断实现层级后，用它开始自己的比赛工程。

## Frequency Meter A/B/C 是什么关系？

三者最终都写出统一的 `frequency_hz`，但输入和代价不同。

| 后端 | 实际方法 | 输入 | 什么时候选 | 主要限制 | Profile / projectspec |
|---|---|---|---|---|---|
| A | Comparator → Timer Capture → Mean Period | 经过比较器后的干净边沿 | 方波或可稳定整形成边沿；只测频率 | 比较器阈值、噪声和捕获溢出必须正确 | P05；`frequency_meter_a_round1_*` |
| B | ADC DMA → Zero Cross → Linear Interpolation → Multi Cycle Average | 单通道电压波形 | 近似周期波，想保留时域波形并获得精细频率 | 噪声会制造假过零；记录中要有多个周期 | P01；`frequency_meter_b_round1_*` |
| C | ADC DMA → Hann → FFT → Peak → Parabolic | 单通道电压波形 | 多频或噪声环境；后面还要频谱 | RAM/CPU 更高；频率范围、Fs、N 必须一致 | P01；`frequency_meter_c_q31_*` |

第一次拿不准时：边沿已经很干净先看 A；一般正弦单频先看 B；需要频谱或存在多个频率分量时看 C。

## Application 卡片

### Signal Meter【简单参考】

- **它是干什么的**：一次采样得到 DC、Min、Max、Vpp、总 RMS、AC RMS 和频率。
- **适合什么时候选**：题目首先要求基础幅值/电压测量；想学习最短单通道完整链。
- **不适合什么时候选**：主要任务是频谱、THD、双通道相位或连续无缝流处理。
- **主要模块**：ADC DMA、Integration Glue、ADC To Voltage、Mean/MinMax/VPP/RMS/AC RMS、Zero Cross、Interpolation、Multi Cycle Average。
- **输入**：PA25 / ADC0.2 的单通道模拟电压。
- **结果**：`signal_meter_result_t` 中已启用的测量字段。
- **ADC / FFT**：单帧 ADC DMA；不使用 FFT。
- **默认 N**：1024；默认 Fs=100 kHz。
- **硬件资源**：P01，ADC0 + TIMG0 + DMA0 + Event。
- **README**：[signal_meter/README.md](signal_meter/README.md)
- **projectspec**：`signal_meter/ticlang/signal_meter_round1_LP_MSPM0G3507_nortos_ticlang.projectspec`
- **基于它修改**：最适合关闭不需要的测量项，或追加 UART/TFT 显示；要频谱则改从 Spectrum Analyzer 起步更清楚。

### Frequency Meter【常用参考】

- **它是干什么的**：用三种不同后端得到统一频率结果。
- **适合什么时候选**：题目核心指标是频率，需要比较硬件捕获、过零和 FFT 方法。
- **不适合什么时候选**：只测幅值，或需要完整谐波/THD 输出。
- **主要模块**：A=Comparator/Timer Capture；B=ADC DMA/Zero Cross/Interpolation/Multi Cycle；C=ADC DMA/Window/FFT/Magnitude/Peak/Parabolic。
- **输入**：A 为比较器边沿；B/C 为 PA25 / ADC0.2 模拟波形。
- **结果**：统一 `frequency_hz` 和对应后端状态。
- **ADC / FFT**：A 无 ADC/FFT；B 用 ADC 无 FFT；C 用 ADC 和 Q31 FFT。
- **默认 N**：B/C 为 1024；默认 Fs=100 kHz。
- **硬件资源**：A=P05；B/C=P01。
- **README**：[frequency_meter/README.md](frequency_meter/README.md)
- **projectspec**：`frequency_meter/ticlang/` 下 A、B、C 三个 projectspec。
- **基于它修改**：先删掉不用的两种后端，再集中修改范围、滞回、N/Fs 或 Capture clock。

### Spectrum Analyzer【常用参考】

- **它是干什么的**：把 ADC 波形变成校正后的单边幅值谱，输出主频、主峰和多个主要峰。
- **适合什么时候选**：要看频谱、找多频成分或建立 FFT 测量链。
- **不适合什么时候选**：只需要一个干净边沿的频率，或 SRAM 极紧。
- **主要模块**：ADC DMA、RawToVoltage、RemoveDC、Hann、FFT、Magnitude、Window Gain Correction、Peak、Parabolic。
- **输入 / 结果**：单通道模拟电压；输出主峰频率/幅值、fractional bin 和主要峰数组。
- **ADC / FFT**：单帧 ADC DMA；默认 Q31 FFT backend。
- **默认 N**：1024；默认 Fs=100 kHz；搜索 100 Hz～40 kHz。
- **硬件资源**：P01；当前 Q31 N=1024 链接基线 SRAM 17,045 B。
- **README**：[spectrum_analyzer/README.md](spectrum_analyzer/README.md)
- **projectspec**：`spectrum_analyzer/ticlang/spectrum_analyzer_q31_LP_MSPM0G3507_nortos_ticlang.projectspec`
- **基于它修改**：适合增加峰值显示、频带能量、SNR/SFDR；不要直接把 complex FFT 当电压幅值。

### Harmonic / THD Analyzer【常用参考】

- **它是干什么的**：定位基波与 H2～H5，并计算 THD%。
- **适合什么时候选**：题目明确要求谐波、失真、基波幅值或 THD。
- **不适合什么时候选**：非周期瞬态波形，或采样带宽无法容纳最高谐波。
- **主要模块**：完整频谱链、Peak/Parabolic、Harmonic 多 bin 能量、THD。
- **输入 / 结果**：单通道模拟电压；输出基波频率/幅值、H1～H5、THD%。
- **ADC / FFT**：单帧 ADC DMA；默认 Q31 FFT backend。
- **默认 N**：1024；Fs=100 kHz；基波搜索上限 9 kHz，保证 H5 低于 Nyquist。
- **硬件资源**：P01。
- **README**：[harmonic_thd_analyzer/README.md](harmonic_thd_analyzer/README.md)
- **projectspec**：`harmonic_thd_analyzer/ticlang/harmonic_thd_analyzer_q31_LP_MSPM0G3507_nortos_ticlang.projectspec`
- **基于它修改**：可删掉不需要的谐波阶数或增加显示；改 Fs/范围时必须重新检查最高谐波与 Nyquist。

### Dual Channel Phase Meter【常用参考】

- **它是干什么的**：用 FFT bin 和互相关两条链测 B−A 相位。
- **适合什么时候选**：两路同频波形，需要比较相位或延迟。
- **不适合什么时候选**：只有一路 ADC；两路频率不同；尚不知道输入频率/周期。
- **主要模块**：Dual ADC Platform、两次 RawToVoltage、两路 RemoveDC、FFT Phase、Correlation、Correlation Phase。
- **输入 / 结果**：PA25/ADC0.2 与 PA17/ADC1.2；输出 FFT phase、correlation phase、lag 和相关系数。
- **ADC / FFT**：双 ADC 双 DMA；默认 Q31 FFT backend。
- **默认 N**：512；Fs=100 kHz；已知频率默认 1 kHz。
- **硬件资源**：P02，ADC0/ADC1、双 DMA 和公共触发链。
- **README**：[dual_channel_phase_meter/README.md](dual_channel_phase_meter/README.md)
- **projectspec**：`dual_channel_phase_meter/ticlang/dual_channel_phase_meter_q31_LP_MSPM0G3507_nortos_ticlang.projectspec`
- **基于它修改**：适合保留一种相位后端、增加通道校准；N=1024 当前只剩约 3,040 B SRAM，必须看 `.map`。

### DDS Generator【简单参考】

- **它是干什么的**：生成软件可调正弦采样块并由 DAC DMA 循环输出。
- **适合什么时候选**：需要改频率、峰值幅度、直流偏置和初相位的周期信号源。
- **不适合什么时候选**：只要固定直流；或需要逐点实时计算而不是 DMA block。
- **主要模块**：Sine、DAC Wave Table、DDS、DAC DMA、DAC DMA Platform Adapter。
- **输入 / 结果**：输入为配置参数；结果是 PA15 / DAC_OUT 的连续模拟波形。
- **ADC / FFT**：不使用 ADC；不使用 FFT。
- **默认参数**：1 kHz，1 Vpeak，1.65 V offset，0°；DAC update=100 kHz；table=256；DMA block=1000。
- **硬件资源**：P03，DAC0 + TIMG6 + DMA1 + Event。
- **README**：[dds_generator/README.md](dds_generator/README.md)
- **projectspec**：`dds_generator/ticlang/dds_generator_round1_LP_MSPM0G3507_nortos_ticlang.projectspec`
- **基于它修改**：适合换表生成其他周期波、增加按键/TFT 控制；必须保证 `offset ± amplitude` 不越 DAC 量程。

### Sweep Analyzer【综合参考】

- **它是干什么的**：DDS/DAC 激励外部 DUT，逐点采集响应并得到 frequency/gain/phase。
- **适合什么时候选**：滤波器、放大器、模拟网络的频率响应准备。
- **不适合什么时候选**：没有外部 DUT 连线或校准路径；只测单一频点。
- **主要模块**：Sine/Wave Table、DDS、DAC DMA、ADC DMA、ADC To Voltage、Lock-in、Sweep Glue。
- **输入 / 结果**：PA15 激励 DUT，PA25 采 DUT 输出；每个扫频点输出频率、增益、相位。
- **ADC / FFT**：ADC DMA；不使用 FFT，幅相来自 Lock-in。
- **默认 N**：1024；ADC/DAC update=100 kHz；1～10 kHz、1 kHz 步进。
- **硬件资源**：P04；当前基线 Flash 18,336 B、SRAM 9,687 B。
- **README**：[sweep_analyzer/README.md](sweep_analyzer/README.md)
- **projectspec**：`sweep_analyzer/ticlang/sweep_analyzer_final_LP_MSPM0G3507_nortos_ticlang.projectspec`
- **基于它修改**：最适合改扫频范围/步进/稳定时间、加入参考通道或校准；当前绝对幅相仍需硬件校准。

### Waveform Capture Replay【综合参考】

- **它是干什么的**：捕获历史波形、找触发和周期段，再重采样后从 DAC 重放。
- **适合什么时候选**：稳定周期波的捕获、提取和形状重放。
- **不适合什么时候选**：非周期瞬态、周期强烈漂移，或必须保持绝对幅值/偏置。
- **主要模块**：ADC DMA、ADC Ring Buffer、Trigger Capture、周期段、线性重采样、DAC DMA。
- **输入 / 结果**：ADC 模拟波形；输出 PA15 DAC 重放波形。
- **ADC / FFT**：ADC DMA；不使用 FFT。
- **默认 N**：捕获 2048，重放表 512，Fs=100 kHz。
- **硬件资源**：P04；当前基线 Flash 7,448 B、SRAM 18,173 B。
- **README**：[waveform_capture_replay/README.md](waveform_capture_replay/README.md)
- **projectspec**：`waveform_capture_replay/ticlang/waveform_capture_replay_final_LP_MSPM0G3507_nortos_ticlang.projectspec`
- **基于它修改**：适合改触发电平/边沿/前触发和重放长度；AutoRange 只保留归一化形状。

### Signal Analyzer【综合参考】

- **它是干什么的**：在一个工程里用编译期 Profile 选择 Basic、Frequency、Spectrum、THD 或 Phase。
- **适合什么时候选**：需要比较多条完整处理链，或从综合工程删成功能专用工程。
- **不适合什么时候选**：第一次只想拼三个模块；此时 Signal Meter 或 Contest Template 更简单。
- **主要模块**：P02 双 ADC 采集，以及由 `signal_features.h` 选中的正式算法链。
- **输入 / 结果**：单路或双路 ADC；结果随 Profile 改变。
- **ADC / FFT**：双 ADC 平台；Spectrum/THD/Phase Profile 使用 Q31 FFT。
- **默认 N**：512；Fs=100 kHz；默认 Profile=Spectrum。
- **硬件资源**：P02。
- **README**：[signal_analyzer/README.md](signal_analyzer/README.md)
- **projectspec**：`signal_analyzer/ticlang/signal_analyzer_final_LP_MSPM0G3507_nortos_ticlang.projectspec`
- **基于它修改**：优先选择一个 Profile，再删掉不用的结果和 buffer；不要让所有功能无条件同时运行。

### Signal Contest Template【从零拼装模板】

- **它是干什么的**：提供 `Init → Acquire → Process → Result → Output` 的薄应用骨架。
- **适合什么时候选**：你已经把功能分成 Direct DriverLib、复杂硬件、算法和外部器件，准备逐项接入并 Build。
- **不适合什么时候选**：还没有判断实现层级；先看 `00_docs/CONTEST_IMPLEMENTATION_GUIDE.md`。
- **已经提供**：`main.c`、集中参数、Feature Profile、Pipeline Glue、P06 完整硬件参考和可导入 projectspec。
- **输入 / 结果**：由你选择的采集模块和 `signal_pipeline` Profile 决定。
- **ADC / FFT**：P06 提供完整参考；默认 FFT backend=Q31，Math backend=Reference。
- **默认 N**：512；Fs=100 kHz。
- **硬件资源**：P06 是完整参考，实际比赛工程应删除不用的外设和模块。
- **README**：[signal_contest_template/README.md](signal_contest_template/README.md)
- **projectspec**：`signal_contest_template/ticlang/signal_contest_template_final_LP_MSPM0G3507_nortos_ticlang.projectspec`
- **基于它修改**：这是最适合手工增删模块的目录；按各模块 README 只冻结复制必要文件，并在 `COPIED_MODULES.md` 记录来源。

### Peripheral System Template

- **它是干什么的**：只提供 Hardware Acquisition、Algorithm Hook、Output 三段回调骨架。
- **适合什么时候选**：想自己控制最薄的 application 结构，或学习平台适配边界。
- **不适合什么时候选**：希望导入后立即采样运行；默认 Acquire 返回 `NOT_SUPPORTED`。
- **主要文件**：`signal_hw_config.h`、`peripheral_system_template.c/.h`、`main_template.c`。
- **ADC / FFT / N**：由你接入的模块决定，不预先绑定算法。
- **硬件资源**：默认 projectspec 带 P01；也可手工换 P02～P06 Profile。
- **README**：[peripheral_system_template/README.md](peripheral_system_template/README.md)
- **projectspec**：`peripheral_system_template/ticlang/peripheral_system_template_LP_MSPM0G3507_nortos_ticlang.projectspec`
- **基于它修改**：填 `App_Acquire`、`App_AlgorithmHook` 和 `App_Output`，每填一段先 Build。

### Oscilloscope（组件参考，不是完整可导入工程）

- **它是干什么的**：把均值、极值、Vpp、RMS 和 AC RMS 组合为一个纯算法分析函数。
- **适合什么时候选**：需要查看组合 API 的写法，或把基础测量组合进自己的应用。
- **不适合什么时候选**：希望直接 Import、SysConfig、烧录；当前目录没有 `main.c`、projectspec 或 `.syscfg`。
- **ADC / FFT**：没有硬件采集；不使用 FFT。
- **README**：[oscilloscope/README.md](oscilloscope/README.md)
- **当前定位**：Application 组件/草稿，不应当作 `PORTABLE_IMPORT_BUILD` 工程。

## common 不是独立 Application

`common/` 保存 Integration Glue、Dual ADC Platform Adapter 和 DAC DMA Platform Adapter。它们由上面的应用链接使用，没有自己的 `main.c` 或 projectspec。只有当模块接口需要它们时才加入，不要单独 Import。

## 当前空占位目录

以下目录真实存在，但当前没有源文件、README 或 projectspec，因此不是可选择的 Application：

- `automatic_gain/`
- `sweep_measurement/`
- `waveform_replay/`

不要因为看见目录名就把它们当成已完成工程。对应的现成功能应分别从 `sweep_analyzer/` 或 `waveform_capture_replay/` 等非空工程选择。

## 正确开始方式

1. 新赛题优先复制并 Import `signal_contest_template/ticlang/*.projectspec`；其他 Application 主要用于查看已验证组合。
2. 打开所选 Application 和每个模块自己的 README，确认输入、结果、N/Fs、复制清单与硬件 Profile。
3. 把 README 明列的必要 `.c/.h/.inc` 冻结复制进新工程 `modules/`，每加入一个模块就 Build 一次。
4. 正常比赛母版不需要设置 `MSPM0_SIGNAL_LIBRARY_ROOT`，也不需要逐项目手工补正式库 Include Path。
5. `COPIED_MODULES.md` 记录 canonical 来源；公共库仍是维护真源，比赛工程是可复现的冻结副本。

## 外部器件是可选 Backend，不强绑定 Application

外部器件库入口：[`../12_external_devices/README.md`](../12_external_devices/README.md)。Application 仍围绕“采集、产生、处理、显示”的功能接口组织，不直接绑定某个商家模块板：

| Application 功能 | 当前内部实现 | 可以替换/增加的外部设备 | 连接原则 |
|---|---|---|---|
| Signal Generator / Sweep 激励 | 内部 DDS → DAC DMA | 外部 DDS 或外部 DAC | 在 Generator Adapter 边界切换；算法层不直接写 GPIO/SPI |
| Signal Meter / Analyzer 输入 | MSPM0 ADC DMA | 外部 SPI/并行 ADC | 适配成明确的 raw buffer、sample count、sample rate 和码型后再接算法 |
| Result / UI | UART、现有 ILI9341 | SSD1306、串口屏等 | 结果结构与显示驱动分离；显示失败不影响测量链 |
| Gain/Range control | 内部 OPA/GPAMP 或固定前端 | Digital Pot、PGA/VGA、Analog MUX、Relay | 用独立 control adapter，不把寄存器写入散在测量代码中 |

加入陌生器件时先完成外部库的最小 Bring-Up；PASS 后按该器件 README 只冻结复制正式驱动所需文件并记录来源。不要临时重写第二份驱动，也不要为了一个显示器或 ADC 改写算法 Backend。
