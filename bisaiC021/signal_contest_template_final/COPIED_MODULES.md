# example01 复制模块

## fuyong 教学复用索引

组合层对应 `04_dual_adc_dma`、`40_dual_channel_measurement`、`70_keypad_usage`、`80_tft_usage` 的 COPY 区。该索引不新增复制模块，也不修改模块 `.c/.h`；双 trace、按键队列和页面状态机仍为本题专属代码。

来源均为 `MSPM0_Signal_Contest` 冻结模块，复制到本工程 `modules/`。应用层按题目组合；ST7789 核心、MSPM0 适配层和 8×16 字库均同步自最新模块。

| 文件 | 来源 | 用途 |
|---|---|---|
| `signal_dual_adc_mspm0g3507.c/.h` | `02_acquisition/adc_dual_sync/` | Timer/Event 双 ADC DMA |
| `signal_dual_adc_phase.c/.h` | `05_precision/dual_adc_phase_measurement/` | 双 ADC 动态中点、滞回过零和相位平均 |
| `signal_matrix_keypad_4x4.c/.h` | `01_bsp/matrix_keypad_4x4/` | 扫描、消抖、鬼键过滤 |
| `signal_tft_st7789.c/.h` | `12_external_devices/display/st7789/` | ST7789 绘图 |
| `signal_tft_st7789_mspm0g3507.c/.h` | 同上 | MSPM0 SPI/GPIO 适配 |
| `signal_tft_st7789_font.c/.h` | 同上 | ASCII 6×12/8×16/12×24/16×32 字库 |
| `signal_tft_st7789_font_data.inc` | 同上 | 字库点阵数据 |
| `signal_status.h`、算法依赖头 | 各模块公共依赖 | 状态码和类型 |

状态：源码层检查和 Build 已完成；用户已实机确认 TFT 波形与键盘响应，`BOARD PARTIALLY VERIFIED`。
