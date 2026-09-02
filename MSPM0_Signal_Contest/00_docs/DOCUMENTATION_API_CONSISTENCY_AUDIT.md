# README ↔ Header ↔ Platform Consistency Audit

> 2026-08-10 简化更新：本审计继续保证旧 public API/README 不漂移，但“一致”不等于“新工程推荐”。DAC/GPIO/UART/单次 ADC minimum examples 已改为 Direct DriverLib；Deprecated/推荐结论见 [MODULE_ABSTRACTION_AUDIT.md](MODULE_ABSTRACTION_AUDIT.md)。

审计日期：2026-08-10  
唯一真相：当前正式 `.h`；README 和 Example 必须跟随头文件，不为旧文档恢复旧 API。

## 1. 审计范围与自动检查

- 扫描 99 份正式模块/Platform README；旧算法目录与非模块 common README 不计入；
- 扫描 103 个正式头文件；
- 枚举 MSPM0G3507 Platform 的 41 个 public API；
- 检查 README 引用的 `Signal*`/`TFT_ILI9341_*` 函数是否仍在正式头文件；
- 检查 README 中的 `signal_*_t`/`tft_*_t` 类型是否仍存在；
- 检查 32 份高风险 README 是否具有固定 `Hardware / Platform Binding` 章节、正式路径和真实 Example/Gap；
- 对两个 DAC DC `COMPILE_VERIFIED_EXAMPLE` 代码块与真实 `dac_dc_minimum/main.c` 做逐字符同步检查；
- 由 `tools/build_platform_closure.ps1` 对 10 个真实 Example 执行 SysConfig、TI Arm Clang `-Wall -Werror` compile 和 final link。

自动检查脚本：`tools/validate_documentation_api_consistency.ps1`。机器可读结果：`10_tests/documentation_api_consistency/documentation_api_consistency_results.json`。

## 2. 本轮最初发现的问题

| 问题 | 数量 | 说明 |
|---|---:|---|
| STALE_API | 1 个 README / 3 处漂移 | Platform README 使用已删除的 `signal_mspm0g3507_dac_context_t`、旧 5 参数 DAC Bind 和不存在的 `SignalDAC_WriteVoltage` |
| MISSING_PLATFORM_LINK | 32 | 高风险模块/Platform README 均缺少统一固定章节；部分虽有零散说明，但不能稳定完成双向导航 |
| MISSING_EXAMPLE | 3 | GPIO、ADC Timer Trigger、ADC Continuous 没有对应的真实最小 `.c` Build 目标 |
| BUILD_FAIL | 0（正式结果） | 新 GPIO Example 首轮暴露生成宏名错误，读取真实 `ti_msp_dl_config.h` 后修正，最终结果为 0 |
| UNKNOWN | 0 | OPA/GPAMP 明确属于既有 `API_GAP`，没有用 UNKNOWN 或伪示例掩盖 |

## 3. 当前一致性审计表

状态只使用：`PASS`、`STALE_API`、`MISSING_PLATFORM_LINK`、`MISSING_EXAMPLE`、`BUILD_FAIL`、`UNKNOWN`。

