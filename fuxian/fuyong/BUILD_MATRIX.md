# Build Matrix

## main.c 教学函数化重构复验（2026-08-22）

使用 SysConfig 1.28.0 与 TI Arm Clang 5.1.1.LTS，对每个工程实际的 `main.c`、全部 `modules/*.c`、SysConfig 生成的 `ti_msp_dl_config.c` 和 MSPM0G350x startup 执行 Generate → `-Wall -Werror` Compile → Link。16/16 PASS；未改模块 `.c/.h`。以下为本轮最终 Flash/SRAM（含链接器 512 B stack）：

| 工程 | Generate | Compile | Link | Flash | SRAM |
|---|---|---|---:|---:|---:|
| 01_adc_basic | PASS | PASS | PASS | 1,232 B | 514 B |
| 02_adc_dma | PASS | PASS | PASS | 2,768 B | 2,581 B |
| 04_dual_adc_dma | PASS | PASS | PASS | 4,784 B | 5,013 B |
| 10_timer_frequency | PASS | PASS | PASS | 2,144 B | 720 B |
| 11_zero_cross_frequency | PASS | PASS | PASS | 7,456 B | 15,256 B |
| 20_fft_analysis | PASS | PASS | PASS | 31,528 B | 21,908 B |
| 21_time_domain_waveform | PASS | PASS | PASS | 7,744 B | 5,060 B |
| 30_basic_measurement | PASS | PASS | PASS | 7,744 B | 13,228 B |
| 40_dual_channel_measurement | PASS | PASS | PASS | 7,808 B | 5,022 B |
| 50_robust_measurement | PASS | PASS | PASS | 9,352 B | 17,304 B |
| 60_precision_measurement | PASS | PASS | PASS | 16,000 B | 9,180 B |
| 61_lock_in | PASS | PASS | PASS | 13,984 B | 9,136 B |
| 70_keypad_usage | PASS | PASS | PASS | 5,008 B | 915 B |
| 80_tft_usage | PASS | PASS | PASS | 19,760 B | 920 B |
| 90_dds_usage | PASS | PASS | PASS | 13,648 B | 2,529 B |
| 91_dac_usage | PASS | PASS | PASS | 1,064 B | 512 B |

SysConfig 的 ADC wake-up、STOP/STANDBY 保持与 DMA Full Channel 信息仍为 `info`，不是 warning/error。Board：全部 `NOT_RUN`。

构建日期：2026-08-21。使用 CCS 自带 **SysConfig 1.28.0**：`D:\TI\CCS\ccs\utils\sysconfig_1.28.0\sysconfig_cli.bat`；TI Arm Clang 5.1.1.LTS；MSPM0 SDK 2.11.0.07。

每个工程已执行：Clean（移除旧 `.o/.out/.map/.d`）→ SysConfig Generate 到 `Debug/` → 编译 `main.c`、复制模块、`ti_msp_dl_config.c`、MSPM0G350x startup → Link。Flash/SRAM 来自各工程实际 `.map` 的 `MEMORY CONFIGURATION`，SRAM 包含 512 B stack。未进行 Flash/board 验证。

| 工程 | Generate | Compile | Link | Warnings / info | Flash | SRAM | Flash | Board |
|---|---|---|---|---|---:|---:|---|---|
| 01_adc_basic | PASS | PASS | PASS | ADC Auto power-down wake-up timing info | 1,232 B | 514 B | NOT_RUN | NOT_RUN |
| 02_adc_dma | PASS | PASS | PASS | ADC wake-up timing; DMA full-channel info | 2,776 B | 2,581 B | NOT_RUN | NOT_RUN |
| 04_dual_adc_dma | PASS | PASS | PASS | ADC wake-up; DMA full-channel; Capture/SPI/DAC timer STOP/STANDBY retention info | 4,792 B | 5,013 B | NOT_RUN | NOT_RUN |
| 10_timer_frequency | PASS | PASS | PASS | Capture STOP/STANDBY retention info | 2,152 B | 720 B | NOT_RUN | NOT_RUN |
| 11_zero_cross_frequency | PASS | PASS | PASS | Full profile informational messages only | 7,536 B | 15,260 B | NOT_RUN | NOT_RUN |
| 20_fft_analysis | PASS | PASS | PASS | Full profile informational messages only | 31,512 B | 21,904 B | NOT_RUN | NOT_RUN |
| 21_time_domain_waveform | PASS | PASS | PASS | Full profile informational messages only | 7,744 B | 5,060 B | NOT_RUN | NOT_RUN |
| 30_basic_measurement | PASS | PASS | PASS | Full profile informational messages only | 7,752 B | 13,228 B | NOT_RUN | NOT_RUN |
| 40_dual_channel_measurement | PASS | PASS | PASS | Full profile informational messages only | 7,824 B | 5,022 B | NOT_RUN | NOT_RUN |
| 50_robust_measurement | PASS | PASS | PASS | Full profile informational messages only | 10,512 B | 17,304 B | NOT_RUN | NOT_RUN |
| 60_precision_measurement | PASS | PASS | PASS | Full profile informational messages only | 16,008 B | 9,180 B | NOT_RUN | NOT_RUN |
| 61_lock_in | PASS | PASS | PASS | Full profile informational messages only | 13,992 B | 9,136 B | NOT_RUN | NOT_RUN |
| 70_keypad_usage | PASS | PASS | PASS | Full profile informational messages only | 5,008 B | 915 B | NOT_RUN | NOT_RUN |
| 80_tft_usage | PASS | PASS | PASS | Full profile informational messages only | 19,760 B | 920 B | NOT_RUN | NOT_RUN |
| 90_dds_usage | PASS | PASS | PASS | Full profile informational messages only | 13,616 B | 2,529 B | NOT_RUN | NOT_RUN |
| 91_dac_usage | PASS | PASS | PASS | ADC Auto power-down wake-up timing info | 1,064 B | 512 B | NOT_RUN | NOT_RUN |

Generate 的信息均非错误：ADC 提示 Auto power-down 的 wake-up 时间应纳入采样窗口；DMA “Full Channel” 是资源占用状态；Capture/SPI/DAC Timer 提示 STOP/STANDBY 后应恢复寄存器。没有 SysConfig warning/error、编译 warning/error 或链接 unresolved symbol。

所有构建产物位于各自 `Debug/`；未手改 `ti_msp_dl_config.*`、makefile、linker 生成文件或任何已有模块 `.c/.h`。
