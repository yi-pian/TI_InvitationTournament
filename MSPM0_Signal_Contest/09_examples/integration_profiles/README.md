# SysConfig 集成配置选择

这些目录不是新的外设模块，而是比赛当天可复制的资源组合基线。`.syscfg` 是唯一配置源；`10_tests/.../build` 下生成文件不能手改或提交为 source of truth。

| Profile | 选择场景 | 主要资源 | 当前证据 |
|---|---|---|---|
| PROFILE_01_ADC_CAPTURE | 单 ADC block capture + debug UART | ADC0.2 PA25, DMA0, TIMG0, Event1 | SysConfig/compile/link PASS |
| PROFILE_02_DUAL_ADC | 双 ADC 同步采集 | ADC0.2 PA25 + ADC1.2 PA17, DMA0/1, TIMG0, Event1/2 | SysConfig/compile/link PASS |
| PROFILE_03_DAC_GENERATOR | DAC 波表输出 | DAC0 PA15, DMA1, TIMG6, Event3 | SysConfig/compile/link PASS |
| PROFILE_04_ADC_DAC | ADC + DAC 独立链 | P01 + P03 | SysConfig/compile/link PASS |
| PROFILE_05_FREQUENCY | Comparator edge capture | COMP0 PA27, Event4, TIMG6 Capture | SysConfig/compile/link PASS |
| PROFILE_06_FULL_SIGNAL | 双 ADC + DAC + capture + UART | DMA0/1/2, TIMG0/6/7, Event1..4 | SysConfig/compile/link PASS |
| PROFILE_07_BASIC_IO | 单点 ADC + DAC DC + UART + GPIO | ADC0.2 PA25, DAC0 PA15, UART0, PA12 | SysConfig/compile/link PASS |
| PROFILE_08_ADC_FIFO_MAX | ADC FIFO 满吞吐率单帧采集 | ADC0.2 PA25, FIFO, 32-bit DMA0 | SysConfig/compile/link PASS |

复验：

```powershell
powershell -NoProfile -ExecutionPolicy Bypass -File .\tools\validate_peripheral_profiles.ps1
```

脚本为每套配置重新运行 CCS 21 自带 SysConfig 1.28，使用 TI Arm Clang 5.1.1.LTS 和 `-Wall -Werror` 编译生成文件并链接最小镜像。它不会烧写板卡，结果中的 board 一律保持 NOT_RUN。

复制到应用时保留实例名约定，尤其是 P01/P04 中 ADC_DMA 正式模块使用的 `SIGNAL_ADC`、`SIGNAL_ADC_DMA`、`SIGNAL_SAMPLE_TIMER`，以及 P08 使用的 `SIGNAL_ADC_FIFO`、`SIGNAL_ADC_FIFO_DMA`。P02/P06 的 A/B 命名需要应用 adapter，不应在 `signal_adc_dma.c` 中加入大量 profile 条件编译。
