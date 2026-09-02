# MODULE CARD: dc_measure

| 项目 | 内容 |
|---|---|
| 路径 | `MSPM0_Signal_Contest/03_measurement/dc_measure` |
| 类型 | 正式 Measurement helper；复用 Mean 的 clean reimplementation |
| 输入/输出 | raw+linear calibration 或 voltage[N] → DC voltage |
| 复杂度 | O(N)，O(1) RAM |
| 主 API | `SignalDCMeasure_FromRawLinear`、`SignalDCMeasure_FromVoltage` |
| 当前验证 | `BUILD_VERIFIED`，Board `NOT_RUN`；见 `VERIFICATION.yaml` |
