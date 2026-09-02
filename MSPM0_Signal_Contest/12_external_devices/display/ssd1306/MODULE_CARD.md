# SSD1306 0.96-inch OLED Module Card

| 项目 | 内容 |
|---|---|
| 明确模块 | Wanghong `WH-X096-2864KSWEG01-A4` |
| 控制器 / 分辨率 | SSD1306 / 128×64 单色 |
| 接口 | 4-pin I2C：GND、VCC、SCL、SDA |
| 7-bit 地址 | 默认 `0x3C`；移动板上地址电阻后可为 `0x3D` |
| 正式源码 | `ssd1306.c/.h` + `ssd1306_font_6x8.inc`；兼容扩展 API 含矩形/位图/尺寸查询 |
| MSPM0 适配层 | `ssd1306_mspm0_i2c.c/.h` + `ssd1306_mspm0g3507.c/.h` + `00_common/mspm0_blocking_bus.c/.h` |
| RAM | 1024 B framebuffer，由应用静态提供；驱动对象不含显存 |
| 状态 | `DOC_VERIFIED`、`CODE_COMPILE_VERIFIED`、`COPY_READY`；尚未实板验证 |
| 资料来源 | 本地厂商规格 `0.96oled/WH-096-4pin-I2C-SSD1306.pdf`，Rev 1.0, 2024-05-08 |
| 重要限制 | 当前传输为阻塞轮询；屏幕无外露 RESET，异常噪声后需重新初始化 |
