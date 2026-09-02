# Direct DriverLib / Complex Module Minimum Example Build Matrix

验证日期：2026-08-10  
命令：`tools/build_platform_closure.ps1`  
流程：Documentation/API check → SysConfig generate → 全部 translation units compile (`-Wall -Werror`) → final link。

| Example | Profile | SysConfig | Compile | Link | Flash B | SRAM B（含 512 B stack） | Board |
|---|---|---|---|---|---:|---:|---|
| DAC fixed code（Direct） | PROFILE_07_BASIC_IO | PASS | PASS | PASS | 1480 | 514 | NOT_RUN |
| ADC single result（Direct） | PROFILE_07_BASIC_IO | PASS | PASS | PASS | 1232 | 514 | NOT_RUN |
| UART blocking TX（Direct） | PROFILE_07_BASIC_IO | PASS | PASS | PASS | 1312 | 512 | NOT_RUN |
| GPIO set（Direct） | PROFILE_07_BASIC_IO | PASS | PASS | PASS | 1064 | 512 | NOT_RUN |
| ADC Timer Trigger | PROFILE_01_ADC_CAPTURE | PASS | PASS | PASS | 2936 | 520 | NOT_RUN |
| ADC Continuous | PROFILE_07_BASIC_IO | PASS | PASS | PASS | 1400 | 524 | NOT_RUN |
| ADC DMA | PROFILE_01_ADC_CAPTURE | PASS | PASS | PASS | 2664 | 668 | NOT_RUN |
| DAC DMA | PROFILE_03_DAC_GENERATOR | PASS | PASS | PASS | 2224 | 683 | NOT_RUN |
| Comparator + Timer Capture | PROFILE_05_FREQUENCY | PASS | PASS | PASS | 3576 | 776 | NOT_RUN |
| TFT ILI9341 | TFT example syscfg | PASS | PASS | PASS | 21064 | 637 | NOT_RUN |

结论：10/10 `BUILD_VERIFIED`。机器可读结果：`10_tests/platform_closure/build/platform_closure_build_results.json`。

四个简单示例已不链接 BSP/mega platform。与本轮修改前同一脚本结果相比，Flash 分别减少：DAC 968 B、ADC 808 B、UART 464 B、GPIO 296 B，合计由 7624 B 降为 5088 B（减少 2536 B，约 33.3%）。复杂模块示例的 source list 与结果保持原路径。

SysConfig 输出包含 ADC conversion-rate、DMA full-channel、Timer/Capture retention 等 `info` 提示，没有 error；这些提示不等于硬件行为已经验证。所有 Board 状态保持 `NOT_RUN`。
