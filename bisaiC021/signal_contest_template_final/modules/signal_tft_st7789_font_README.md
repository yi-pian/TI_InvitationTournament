# ST7789 字库模块

本工程同步 `MSPM0_Signal_Contest/12_external_devices/display/st7789` 的 ST7789 字库模块。
它使用 `TFT_ST7789_DrawMonoBitmap()` 输出 ASCII 点阵，提供 6×12、8×16、12×24、
16×32 四种字号；`example01` 的标题、参数标签和数值统一使用 `TFT_ST7789_FONT_8X16`。

## 内置内容

- 可打印 ASCII：`0x20` 到 `0x7E`；
- 字号：6×12、8×16、12×24、16×32；
- 示例 16×16 汉字点阵：`电`、`子` 两个；
- 完整中文库不在本资源中。需要新汉字时，将对应 16×16 点阵传给
  `TFT_ST7789_DrawMonoBitmap()`。

## SysConfig

无新增 SysConfig 配置。先按 ST7789 模块 README 配置 SPI、DC、CS 和背光，并在
`SignalTFTST7789_MSPM0_Init()` 成功后调用本模块。

## 最小代码示例

```c
#include "signal_tft_st7789_font.h"

(void)TFT_ST7789_DrawString(&g_tft, 8, 8, "FREQ=1000", 
    TFT_ST7789_FONT_8X16, TFT_ST7789_CYAN, TFT_ST7789_BLACK, false, false);
(void)TFT_ST7789_DrawFloat(&g_tft, 8, 30, 10000.08f, 2U,
    TFT_ST7789_FONT_8X16, TFT_ST7789_WHITE, TFT_ST7789_BLACK, false);
(void)TFT_ST7789_DrawMonoBitmap(&g_tft, 8, 50, 16U, 16U,
    TFT_ST7789_GLYPH_CN_DIAN_16X16, TFT_ST7789_GLYPH_16X16_BYTES,
    TFT_ST7789_YELLOW, TFT_ST7789_BLACK, false);
```

参数 `transparent_background=false` 会同时绘制前景和背景；设为 `true` 时仅绘制
字模中为 1 的像素，适合叠加到已有图形。
