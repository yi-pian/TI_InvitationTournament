# 24_A 模块复制记录

全部文件于 2026-08-18 从 `MSPM0_Signal_Contest` 集成库复制，库内正式 `.c/.h` 未修改。

| 模块 | 集成库目录 | 复制内容 |
|---|---|---|
| AD9850 | `12_external_devices/dds/ad9850` | core + MSPM0 platform `.c/.h` |
| ADC DMA | `02_acquisition/adc_dma` | `signal_adc_dma.c/.h` |
| 4x4 键盘 | `01_bsp/matrix_keypad_4x4` | `signal_matrix_keypad_4x4.c/.h` |
| 稳健 Vpp | `05_precision/robust_peak_to_peak` | `.c/.h` |
| 压摆率 | `05_precision/slew_rate_measurement` | `.c/.h` |
| 静态功耗 | `05_precision/static_power_measurement` | `.c/.h` |
| VCA820 | `05_precision/vca820_gain_control` | `.c/.h` |
| ST7789 | `12_external_devices/display/st7789` | core、MSPM0 platform、font `.c/.h/.inc` |
| 公共依赖 | `01_bsp/common`、`03_measurement/common` | `signal_status.h`、`signal_types.h`、`signal_algorithm_status.h` |
