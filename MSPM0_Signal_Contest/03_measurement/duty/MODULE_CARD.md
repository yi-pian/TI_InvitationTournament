# MODULE CARD: duty

| 项目 | 内容 |
|---|---|
| 路径 | `MSPM0_Signal_Contest/03_measurement/duty` |
| 类型 | 正式测量 Primitive；SOURCE_LOST 后 clean reimplementation |
| 输入 | `float samples[N]`、真实 Fs、阈值/电平配置 |
| 输出 | duty ratio/%、period/frequency、high/low width、有效周期数 |
| 复杂度 | O(N) 时间、O(1) 额外 RAM、无动态分配 |
| SysConfig | 不需要 |
| 主 API | `SignalDuty_GetDefaultConfig`、`SignalDuty_Process` |
| 历史验证 | `BUILD_VERIFIED`，仅属于已丢失旧源码 |
| 当前验证 | 见 `VERIFICATION.yaml`，必须从 DRAFT 重新开始 |
| 板级验证 | `NOT_RUN` |
