# Copied Modules

比赛工程复制模块后，只做这份简单来源记录；不需要维护复杂依赖系统。

| 模块 | 原始路径 | 复制日期 | 本题修改 |
|---|---|---|---|
| 单 ADC/DMA | `MSPM0_Signal_Contest/02_acquisition/adc_dma/` | 2026-08-20 | 仅应用层 Fs=500 kS/s、N=1024；模块原文未改 |
| 上升过零 + 插值 | `03_measurement/frequency_zero_cross/`、`05_precision/zero_cross_interpolation/` | 2026-08-20 | 仅应用层选择与 FFT 目标频率最接近的过零周期；模块原文未改 |
| ST7789 + 8x16 字库 | `MSPM0_Signal_Contest/12_external_devices/display/st7789/` | 2026-08-20 | 仅调用旋转 90°与 8x16 API；模块原文未改 |
| 4x4 矩阵键盘 | `MSPM0_Signal_Contest/01_bsp/matrix_keypad_4x4/` | 2026-08-20 | 仅应用层接受数字 1-5；模块原文未改 |
| 公共状态头 | `MSPM0_Signal_Contest/01_bsp/common/signal_status.h` | 2026-08-20 | 原样复制 |

复制后使用 SHA-256 与来源逐文件核对；核对结果和完整哈希见总步骤文档。
