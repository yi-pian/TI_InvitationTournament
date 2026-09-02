# ADC_DMA 板载自检（LEVEL 1）

本工程只做 `BOARD-ONLY VERIFIED` 准入，不建立任何测量算法模块：

```text
板载 TMP6131 -> J9 1-2 -> PB24 / ADC0.5
              -> Timer -> Event -> ADC0 -> DMA -> g_adc_buffer
板载 32.768 kHz LFXT -> FCC -> 交叉估算 32 MHz SYSOSC/Timer 节拍
```

`min/max/mean`、0xFFFF 哨兵、FCC 和验收全局变量全部在本例程中；正式 `signal_adc_dma.c` 没有 TMP6131、LFXT、FCC 或 UART 依赖。

## 十分钟操作

1. 断电检查跳线：`J9 1-2`，`J13 已安装`。J9 2-3 是放大器路径，本测试不要使用。
2. 用 USB 连接板卡。让 CCS 的导入目标位于本目录的 `ticlang` 下，再导入 `ticlang/adc_dma_onboard_selftest_LP_MSPM0G3507_nortos_ticlang.projectspec`。生成的 `${PROJECT_ROOT}` 应为 `ticlang/adc_dma_onboard_selftest_LP_MSPM0G3507_nortos_ticlang`。
3. Clean Project、Build、Download、Run。程序依次测试 N=256/512/1024/2048/4096，每档执行 100 次 `Start -> WFE -> Done`，总计 500 帧。
4. 成功后停在最后的 `__BKPT(0)`。在 Expressions 中加入下表变量。

| 变量 | LEVEL 1 通过值 |
|---|---:|
| `g_acceptance_complete` | `true` |
| `g_acceptance_pass` | `true` |
| `g_completed_sizes` | `5` |
| `g_completed_blocks` | `100`（最后一档） |
| `g_total_completed_blocks` | `500` |
| `g_failed_block` | `0xFFFFFFFF` |
| `g_failed_sample_count` | `0` |
| `g_last_result` | `SIGNAL_RESULT_OK` / `0` |
| `g_module_status` | `MODULE_DONE` / `2` |
| `g_sentinel_residue_count` | `0` |
| `g_wfe_completed_blocks` | `500` |
| `g_fcc_done`, `g_fcc_pass` | 都为 `true` |
| `g_configured_trigger_rate_hz` | `100000`，仅为配置推导值 |
| `g_estimated_trigger_rate_hz` | 约 `100000`，由 LFXT/FCC 板内估算 |

TMP6131 是缓慢变化的近直流信号，最后一帧的 `g_adc_raw_min/max/mean` 应落在 12-bit 范围内，不能全 0 或全 4095；室温下 mean 通常接近中量程。本测试用 1000..3100 的宽松窗口，只负责发现断路/饱和等明显错误，不把温度换算做成正式模块。

完整准入判据、CCS Graph、FCC 语义、故障排查和未来 DAC 环回方案见 [BOARD_ONLY_ACCEPTANCE_TEST.md](BOARD_ONLY_ACCEPTANCE_TEST.md)。

## 当前代码验证记录

- SysConfig 1.26.2：生成通过；Event 图为 TIMG0 publisher channel 1 -> ADC0 subscriber channel 1。
- CCS TI Arm Clang 5.1.1.LTS：`-O2 -Wall -Werror` 编译并完整链接通过。
- 链接结果：约 4088 B text、8792 B BSS；BSS 包括 8192 B 的 4096 点缓冲。
- 以上是本机静态/构建验证，不是实板运行结果；必须由你在板上看到通过变量后，才能把状态升级为 `BOARD-ONLY VERIFIED`。

## CCS 工程集成约束

`modules/adc_dma` 和 `modules/common` 是 Project Explorer 中的虚拟目录，只用于展示 linked resources。编译器 Include Search Path 直接指向唯一正式源码目录：

```text
${PROJECT_ROOT}/../../../../02_acquisition/adc_dma
${PROJECT_ROOT}/../../../../01_bsp/common
```

因此 `main.c` 保持 `#include "signal_adc_dma.h"`，`signal_adc_dma.h` 保持 `#include "signal_status.h"`；不复制模块文件，也不依赖虚拟目录作为 Windows 物理路径。修改 `.projectspec` 后，已经导入的 `.cproject` 不会自动刷新，必须移除旧导入工程并重新导入。