| Module/Platform | README API 与 .h 一致 | Platform 链接完整 | Minimal Example | Build 验证 | 状态 |
|---|---:|---:|---|---|---|
| BSP ADC | 是（旧兼容） | 新工程单次读取 Direct DriverLib | `adc_basic_minimum` | PASS | PASS |
| Button | 是 | GPIO ReadActive | PC callback test + GPIO Platform example | PC PASS / Platform PASS | PASS |
| Comparator | 是 | MSPM0G3507 Comparator + Capture | `timer_capture_minimum` | PASS | PASS |
| BSP DAC | 是 | MSPM0G3507 DAC Bind/Write | `dac_dc_minimum` | PASS | PASS |
| BSP DMA | 是 | MSPM0G3507 DMA Bind | ADC/DAC DMA examples | PASS | PASS |
| BSP GPIO | 是 | MSPM0G3507 GPIO Bind | `gpio_minimum` | PASS | PASS |
| BSP GPAMP | 是 | 明确 `API_GAP`，无伪 Platform | N/A（Gap） | PC compile PASS | PASS |
| Latching Button Switch | 是 | GPIO ReadActive | PC callback test + GPIO Platform example | PC PASS / Platform PASS | PASS |
| Matrix Keypad 4×4 | 是 | MSPM0G3507 Keypad callbacks | PC callback test + GPIO Platform compile | PC PASS / Platform PASS | PASS |
| BSP OPA | 是 | 明确 `API_GAP`，无伪 Platform | N/A（Gap） | PC compile PASS | PASS |
| TFT ILI9341 | 是 | MSPM0G3507 TFT Platform | `tft_ili9341_lp_mspm0g3507` | PASS | PASS |
| BSP Timer | 是 | MSPM0G3507 Timer Bind | `adc_timer_trigger_minimum` | PASS | PASS |
| BSP UART | 是 | MSPM0G3507 UART Bind | `uart_minimum` | PASS | PASS |
| ADC Basic | 是（旧兼容） | 新工程单次读取 Direct DriverLib | `adc_basic_minimum` | PASS | PASS |
| ADC Continuous | 是 | callback 明确为业务消费者 | `adc_continuous_minimum` | PASS | PASS |
| ADC DMA | 是 | 模块自身落到 Timer/Event/ADC/DMA | `adc_dma_minimum` | PASS | PASS |
| ADC Dual Sync | 是 | 纯软件拆分 + Dual ADC Platform 双向说明 | Phase Meter Application | Round 1 PASS | PASS |
| ADC Timer Trigger | 是 | Timer Bind + ADC Enable/Disable | `adc_timer_trigger_minimum` | PASS | PASS |
| Timer Capture | 是 | MSPM0G3507 Capture Platform | `timer_capture_minimum` | PASS | PASS |
| DAC DC | 是（Deprecated for new apps） | `DL_DAC12_output12` | `dac_dc_minimum` 同步代码块 | PASS | PASS |
| DAC DMA | 是 | DAC DMA Platform Start/Stop | `dac_dma_minimum` | PASS | PASS |
| Comparator Threshold | 是 | MSPM0G3507 Comparator | `timer_capture_minimum` | PASS | PASS |
| Comparator Zero Cross | 是 | Comparator + Capture Platform | `timer_capture_minimum` | PASS | PASS |
| GPAMP Buffer | 是 | 明确继承 `API_GAP` | N/A（Gap） | PC compile PASS | PASS |
| GPAMP Gain | 是 | 明确继承 `API_GAP` | N/A（Gap） | PC compile PASS | PASS |
| OPA Buffer | 是 | 明确继承 `API_GAP` | N/A（Gap） | PC compile PASS | PASS |
| OPA Inverting | 是 | 明确继承 `API_GAP` | N/A（Gap） | PC compile PASS | PASS |
| OPA Non-inverting PGA | 是 | 明确继承 `API_GAP` | N/A（Gap） | PC compile PASS | PASS |
| DAC DMA Platform Adapter | 是 | 唯一父目录 `.c/.h` + 双向链接 | `dac_dma_minimum` | PASS | PASS |
| Dual ADC Platform Adapter | 是 | 唯一父目录 `.c/.h` + 双向链接 | Phase Meter Application | Round 1 PASS | PASS |
| Integration Glue | 是 | Not Applicable（纯软件） | Round 1 Applications | 8/8 PASS | PASS |
| MSPM0G3507 Platform Adapter | 41/41 旧 API 可定位 | 简单 binding 兼容；Capture/TFT 专用实现保留 | 4 direct + 6 complex examples | 10/10 PASS | PASS |

## 4. 当前剩余数量

| 状态 | 剩余 |
|---|---:|
| STALE_API | 0 |
| MISSING_PLATFORM_LINK | 0 |
| MISSING_EXAMPLE | 0 |
| BUILD_FAIL | 0 |
| UNKNOWN | 0 |

这里的 0 只针对本次 README/API/Platform 一致性范围。OPA/GPAMP 的 7 个集成 `API_GAP` 仍保留在 `MODULE_INTEGRATION_GAPS.md`，没有被文档 PASS 偷换为硬件 READY。

## 5. 防漂移规则

1. 修改 public `.h` 后必须运行 `tools/build_platform_closure.ps1`；
2. `【COMPILE-VERIFIED EXAMPLE】` 必须来自真实 `.c`，带 source marker，并进入 full-link 构建；
3. `【ILLUSTRATIVE SNIPPET】` 只能解释局部概念，不声称可独立编译；
4. 模块依赖硬件时，README 必须保留 `Hardware / Platform Binding` 固定章节；
5. 生成宏只从实际 `ti_msp_dl_config.h` 读取，不从 README 猜；
6. 未上板验证不得写 `BOARD_VERIFIED`。
