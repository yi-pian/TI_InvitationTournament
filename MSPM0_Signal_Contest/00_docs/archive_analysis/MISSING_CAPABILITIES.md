# Missing Capabilities

本表是能力缺口队列，不是“看到一点不方便就新建模块”的清单。

| 缺失能力 | 影响题型 | 当前替代方案 | 优先级 | 允许新增模块的条件 |
|---|---|---|---|---|
| 端到端 Native Q15 4096 点 Pipeline | 超长记录、高分辨窄带频谱 | 降低 N/带宽；N=512/1024 Q31；分段或 Lock-in | Medium | 真实赛题必须 4096 且现有 Recipe 无法满足 RAM/精度 |
| Board cycle/deadline profiler | 所有高 Fs 实时题 | 用 frame deadline 静态预算；功能逐项开启 | High before board | 有开发板和稳定计时基准后补正式 benchmark，不放入应用业务逻辑 |
| 双通道 skew 自动实板校准流程 | 高精度相位/群延迟 | 现有 ChannelDelayCalibration + 仪器手工标定 | High before phase contest | 真题相位精度受 skew 主导且仪器可提供同相信号 |
| 绝对幅值保真的 Capture Replay 映射 | 任意波复制/校准源 | 当前 AutoRange 重放只保形状；按 VREF 手工映射 | Medium | 题目明确要求绝对幅值/offset 重放 |
| 连续无缝双缓冲系统应用 | 长时流式频谱/记录 | 当前 block acquire/process；降低刷新率 | Medium | 真题不允许采集空窗且 CPU/ DMA 资源已验证 |

当前没有缺口阻塞核心 12 个集成应用的构建，也没有理由在本 Sprint 新建大批基础模块。
