# 外部显示设备

README 类型：`CATEGORY_INDEX`

- [`ssd1306/`](ssd1306/README.md)：旺泓 WH-X096-2864KSWEG01-A4，SSD1306 128×64 4-pin I2C，已有 MSPM0 正式驱动与 COPY_READY 最小例。
- [`st7789/`](st7789/README.md)：GMT024-01 / ST7789V2，240×320、16-bit RGB565、7PIN SPI；已有核心 `.c/.h`、MSPM0G3507 适配层和最小示例，状态为 `CODE_COMPILE_VERIFIED`、`COPY_READY`，尚未上板。
- ILI9341 已有正式唯一源码：[`../../01_bsp/tft_ili9341/README.md`](../../01_bsp/tft_ili9341/README.md)，不要复制到这里。

串口屏或其他控制器先走陌生器件流程。
