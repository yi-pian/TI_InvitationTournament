# 80_tft_usage

## 推荐复制函数

初始化复制 `InitTFTDemo()`，固定内容复制 `DrawStaticText()`，实时数值复制 `DisplayVariable() + UpdateLiveValue()`，页面选择复制 `DrawPage()`。所有函数使用 `frequency_hz`（Hz）与 `current_page`。

## 1. 这个工程干什么

演示 ST7789 初始化、固定文字、变量显示、局部刷新及 `current_page` 两页状态。

## 2. 这个工程包含哪些子功能

| 我要做什么 | COPY 区域 |
|---|---|
| 固定文字 | `TFT_TEXT` |
| 显示变量 | `TFT_VARIABLE` |
| 实时局部刷新 | `TFT_LIVE_VALUE` |
| 两页 UI | `TFT_TWO_PAGES` |

## 3. 输入

`frequency_hz` 为显示变量，`current_page` 为 UI 页号。

## 4. 输出

ST7789 屏幕内容。

## 5. 公共数据链

`SysConfig SPI/GPIO → SignalTFTST7789_MSPM0_Init → DrawString/DrawInt32`。

## 6. 功能与 COPY 区对应表

见第 2 节，各块可单独复制。

## 7. 使用的模块

`signal_tft_st7789`、`signal_tft_st7789_mspm0g3507`、`signal_tft_st7789_font`；API 来自真实头文件和 restored example04。

独立复制文件清单：`signal_status.h`、`signal_tft_st7789.c/.h`、`signal_tft_st7789_mspm0g3507.c/.h`、`signal_tft_st7789_font.c/.h`、`signal_tft_st7789_font_data.inc`。缺少字库 `.inc` 或平台层 `signal_tft_st7789_mspm0g3507.c/.h` 都不能链接或显示。

## 8. SysConfig / 引脚

复制 restored example04 的 TFT/SPI/GPIO 配置。

## 9. main.c 流程

初始化后显示固定标题、数值和当前页。

## 10. 每个 COPY 区说明

`TFT_LIVE_VALUE` 先填黑色矩形，防止新数值位数变少时遗留字符。

## 11. 如何复制到新工程

复制相关 TFT 模块/字库、所需 COPY 区和相同 SysConfig。

## 12. 可调参数

坐标、颜色、字体、旋转、刷新频率。

## 13. 常见错误

SPI/GPIO 宏名必须来自同一 SysConfig；不可手改生成的 `ti_msp_dl_config.*`。

## 14. 本工程没有做什么

不采集 ADC、不画波形、不实现按键状态机。

## 15. Build 状态

SysConfig 1.28 Generate、TI Arm Clang 5.1 Compile/Link 已通过；实板 `NOT_RUN`。
