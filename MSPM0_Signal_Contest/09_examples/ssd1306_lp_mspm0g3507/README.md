# SSD1306 LP-MSPM0G3507 SysConfig Profile

用于 `12_external_devices/display/ssd1306/README_MINIMAL_EXAMPLE.c` 的独立复制构建基线：I2C Controller 100 kHz，PB2=SCL、PB3=SDA。接屏幕前确认这两个 Pin 未被目标应用占用，且 OLED 模块供电/上拉均为 3.3 V。

该目录只提供 SysConfig 资源基线；正式驱动唯一源码仍位于 `12_external_devices/display/ssd1306/`。
