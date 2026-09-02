# MODULE CARD: czt

| 项目 | 内容 |
|---|---|
| 路径 | `MSPM0_Signal_Contest/05_precision/czt` |
| 类型 | 正式 DSP Primitive；unit-circle direct clean reimplementation |
| 输入/输出 | real[N] + start/step/M → complex[M] |
| 复杂度 | O(NM)，O(1) 额外 RAM；非 Bluestein |
| 主 API | `SignalCZT_UnitCircleRealDirect` |
| 当前验证 | `BUILD_VERIFIED`，Board `NOT_RUN`；见 `VERIFICATION.yaml` |
