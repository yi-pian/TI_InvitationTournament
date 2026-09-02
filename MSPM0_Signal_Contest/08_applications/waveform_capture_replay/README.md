# Waveform Capture Replay

> **REFERENCE ASSEMBLY EXAMPLE**：演示 Trigger/Ring/Segment 到 DAC DMA 的连接，不是赛题解决方案。

## 本 Application 的实现层级

| 功能 | 类型 | 当前实现 |
|---|---|---|
| ADC 捕获与 DAC 连续重放 | B Complex Hardware Module | ADC DMA + DAC DMA + Platform |
| 历史/触发/周期段/重采样 | C Algorithm/Buffer Module | Ring Buffer、Trigger、Period Segment、Resample/Normalize |
| 简单固定 DAC | A Direct DriverLib | 本工程不适用；只输出固定 code 时不应使用重放链 |
| 完整组合 | E Application Reference | 参考 capture buffer 到 replay buffer 的所有权与顺序 |

```text
ADC DMA → Ring Buffer → Trigger Find → one-period Segment
        → linear Resample + AutoRange Normalize → DAC DMA replay
```

- Status：`BUILD_VERIFIED`；PC Glue PASS；Board=`PENDING_BOARD`。
- Config：`signal_config.h`。
- Hardware profile：P04。
- Projectspec：`ticlang/waveform_capture_replay_final_LP_MSPM0G3507_nortos_ticlang.projectspec`。
- Resource baseline：Flash 7,456 B，SRAM 18,173 B。
- 重点学习：Ring storage=N+1、同向 trigger、period segment、replay update rate。

AutoRange 改变绝对 amplitude/offset，只保留归一化形状；非周期或不稳定波形可能无法提取周期。
