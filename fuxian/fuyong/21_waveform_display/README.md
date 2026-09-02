# 21_waveform_display

仅完成双通道 ADC 电压波形的自动居中/缩放、按屏宽降采样、TFT 时域绘制和 XY 绘制。两路共用本帧全局 min/max，既保留两路相对偏置，又避免 1.65 V ADC 偏置把交流波形挤到屏幕上半部。

## COPY

`WAVEFORM_PREPARE` 输入双路 ADC code，输出双路物理 V；`WAVEFORM_DRAW` 输入两路 V 和 `tft`，调用 `Waveform_Draw(false)` 显示时域，调用 `Waveform_Draw(true)` 显示 XY。无需 FFT、滤波或额外数组。
