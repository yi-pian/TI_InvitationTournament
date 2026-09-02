# Copy Assembly Readiness

日期：2026-08-12

> **算法简化后的解释（2026-08-11）：** 本文记录的旧算法 COPY TEST 仍是有效构建证据，但 ADC To Voltage、Vpp、RMS、AC RMS、Remove DC、普通 Peak 等已经降为 `LEVEL_A_DIRECT_RECIPE`，新比赛工程不再需要复制这些 `.c/.h`。它们的旧 `COPY_READY` 应理解为“兼容 API 仍可构建”，推荐入口以算法 Cookbook 为准。FFT、FFT Magnitude、Interpolation、Harmonic、THD、Phase 等复杂模块的 COPY_READY 结论不变。

## 状态定义

- `DIRECT_DRIVERLIB`：功能过于简单，不应复制 Signal Module；使用 SysConfig + 当前 TI DriverLib。
- `COPY_READY`：从隔离空工程只复制 README 指定文件，使用 README 对应最小例，完成 SysConfig generate、TI Arm Clang compile 和完整 application link。
- `NOT_READY`：缺正式驱动/比赛入口，或未通过上述隔离测试。

`COPY_READY` 不等于 `BOARD_VERIFIED`。本轮 COPY TEST 没有连接开发板，所有表内 Board 均为 `NOT_RUN`。

空白 `signal_contest_template` 自身也已单独完成 `SysConfig PASS / Compile PASS / Full Link PASS`；基线镜像 Flash 512 B、SRAM（含栈）516 B。

## 1. Direct DriverLib

| 功能 | 状态 | 正确入口 |
|---|---|---|
| GPIO set/clear/read | `DIRECT_DRIVERLIB` | SysConfig GPIO + `DL_GPIO_*` |
| 固定 DAC code | `DIRECT_DRIVERLIB` | SysConfig DAC12 + `DL_DAC12_output12(...)` |
| 单点 ADC bring-up | `DIRECT_DRIVERLIB` | SysConfig ADC12 + 当前 SDK ADC start/get-result API |
| 简单 Timer start/stop/read | `DIRECT_DRIVERLIB` | SysConfig Timer + `DL_Timer*` |
| 简单 blocking UART/SPI | `DIRECT_DRIVERLIB` | SysConfig + 当前 SDK blocking API |

这些项目不做模块 COPY TEST，因为正确用法就是不复制模块。

## 2. 复杂硬件模块 COPY TEST

假定 MSPM0G3507 资源上限为 Flash 131072 B、SRAM 32768 B；余量只是最小例构建余量，不包含你比赛工程后续大 Buffer。

| 模块 | 比赛入口/复制范围 | SysConfig | Compile | Full Link | Flash B / 余量 | SRAM B / 余量 | 状态 |
|---|---|---|---|---|---:|---:|---|
| ADC DMA | `signal_adc_dma.c/.h` + `signal_status.h` | PASS | PASS | PASS | 2656 / 128416 | 565 / 32203 | `COPY_READY` |
| ADC FIFO DMA | `signal_adc_fifo_dma.c/.h` + `signal_status.h` | PASS | PASS | PASS | 1848 / 129224 | 2573 / 30195 | `COPY_READY` |
| Dual ADC | `signal_dual_adc_mspm0g3507.c/.h` + status | PASS | PASS | PASS | 3192 / 127880 | 603 / 32165 | `COPY_READY` |
| Timer Capture | `signal_timer_capture_mspm0g3507.c/.h` + status | PASS | PASS | PASS | 2368 / 128704 | 771 / 31997 | `COPY_READY` |
| DAC DMA | `signal_dac_dma_mspm0g3507.c/.h` + status | PASS | PASS | PASS | 2584 / 128488 | 688 / 32080 | `COPY_READY` |
| DDS | `signal_dds.c/.h` + status | PASS | PASS | PASS | 3368 / 127704 | 561 / 32207 | `COPY_READY` |
| TFT ILI9341 | core c/h + font inc + MSPM0 entry c/h + status | PASS | PASS | PASS | 17864 / 113208 | 597 / 32171 | `COPY_READY` |
| SSD1306 0.96 OLED | core c/h + font inc + MSPM0 I2C adapter + MSPM0G3507 binding + common bus | PASS | PASS | PASS | 3776 / 127296 | 1560 / 31208 | `COPY_READY` |
| ST7789 2.4 TFT | core c/h + MSPM0G3507 SPI entry c/h + status | PASS | PASS | PASS | 3880 / 127192 | 601 / 32167 | `COPY_READY` |

