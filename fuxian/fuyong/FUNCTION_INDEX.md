# fuyong 教学函数索引

本索引仅列新增主题的可复制 `main.c static` 函数；数组类型和单位以 `SINGLE_FUNCTION_INTERFACE_STANDARD.md` 为准。

| 工程 / COPY 区 | 函数 | 输入 | 输出 | 依赖 | 单帧唯一 |
|---|---|---|---|---|---|
| 15 / `MOVING_AVERAGE` | `ApplyMovingAverage` | `voltage_samples[]` V | `filtered_samples[]` V | 无 | 是 |
| 15 / `MEDIAN_HAMPEL` | `ApplyMedianOrHampel` | `voltage_samples[]` V、模式 | `filtered_samples[]` V、`outlier_count` | Median/Hampel 模块、`workspace[]` | 是 |
| 15 / `FIR_IIR` | `ApplyFIRorIIR` | `voltage_samples[]` V、模式 | `filtered_samples[]` V | CMSIS-DSP、状态数组 | 是 |
| 21 / `WAVEFORM_PREPARE` | `Waveform_ConvertToVoltage` | 两路 ADC code | 两路 V 数组 | `signal_config.h` | 是 |
| 21 / `WAVEFORM_DRAW` | `Waveform_Draw` | 两路 V、`tft`、XY 选择 | TFT 曲线 | ST7789 | 是（绘图） |
| 22 / `SPECTRUM_DISPLAY` | `Spectrum_Draw` | `fft_magnitude[]`、采样率 | TFT dB 柱图/峰值 | ST7789、math | 是（显示；不 FFT） |
| 23 / `TRIGGER_CAPTURE` | `Trigger_Capture` | ADC code、阈值、迟滞 | 触发帧、触发点 | `signal_trigger_capture` | 是 |
| 24 / `AUTO_RANGE` | `AutoRange_Update` | `voltage_samples[]` V | 显示半量程、建议增益档 | math | 是 |
| 25 / `ADC_GAIN_OFFSET_CALIBRATION` | `Calibration_ApplyADC` | 两点测量/真值、V 数组 | 校准后 V 数组 | ADC 校准模块 | 是 |
| 25 / `CHANNEL_DELAY_CALIBRATION` | `Calibration_ApplyDelay` | 实测/期望相位、Hz | `phase_deg`、`delay_s` | 延迟校准模块 | 否（系数可跨帧保留） |
