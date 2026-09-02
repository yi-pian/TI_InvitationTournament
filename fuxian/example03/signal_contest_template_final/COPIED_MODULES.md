# example03 复制模块

来源均为 `MSPM0_Signal_Contest` 冻结模块，复制到本工程 `modules/`，未修改模块 `.c/.h`。

| 文件 | 来源 | 用途 |
|---|---|---|
| `signal_dual_adc_mspm0g3507.c/.h` | `02_acquisition/adc_dual_sync/` | 双 ADC 连续 DMA 块 |
| `signal_adc_pingpong_dma.c/.h` | `02_acquisition/adc_pingpong_dma/` | CPU 消费块状态 |
| `signal_adc_ring_buffer.c/.h` | `02_acquisition/adc_ring_buffer/` | 事件样本 FIFO |
| `signal_matrix_keypad_4x4.c/.h` | `01_bsp/matrix_keypad_4x4/` | 阈值/页面键控 |
| `signal_tft_st7789.c/.h`、`signal_tft_st7789_mspm0g3507.c/.h` | `12_external_devices/display/st7789/` | ST7789 波形页 |
| `signal_status.h`、算法依赖头 | 模块公共依赖 | 状态码和类型 |

状态：源码层检查完成，`BOARD NOT_RUN`。
