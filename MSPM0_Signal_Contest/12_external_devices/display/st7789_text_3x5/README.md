# ST7789 竞赛文字叠加模块

## 作用

正式 ST7789 驱动只提供像素、线、矩形和位图接口，没有文字接口。这个独立的应用辅助模块提供 5x7 大写字母和数字，用于比赛屏幕显示参数；它不修改 `signal_tft_st7789.c/.h`。

## 复制

复制 `signal_tft_st7789_text.c/.h`，并确保同一工程已经复制 ST7789 核心模块。它不需要额外 SysConfig；SPI、DC、CS、背光仍按 ST7789 README 配置。

## 最小调用

```c
#include "signal_tft_st7789_text.h"
SignalTFTST7789Text_DrawString(&g_tft, 4, 4, "FREQ", 2,
    TFT_ST7789_WHITE, TFT_ST7789_BLACK);
SignalTFTST7789Text_DrawUint(&g_tft, 64, 4, 10000U, 5U, 2U,
    TFT_ST7789_YELLOW, TFT_ST7789_BLACK);
```

这是显示层辅助模块，不负责页面刷新、参数含义或键盘扫描。数值单位和动态刷新策略由应用层决定。
