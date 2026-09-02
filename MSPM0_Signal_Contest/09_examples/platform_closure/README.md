# MSPM0G3507 Direct DriverLib / Complex Module Minimum Examples

这些工程同时验证两条比赛路径：简单硬件动作直接使用 SysConfig + DriverLib；复杂流程使用正式模块。它们不声称已经上板通过。

| 目录 | 闭环 | Profile |
|---|---|---|
| `dac_dc_minimum` | `SYSCFG_DL_init` → `DL_DAC12_output12` → DAC0/PA15 | PROFILE_07_BASIC_IO |
| `adc_basic_minimum` | software trigger → `DL_ADC12_getMemResult` → ADC0/PA25 | PROFILE_07_BASIC_IO |
| `uart_minimum` | `DL_UART_Main_transmitDataBlocking` → UART0/PA10/PA11 | PROFILE_07_BASIC_IO |
| `gpio_minimum` | `DL_GPIO_setPins` → GPIO output PA12 | PROFILE_07_BASIC_IO |
| `adc_timer_trigger_minimum` | ADC Timer Trigger callbacks → MSPM0G3507 ADC/Timer adapter | PROFILE_01_ADC_CAPTURE |
| `adc_continuous_minimum` | ADC Continuous 业务 callback 签名与状态机 | PROFILE_07_BASIC_IO（模块本身无硬件） |
| `adc_dma_minimum` | 正式 ADC DMA → Timer/Event/ADC0/DMA0 | PROFILE_01_ADC_CAPTURE |
| `dac_dma_minimum` | DAC DMA → 正式 DAC DMA platform → Timer/Event/DMA1/DAC0 | PROFILE_03_DAC_GENERATOR |
| `timer_capture_minimum` | Comparator → adapter → Event → capture platform → Timer Capture | PROFILE_05_FREQUENCY |
| `tft_ili9341_minimum` | ILI9341 → TFT platform → SPI/GPIO | TFT example syscfg |

统一验证命令：

```powershell
powershell.exe -NoProfile -ExecutionPolicy Bypass -File tools/build_platform_closure.ps1
```

结果写入 `10_tests/platform_closure/build/platform_closure_build_results.json`。

构建矩阵见 [PLATFORM_MINIMAL_EXAMPLE_BUILD_MATRIX.md](../../00_docs/PLATFORM_MINIMAL_EXAMPLE_BUILD_MATRIX.md)，README/API 审计见 [DOCUMENTATION_API_CONSISTENCY_AUDIT.md](../../00_docs/DOCUMENTATION_API_CONSISTENCY_AUDIT.md)。
