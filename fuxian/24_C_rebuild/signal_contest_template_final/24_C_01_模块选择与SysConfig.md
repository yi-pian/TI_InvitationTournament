# 24_C-01 模块选择与 SysConfig

按原功能拆成双路同步采集、Timer Capture 测频占空比、动态采样率、FFT/谐波、周期/突发识别、ST7789 波形与文字六步。模块全部从集成库复制，清单见 `COPIED_MODULES.md`。

SysConfig 按各 README 和原 24_C 配置：ADC0 CH2/PA25 + DMA_CH0，ADC1 CH2/PA17 + DMA_CH1；TIMG0 发布同一事件同步触发两 ADC；TIMG6/PB2 做 capture；ST7789 使用 SPI1 PB9/PB8、CS PB6、DC PB15、BL PB12。与 README 没有接口差异，采样率由 main 在帧边界动态修改。

自写变量逐行解释：`capture_buffer` 是 DMA 正在写的块号；`previous_buffer` 是上一完整块；`pending_sample_rate_hz` 保存下帧准备采用的 Fs；三个变量保证处理旧数据时不覆盖正在采集的数据。
