# ST7789 波形绘制模块

## 1. 作用

把一段 `float` 采样数据画入 ST7789 的指定矩形区域。周期信号可用抽点连线，点数远多于屏幕宽度或突发波形可用每列最小/最大包络。模块支持固定/自动纵轴、网格、零线、边框和仅清除绘图区。

## 2. 复制文件

从本目录复制 `signal_tft_waveform_st7789.c/.h`，同时按 ST7789 README 复制显示核心、MSPM0 平台、字库和 `signal_status.h`。所有文件放入工程 `modules/`，Refresh 后确认 `.c` 未 Exclude from Build。

## 3. SysConfig

本模块不新增外设，完全复用 ST7789 README 的配置：SPI1 controller、PB9 SCLK、PB8 MOSI、PB6 hardware CS、PB15 DC、PB12 BL。若工程换脚，只改 SysConfig 和 ST7789 平台层宏，不改本模块。

## 4. 最小代码

```c
#include "signal_tft_waveform_st7789.h"

signal_tft_waveform_st7789_result_t result;
const signal_tft_waveform_st7789_config_t plot = {
    .x = 40, .y = 18, .width = 272, .height = 118,
    .mode = SIGNAL_TFT_WAVEFORM_DECIMATE,
    .scale_mode = SIGNAL_TFT_WAVEFORM_FIXED_SCALE,
    .minimum_value = -2.75f, .maximum_value = 2.75f,
    .baseline_value = 0.0f,
    .waveform_color = TFT_ST7789_YELLOW,
    .background_color = TFT_ST7789_BLACK,
    .grid_color = TFT_ST7789_RGB565(55, 75, 85),
    .baseline_color = TFT_ST7789_CYAN,
    .horizontal_grid_divisions = 4,
    .vertical_grid_divisions = 5,
    .clear_background = true, .draw_grid = true,
    .draw_border = false, .draw_baseline = true
};

(void)SignalTFTWaveformST7789_Draw(
    &g_tft, samples, sample_count, &plot, &result);
```

静态界面初始化时单独调用一次 `TFT_ST7789_DrawRect` 画边框，连续刷新配置 `draw_border=false`。这样每帧只刷新绘图区，边框不会闪烁。突发波形把 `mode` 改为 `SIGNAL_TFT_WAVEFORM_MIN_MAX_ENVELOPE`。

## 5. main 自写边界

main 只决定 `samples/count`、显示范围和模式，不写坐标映射、列包络或折线循环。动态坐标轴如需自动缩放只把 `scale_mode` 改为 `SIGNAL_TFT_WAVEFORM_AUTO_SCALE`。