硬件 profile 分别来自 P01 ADC、P08 ADC FIFO、P02 Dual ADC、P05 Capture、P03 DAC 和 TFT 专用 `.syscfg`。DDS 本身无硬件 Pin，使用空母版 profile。

## 3. 算法模块 COPY TEST

算法在空白 Board/SYSCTL profile 中测试，用来证明它不偷偷依赖 Application、Platform 或仓库 Include Path。

| 模块 | 额外复制依赖 | SysConfig | Compile | Full Link | Flash B / 余量 | SRAM B / 余量 | 状态 |
|---|---|---|---|---|---:|---:|---|
| ADC To Voltage | algorithm status | PASS | PASS | PASS | 1896 / 129176 | 529 / 32239 | `COPY_READY` |
| VPP | algorithm status | PASS | PASS | PASS | 1152 / 129920 | 525 / 32243 | `COPY_READY` |
| RMS | status + math backend headers | PASS | PASS | PASS | 2080 / 128992 | 521 / 32247 | `COPY_READY` |
| AC RMS | status + math backend headers | PASS | PASS | PASS | 2136 / 128936 | 525 / 32243 | `COPY_READY` |
| Remove DC | algorithm status | PASS | PASS | PASS | 1288 / 129784 | 533 / 32235 | `COPY_READY` |
| Window | algorithm status | PASS | PASS | PASS | 7288 / 123784 | 557 / 32211 | `COPY_READY` |
| FFT | status + complex + backend config | PASS | PASS | PASS | 9496 / 121576 | 581 / 32187 | `COPY_READY` |
| FFT Magnitude | status + complex | PASS | PASS | PASS | 2368 / 128704 | 541 / 32227 | `COPY_READY` |
| Peak Detect | algorithm status | PASS | PASS | PASS | 952 / 130120 | 521 / 32247 | `COPY_READY` |
| FFT Parabolic Interpolation | algorithm status | PASS | PASS | PASS | 1944 / 129128 | 529 / 32239 | `COPY_READY` |
| Harmonic | status + Multi Bin Energy c/h | PASS | PASS | PASS | 2936 / 128136 | 877 / 31891 | `COPY_READY` |
| THD | status + harmonic header | PASS | PASS | PASS | 2488 / 128584 | 533 / 32235 | `COPY_READY` |
| Phase | status + complex + math backend headers | PASS | PASS | PASS | 1888 / 129184 | 521 / 32247 | `COPY_READY` |

FFT 最小例只用 N=8，所以表内 SRAM 绝不能作为比赛 N=1024/2048 的预算；真实 complex buffer 为 `8N` 字节，另加输入、窗口、magnitude 和栈。

## 4. 第一批未准备好

| 项目 | 状态 | 真实原因 |
|---|---|---|
| Rotary Encoder / EC11 | `NOT_READY` | 已有正式 `01_bsp/rotary_encoder`，PC 测试及合适 SysConfig profile 下 TI 完整链接 PASS；但尚未完成独立 SysConfig/COPY TEST，也未接真实编码器 |
| TFT Waveform | `NOT_READY` | 已有正式 `01_bsp/tft_waveform`，PC 测试及 TFT profile 下 TI 完整链接 PASS；但 `tft_waveform + tft_ili9341` 依赖链尚未做隔离 COPY TEST，真实刷新耗时也未上板 |
SSD1306 已在 2026-08-12 根据明确模块 `WH-X096-2864KSWEG01-A4` 资料补齐正式驱动、MSPM0 I2C 适配层和独立 COPY TEST，已从本节移出。仍未上板，不能标 `BOARD_VERIFIED`。

## 5. 测试隔离规则

脚本：`tools/run_copy_assembly_tests.ps1`

每个目标都在 `10_tests/copy_assembly/build/<target>/copied_project/` 创建独立副本：

1. 复制 README 最小例为 `main.c`；
2. 机器检查 README 确实逐个写出全部必需文件名和 `README_MINIMAL_EXAMPLE.c`；
3. 只复制列出的正式模块文件到 `modules/`；
4. 复制对应 `.syscfg` 并生成到独立目录；
5. Include Path 仅允许复制工程、生成目录、TI SDK 和 CMSIS Core；
6. 编译复制件、生成件和 TI startup；
7. 完整链接并读取 `.map`。

结果原始记录：

- `10_tests/copy_assembly/build/copy_assembly_results.json`
- `10_tests/copy_assembly/build/copy_assembly_results.csv`

本轮结果：新增 SSD1306 与 ST7789 目标均完成 `README manifest + COPY TEST PASS`。2026-08-17 全库脚本实测 `14/22` 目标通过；其余 8 项因既有算法目录缺 README，在 PREPARE 阶段 `NOT_READY`，不影响两个显示模块。Board：`NOT_RUN`。
