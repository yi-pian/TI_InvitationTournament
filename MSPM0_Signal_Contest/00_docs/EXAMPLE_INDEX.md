# Example Index

本索引只整理有源码/README/构建或测试证据的现成入口。`BUILD_VERIFIED` 只表示目标工程完成 SysConfig、compile 和 final link；两个分别通过的工程不能拼成一个新的 `BUILD_VERIFIED` Example。

## Agent 搜索入口

```text
rg -n -i "ADC DMA|FFT|Display|DDS|扫频|ADC \+ DAC|AGC|TFT|4x4|运放|频响" MSPM0_Signal_Contest/00_docs/EXAMPLE_INDEX.md
```

## 快速表

| 搜索词 / 功能 | 正式入口 | 状态 | 关键限制 |
|---|---|---|---|
| ADC DMA；DMA 采集 | `09_examples/adc_dma_onboard_selftest` / `adc_dma_demo` | 板载慢信号 `BOARD_VERIFIED`；PA25 动态输入 `BUILD_VERIFIED` | TMP6131 证据不能外推到动态带宽 |
| ADC DMA + FFT；频谱 | `08_applications/spectrum_analyzer` | `BUILD_VERIFIED`；算法 truth `PC_VERIFIED` | Board `NOT_RUN`；新工程算法改从正式算法库选择 |
| ADC DMA + FFT + Display | 无同一端到端工程 | `NOT_RUN` | Spectrum 与 TFT 分别 Build 不能合并冒充验证 |
| DDS 输出；DAC DMA | `08_applications/dds_generator` | `BUILD_VERIFIED` | Board `NOT_RUN`，幅值/失真/更新率未实测 |
| DDS 扫频；ADC + DDS；频响 | `08_applications/sweep_analyzer` | `BUILD_VERIFIED`；PC Glue PASS | 单 ADC 路径，绝对 gain/phase 仍需 thru/参考通道校准 |
| ADC + DAC | `09_examples/integration_profiles/PROFILE_04_ADC_DAC` | `BUILD_VERIFIED` | 环回 Board `NOT_RUN` |
| 自动幅度；AGC | `08_applications/automatic_gain` 为空 | `NOT_RUN` | 只有 Recipe，无可调用完整 Application |
| TFT；ILI9341 | `09_examples/tft_ili9341_lp_mspm0g3507` | `BUILD_VERIFIED` | 未观察实屏；RESET 未接 MCU |
| 4x4 键盘 | `01_bsp/matrix_keypad_4x4` | `PC_VERIFIED` + 模块 build | 无完整固定 `.syscfg` Example；Board `NOT_RUN` |
| 运放参数测试 | `fuxian/24_A` 历史工作目录 | `NOT_RUN` | 现 README 是骨架；历史产物不等于当前验收 |
| 频率响应 / -3 dB | `08_applications/sweep_analyzer` + Measurement Recipe | `BUILD_VERIFIED` / Recipe `DRAFT` | 未做绝对幅相/截止板测 |
| THD / 谐波 | `08_applications/harmonic_thd_analyzer` | `BUILD_VERIFIED`；算法 truth `PC_VERIFIED` | Board `NOT_RUN`，H5/Nyquist 必查 |
| 预测题：双通道可调信号分析仪 | `fuxian/example01/` | 源码整理；`BOARD NOT_RUN` | ST7789、4×4 键盘、双 ADC、过零相位；未做当前 CCS 全量构建 |
| 预测题：扫频网络幅频/相频测试仪 | `fuxian/example02/` | 源码整理；`BOARD NOT_RUN` | DDS/DAC DMA/双 ADC/ST7789/键盘；DAC CH2 资源需现场核对 |
| 预测题：猝发/调制双路信号识别仪 | `fuxian/example03/` | 源码整理；`BOARD NOT_RUN` | 连续 ADC、Ping-Pong、Ring Buffer、ST7789/键盘；跨块猝发需扩展状态 |

| 预测题：可编程信号综合测量与波形复现仪 | `fuxian/example04/` | 源码整理；`BOARD NOT RUN` | OUT 四波形、双 ADC、三路测频、FFT/THD、鲁棒测量、Sine Fit、Lock-In、单次捕获复现、校准；未做 CCS 全量构建和实板验证 |

## E01 ADC DMA 采集

