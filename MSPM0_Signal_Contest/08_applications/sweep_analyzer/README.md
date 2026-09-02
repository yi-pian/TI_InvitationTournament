# Sweep Analyzer

> **REFERENCE ASSEMBLY EXAMPLE**：演示 ADC+DAC+Lock-in 扫描连接，不是赛题解决方案。

## 本 Application 的实现层级

| 功能 | 类型 | 当前实现 |
|---|---|---|
| 连续激励与采样 | B Complex Hardware Module | DAC DMA + ADC DMA + 两个平台绑定 |
| 扫频与幅相计算 | C Algorithm Module | DDS、Frequency Sweep、ADC To Voltage、Lock-In |
| 外部 DUT | 外部被测对象 | 不是 External Device Driver；只从模拟输入/输出连接 |
| 完整组合 | E Application Reference | 参考频点推进和 gain/phase 结果保存 |

```text
DDS → DAC DMA/PA15 → external DUT → PA25/ADC DMA
    → RawToVoltage → Lock-in amplitude/phase → sweep point result → next frequency
```

- Status：`BUILD_VERIFIED`；PC Glue PASS；Board=`PENDING_BOARD`。
- Config：`signal_config.h`。
- Hardware profile：P04。
- Projectspec：`ticlang/sweep_analyzer_final_LP_MSPM0G3507_nortos_ticlang.projectspec`。
- Resource baseline：Flash 18,352 B，SRAM 9,687 B。
- 重点学习：DAC/ADC 两条独立 Timer/Event/DMA 链与每点 settling/acquire/process 顺序。

DUT 是外部模拟路径。当前 reference amplitude 来自 DDS 设定值；绝对增益/相位需直通校准或参考通道。
