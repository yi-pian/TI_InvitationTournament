# GMT024-01 / ST7789 Module Card

| 项目 | 内容 |
|---|---|
| 类别 | External display / SPI TFT |
| 资料标注 | GMT024-01，ST7789V2，2.4 inch，7PIN |
| 分辨率 / 像素 | 240×320，16-bit RGB565，MSB first（资料结论） |
| 主接口 | 4-wire write SPI 角色：SCK、MOSI、CS、DC；另有 RST、BL 和待确认电源/脚位 |
| 控制命令 | `0x2A` 列地址、`0x2B` 行地址、`0x2C` Memory Write；完整初始化需精确资料确认 |
| MSPM0 复用形态 | `signal_tft_st7789.c/.h` 核心 + `signal_tft_st7789_font.c/.h/.inc` 字库 + `signal_tft_st7789_mspm0g3507.c/.h` 平台绑定 + 最小示例 |
| 原始资料 | `2.4TFT_ST/2.4TFT8PIspi-GMT024-01.ino` 等旧 Arduino/STM32 工程 |
| 状态 | `CODE_COMPILE_VERIFIED`、`COPY_READY`；`BOARD_VERIFIED` 尚无 |
| 重要限制 | 旧工程的 STM32 GPIO、延时和初始化表不能直接用于 MSPM0G3507 |
