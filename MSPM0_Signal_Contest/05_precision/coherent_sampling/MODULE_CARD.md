# MODULE CARD: coherent_sampling

| 项目 | 内容 |
|---|---|
| 路径 | `MSPM0_Signal_Contest/05_precision/coherent_sampling` |
| 类型 | 正式 Precision Primitive；SOURCE_LOST clean reimplementation |
| 输入/输出 | target f、Fs、N、J range → nearest coherent f / error |
| 复杂度 | O(J range)，O(1) RAM |
| 主 API | `SignalCoherentSampling_FindNearest` |
| 当前验证 | `BUILD_VERIFIED`，Board `NOT_RUN`；见 `VERIFICATION.yaml` |
