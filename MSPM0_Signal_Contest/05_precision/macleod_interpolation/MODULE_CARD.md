# MODULE CARD: macleod_interpolation

| 项目 | 内容 |
|---|---|
| 路径 | `MSPM0_Signal_Contest/05_precision/macleod_interpolation` |
| 类型 | 正式 Precision Primitive；SOURCE_LOST clean reimplementation |
| 输入/输出 | 矩形窗三复 FFT bin、Fs、N → fractional bin / Hz |
| 复杂度 | O(1)，含平方根 |
| 主 API | `SignalMacleod_Process` |
| 当前验证 | `BUILD_VERIFIED`，Board `NOT_RUN`；见 `VERIFICATION.yaml` |
