# MODULE CARD: adc_dma

| 项目 | 内容 |
|---|---|
| 作用 | Timer/Event 触发 ADC，DMA 完整搬运 N 点到调用者 RAM |
| 输入 | 目标 Fs、Timer 时钟、N、`uint16_t buffer[N]`；通道由 SysConfig 决定 |
| 输出 | 原始帧、运行状态、配置触发率 |
| 依赖 | SysConfig Timer/Event/ADC12/DMA；`signal_status.h` |
| RAM | `2N` bytes + 常数状态；无动态分配 |
| 重复启动 | 已在 TMP6131 上连续完成 500 帧（5×100） |
| 当前状态 | `MODULE_STATUS_BOARD_VERIFIED`（2026-08-07） |
| 未验证 | PA25 动态输入；100/200/500 kSPS 动态 samples/cycle；完整比赛链 |
| 移除 | 移除源文件和上层调用，再删 SysConfig 对应资源 |

`SignalADC_GetConfiguredTriggerRate()` 是整数 Timer 配置推导值，不是外部实测采样率。
