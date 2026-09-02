# 25_calibration

提供两点 ADC 增益/零偏校准和双通道固定延迟校准，所有物理量均明确使用 V、Hz、deg、s。

- `ADC_GAIN_OFFSET_CALIBRATION`：输入两组测量/真值电压与 `voltage_samples[]`，输出 `calibrated_samples[]`。
- `CHANNEL_DELAY_CALIBRATION`：输入实测/期望相位及 `frequency_hz`，输出补偿后 `phase_deg` 和 `delay_s`。

它只计算和应用校准系数，不改变既有前端标定含义，也不自行写入 Flash。