- 功能与 Pipeline：`Timer -> Event -> ADC0 -> DMA0 -> uint16_t raw[N]`。板载自检另含 TMP6131 与 LFXT/FCC；动态 Demo 使用 PA25/ADC0.2。
- 模块/API：`02_acquisition/adc_dma`；真实入口 `SignalADC_Init`、`SignalADC_Start`、`SignalADC_IsFinished`、`SignalADC_GetBuffer`、`SignalADC_GetSampleCount`、`SignalADC_GetConfiguredTriggerRate`，使用前仍重读 `signal_adc_dma.h`。
- SysConfig/资源：P01 类 `ADC0 + TIMG0 + Event1 + DMA0`；动态 Demo 另用 PA12 输出验收时钟；UART 仅调试时按 Profile。
- 接线：板载自检要求 J9 1-2/J13；动态输入为信号源 OUT→PA25/J1.2、GND 共地，电压范围以 README/板卡资料为准。
- 文件与 main：源 README、`main.c`、`signal_config.h`、`*.projectspec`、`.syscfg`/验收文档；main 顺序为 SysConfig→Init→Start→WFE→检查 buffer/哨兵。
- RAM/Flash：板载自检 map 约 text 4,088 B、BSS 8,792 B，4096 点 raw buffer 占 8,192 B；以目标 `.map` 为准。
- 状态：[`adc_dma_onboard_selftest`](../09_examples/adc_dma_onboard_selftest/README.md) 的板载近 DC 路径为 `BOARD_VERIFIED`；[`adc_dma_demo`](../09_examples/adc_dma_demo/README.md) 当前为 `BUILD_VERIFIED`，动态实板 `NOT_RUN`。
- 已知限制：`GetConfiguredTriggerRate` 是配置推导值，不是采样率实测；TMP6131 不能证明 PA25 高速动态性能。

## E02 ADC DMA + FFT 频谱

- 功能与 Pipeline：`ADC DMA -> raw to V -> Remove DC -> Hann -> FFT -> Magnitude -> gain correction -> Peak/Interpolation`。
- 模块/API：硬件入口同 E01；当前 Application 入口是 `SignalIntegration_RawToVoltage`、`SignalIntegration_Spectrum`。该 Application 保留历史兼容算法链接；新比赛工程的算法必须从 `MSPM0_Signal_Contest` 读取正式 `.h` 并选择性复制。
- SysConfig/资源：P01，PA25/ADC0.2、TIMG0、Event1、DMA0；Q31/CMSIS backend 还占用库和 workspace，不改 backend 就不要改其配置。
- 接线：模拟输入→PA25，GND 共地；输入保护/范围由实际前端决定。
- 文件与 main：[`spectrum_analyzer/README.md`](../08_applications/spectrum_analyzer/README.md)、`main.c`、`signal_config.h`、Q31 projectspec；新工程从 `signal_contest_template` 复制需要的硬件/正式算法文件。
- RAM/Flash：N=1024 Q31 当前链接 SRAM 17,045 B；改 N/backend 后必须重新 full link/read map。
- 状态：Application `BUILD_VERIFIED`，Q31 PC truth PASS；Board=`NOT_RUN`。
- 已知限制：FFT 输出 complex，不是 magnitude/Hz/V；N/2+1、窗口增益、实际 Fs 和峰值插值必须保持一致。

## E03 ADC DMA + FFT + Display

- 功能目标：采集→FFT→显示频谱/峰值。
- 可复用模块：E02 Spectrum 链和 E08 ILI9341 分别存在；没有同一工程的资源表、main、map 或板测证据。
- SysConfig/资源预检查：P01 的 ADC/Timer/Event/DMA 加 SPI1、CS/DC/BL GPIO；还要检查 UART、Pin、IRQ、峰值 live RAM 和显示刷新阻塞。
- API/接线：必须分别从 `signal_adc_dma.h`、正式算法 `.h`、`signal_tft_ili9341.h` 和 TFT MSPM0 platform `.h` 查；禁止凭两个 README 猜 glue。
- 文件/main：尚无正式 Example 文件清单；应从 `signal_contest_template` 逐模块加入并每步 Build。
- RAM/Flash：未知；不能把 E02/E08 map 简单相加替代 live RAM 与最终 link。
- 状态：`NOT_RUN`。
- 已知限制：显示刷新不能在 ADC/DMA ISR 中执行；是否能复用 buffer 必须由各 `.h/.c` 的 ownership/in-place 证据决定。

## E04 DDS + DAC DMA 输出

