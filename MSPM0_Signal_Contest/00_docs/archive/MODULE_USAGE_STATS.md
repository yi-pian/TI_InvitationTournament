# Module Usage Stats

统计范围：12 个最终 Integration Applications。数字表示有多少应用完整 Pipeline 使用该模块/能力，用于安排实板验证优先级；不把 `_TEMPLATE` 计入。

| 模块/能力 | 使用应用数 | 主要应用 | 实板优先级 |
|---|---:|---|---|
| ADC DMA | 10 | 除 Frequency A、DDS 外 | P0 |
| RawToVoltage / ADCToVoltage | 9 | 所有 ADC 测量链 | P0 |
| Timer-triggered sample rate | 10 | ADC/DAC pipelines | P0 |
| RemoveDC | 7 | Frequency B/C、Spectrum、THD、Phase、Analyzer、Template | P0 |
| ZeroCross/Interpolation | 4 | Signal Meter、Frequency B、Analyzer、Template | P1 |
| Window + FFT + Magnitude | 6 | Frequency C、Spectrum、THD、Phase、Analyzer、Template | P0 |
| CMSIS Q31 FFT Backend | 6 | 同上最终配置 | P0 |
| Peak/Interpolation | 4 | Frequency C、Spectrum、Analyzer、Template | P1 |
| Harmonic/THD | 3 | THD、Analyzer、Template | P1 |
| DualADC | 3 | Phase、Analyzer、Template | P0 for phase |
| Correlation Phase | 3 | Phase、Analyzer、Template | P1 |
| DAC DMA platform | 4 | DDS、Sweep、Replay、Template reserved | P0 |
| DDS | 3 | DDS、Sweep、Template reserved | P0 |
| Trigger/Ring/Segment | 1 | Wave Replay | P2 |
| LockIn | 1 | Sweep | P1 |
| SNR/SFDR | 1 | Analyzer Spectrum | P2 |

每建立一个真实赛题复现目录，应在此增加一列或一条“题目使用”记录；高频模块优先做 Board、温漂、边界与仪器校准验证。
