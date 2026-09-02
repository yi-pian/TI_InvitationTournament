# DDS Generator

> **REFERENCE ASSEMBLY EXAMPLE**：演示 DDS 与 DAC DMA 连接，不是赛题解决方案。

## 本 Application 的实现层级

| 功能 | 类型 | 当前实现 |
|---|---|---|
| 固定 DAC code | A Direct DriverLib | 本工程不使用；若只要固定电压，应改走 SysConfig + `DL_DAC12_output12()` |
| 连续 DAC 数据流 | B Complex Hardware Module | DAC DMA + MSPM0G3507 DAC Platform |
| 频率/相位与波表填充 | C Algorithm Module | Sine Wave Table + DDS |
| 完整组合 | E Application Reference | 本工程仅作参考，仍链接正式唯一源码 |

```text
Sine WaveTable → DDS phase accumulator → uint16_t DMA block
               → DAC DMA wrapper → DAC platform → DMA1/TIMG6/Event3 → DAC0/PA15
```

- Status：`BUILD_VERIFIED`；Board=`PENDING_BOARD`。
- Config：`signal_config.h`。
- Hardware profile：P03。
- Projectspec：`ticlang/dds_generator_round1_LP_MSPM0G3507_nortos_ticlang.projectspec`。
- 重点学习：table、phase step、fill block、platform callback、repeat DMA。

检查 `offset±amplitude` 不越 DAC 量程，并检查 repeat block 周期边界闭合。