- 功能与 Pipeline：`Sine table -> DDS phase accumulator -> uint16_t block -> DAC DMA -> DAC0/PA15`。
- 模块/API：`SignalSine_Generate`、`SignalDDS_Init`、`SignalDDS_Fill`、`SignalDACPlatform_Init`、`SignalDACDMA_Init/Start`；以各当前 `.h` 参数为准。
- SysConfig/资源：P03，DAC0/PA15、DMA1、TIMG6、Event3。
- 接线：PA15 为模拟输出；外接负载、滤波、共地和允许范围必须按板卡/被测电路确认。
- 文件/main：[`dds_generator/README.md`](../08_applications/dds_generator/README.md)、`main.c`、`signal_config.h`、Application `.c/.h`、projectspec；main 顺序为 table→DDS→Fill→DAC platform/DMA→Start。
- RAM/Flash：波表和 DMA block 同时存活；具体数值读目标 map。
- 状态：`BUILD_VERIFIED`，Board=`NOT_RUN`。
- 已知限制：software DDS 不等于硬件 DDS；`offset±amplitude`、更新率、表边界、DAC settling/THD 未实板验证。

## E05 DDS 扫频 / ADC + DDS / Frequency Response

- 功能与 Pipeline：`DDS -> DAC DMA/PA15 -> DUT -> PA25/ADC DMA -> voltage -> LockIn -> sweep point`。
- 模块/API：`SignalFrequencySweep_Generate`、DDS/DAC DMA、ADC DMA、`SignalADCToVoltage_Process`、`SignalLockIn_Process`、`SignalSweepAnalyzer_PointAtFrequency`。
- SysConfig/资源：P04，ADC0/PA25+DMA0+TIMG0+Event1；DAC0/PA15+DMA1+TIMG6+Event3；UART0 PA10/PA11 仅 debug。
- 接线：PA15→外部 DUT 输入，DUT 输出→PA25，所有设备共地并确认电压/偏置/保护；直连环回仅在幅值安全时使用。
- 文件/main：[`sweep_analyzer/README.md`](../08_applications/sweep_analyzer/README.md)、`main.c`、`signal_config.h`、Application `.c/.h`、projectspec；main 按 set frequency→settle→acquire→process→store→next。
- RAM/Flash：当前 baseline Flash 18,352 B、SRAM 9,687 B。
- 状态：`BUILD_VERIFIED`、PC Glue PASS，Board=`NOT_RUN`。
- 已知限制：当前 reference amplitude 来自 DDS 设定，不是同步参考通道；绝对增益/相位与 -3 dB 结论需 thru/fixture 和通道延时校准。

## E06 ADC + DAC Profile

- 功能：把 P01 ADC capture 与 P03 DAC generator 放入一个可链接资源基线；不是完成的环回 Application。
- Pipeline：DAC0/PA15 独立更新；ADC0/PA25 独立采集；由 Application 决定是否接线/闭环。
- API：Profile 不定义新 API；使用 ADC DMA、DAC DMA 与各 platform 真实头文件。
- SysConfig/资源：ADC DMA0/TIMG0/Event1；DAC DMA1/TIMG6/Event3；UART0 PA10/PA11 debug。
- 接线：未来环回 PA15→PA25；先确认 DAC 输出范围、ADC 输入范围与共地。
- 文件/main：[`PROFILE_04_ADC_DAC/README.md`](../09_examples/integration_profiles/PROFILE_04_ADC_DAC/README.md) 和 `profile.syscfg`；最小 profile main 位于对应构建验证目标。
- RAM/Flash：只证明最小镜像可链接；完整 Application 加 buffer/算法后重新读 map。
- 状态：`BUILD_VERIFIED`；Board 环回=`NOT_RUN`。

## E07 自动量程 / AGC

- 功能与 Recipe：见正式算法库 `auto_range.md`、`automatic_gain.md` 和 `vga_calibration.md`。
- 现成模块/API：幅值/削顶/校准 Primitive 可复用；硬件 set gain API 依 exact VGA/PGA/数字电位器而定。
- SysConfig/接线/文件/main：尚无统一完成工程；`08_applications/automatic_gain` 当前为空。
- RAM/Flash：未知，依幅值算法和硬件控制表。
- 状态：`NOT_RUN`。
- 已知限制：不能把 Recipe 状态机或占位函数名当现成 Application；锁定前不能采正式 THD/幅值帧。

## E08 ILI9341 TFT

