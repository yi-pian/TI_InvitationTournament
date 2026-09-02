# MODULE CARD: frequency_response_correction

| 项目 | 内容 |
|---|---|
| 路径 | `MSPM0_Signal_Contest/05_precision/frequency_response_correction` |
| 类型 | 正式 Precision/Calibration Primitive；SOURCE_LOST clean reimplementation |
| 输入/输出 | correction LUT + measured gain/phase → corrected gain/phase |
| 复杂度 | O(K)，O(1) RAM |
| 主 API | `SignalFrequencyResponseCorrection_Process` |
| 当前验证 | `BUILD_VERIFIED`，Board `NOT_RUN`；见 `VERIFICATION.yaml` |