- 功能与 Pipeline：`main -> TFT callbacks -> SPI1/GPIO -> ILI9341`，显示彩条、ASCII 与中文字模。
- 模块/API：初始化入口 `SignalTFTILI9341_MSPM0_Init`；绘制 API 以 `signal_tft_ili9341.h` 为准。
- SysConfig/资源：SPI1 mode 0/8-bit/MSB/8 MHz；PB9 SCLK、PB8 MOSI、PB6 CS、PB15 DC、PB12 BL；POCI PA16 配置但不接屏。
- 接线：完整表见 [`TFT Example README`](../09_examples/tft_ili9341_lp_mspm0g3507/README.md)；BL 若为电源脚不得由 PB12 直接供电。
- 文件/main：README、`main.c`、`tft_ili9341.syscfg`、projectspec；正式 driver `.c/.h/.inc` 由工程链接。
- RAM/Flash：2026-08-12 本轮重建 map Flash 21,064 B、SRAM 637 B（含 512 B stack）；后续仍以目标工程最新 `.map` 为准。
- 状态：`BUILD_VERIFIED`，实屏=`NOT_RUN`。
- 已知限制：当前 RESET 未接 MCU；白屏先排电源/共地/SDI/CS/DC/mode/时钟，不能仅改代码猜测。

## E09 4×4 矩阵键盘

- 功能与 Pipeline：4 行轮询驱动→4 列上拉读取→消抖→pressed/released/stable/ghost event→Application。
- 模块/API：`SignalMatrixKeypad4x4_Init`、`SignalMatrixKeypad4x4_Scan`、`GetKey/GetFirstPressed`；MSPM0 platform callbacks 已有正式实现。
- SysConfig/资源：8 GPIO；不使用 DMA/Timer/IRQ。README 给出一套 LP 引脚建议，但没有固定通用 Profile。
- 接线：必须先用万用表确认真实 R1..R4/C1..C4，不能猜 8-pin 排列；行 output 初始高、列 input pull-up。
- 文件/main：[`matrix_keypad_4x4/README.md`](../01_bsp/matrix_keypad_4x4/README.md)、正式 `.c/.h` 与 MSPM0 platform；main 每约 5 ms Scan，UI 看 `pressed_mask`。
- RAM/Flash：动态内存 0，状态几十字节；最终 Flash 读目标 map。
- 状态：扫描/消抖 `PC_VERIFIED`，模块 TI compile/link 通过；真实键盘 Board=`NOT_RUN`。
- 已知限制：无二极管矩阵鬼键无法靠软件完全消除；这不是一个完整固定 SysConfig Application Example。

## E10 运放参数测试

- 当前证据：`fuxian/24_A` 有历史工程文件、`.out/.map` 和详细操作指南，但顶层 README 仍把主程序定义为返回 `NOT_SUPPORTED` 的骨架；未形成统一的题目、结果、版本与验收表。
- 可复用部分：ADC DMA、AC RMS/Robust VPP、AD9850、TFT、4×4 键盘与相关正式源可分别查库。
- SysConfig/接线/API/main：必须从该历史工程与正式库重新核对，不能把 Debug 生成文件或副本当 source of truth。
- RAM/Flash：历史 map 可用于线索，不足以证明当前重建工程；需要当前工具链 full link。
- 状态：作为“运放参数测试 Example”为 `NOT_RUN`。
- 已知限制：在补齐原题、Pipeline、资源 owner、当前 build 与板级仪器结果前，不升级为 `BUILD_VERIFIED/BOARD_VERIFIED/CONTEST_VERIFIED`。

## E11 Harmonic / THD

- Pipeline：`ADC DMA -> Voltage -> Remove DC -> Hann -> FFT -> Magnitude -> f0 interpolation -> H1..H5 multi-bin -> THD`。
- API：当前历史 Application 用 `SignalIntegration_RawToVoltage/THD`；新工程算法从正式 `SignalHarmonic_Process`、`SignalTHD_Process` 等头文件选择。
- SysConfig/资源：P01，ADC0/PA25、TIMG0、Event1、DMA0，另含 Q31 backend/workspace。
- 文件/main：[`harmonic_thd_analyzer/README.md`](../08_applications/harmonic_thd_analyzer/README.md)、`main.c`、`signal_config.h`、Application `.c/.h`、Q31 projectspec。
- RAM/Flash：改 N/Backend/谐波阶数后读 map；H5 必须低于 Nyquist 且主瓣不重叠。
- 状态：`BUILD_VERIFIED`、Q31 PC truth PASS，Board=`NOT_RUN`。
- 已知限制：不得在 THD 前滤掉目标谐波；绝对谐波幅值还需窗/频响校准。

## Contest reproductions

`contest_reproductions/` 当前只有说明页，没有可升级为 `CONTEST_VERIFIED` 的工程。`fuxian/2024_C_signal_detection_display` 顶层仍为 DRAFT/TBD，即使其 application 子目录有 build baseline，也不能作为完整赛题复现证据。
