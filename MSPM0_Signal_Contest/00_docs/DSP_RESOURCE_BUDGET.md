# DSP 资源预算与内存复用指南

MSPM0G3507 是 Cortex-M0+，没有硬件浮点单元。`float` 接口便于比赛快速拼装和减少单位错误，但平方根、三角函数、log/exp 与大量浮点乘加都由软件执行。原则是：ISR 只完成采样握手，整帧算法放在主循环/任务中。

| 模块 | 时间复杂度 | 额外 RAM | M0+注意 |
|---|---:|---:|---|
| ADC_ToVoltage / RemoveDC / Mean / Vpp / RMS | O(N) | O(1)，另有输出时 4N | 首选基础积木 |
| Statistics | O(N) | O(1) | 一遍稳定统计，含 sqrt |
| ZeroCross + Interpolation | O(N)+O(E) | event约12E、position 4E | E 是过零数，容量不足会返回错误 |
| MultiCycleAverage | O(E) | O(1) | 很省 RAM/CPU |
| MovingAverage | O(N) | 输出 4N | 运行和维护，非居中；改变带宽 |
| Median | O(N·W log W)量级 | 输出4N + workspace4W | W 应小；不可进 ISR |
| MAD / Hampel | 平均 O(N) / 约O(N·W) | workspace4N / 输出4N+4W | 鲁棒但非线性 |
| FIR | O(N·T) | state 4(T-1) | taps 外部提供；线性相位需对称系数 |
| IIR Biquad SOS | O(N·S) | state 8S | 系数/稳定性/相位需离线设计验证 |
| Window | O(N) | 可原地 O(1) | 实时算 sin/cos 省表 RAM 但耗 CPU |
| FFT | O(N log2 N) | complex 8N，见 FFT 预算 | 1024 Simple 优先 |
| Magnitude | O(N/2) | 约2N | 每 bin hypot/sqrt 较慢 |
| Harmonic/THD/SNR/SFDR | O(bins或阶数) | 多数 O(1) | 标度与分析带必须一致 |
| Correlation | O(N·L) | 输出 4(2L+1) | L 大时很慢 |
| Autocorrelation | O(N·L) | 输出 4(L+1) | 先限制合理周期范围 |
| SineFit3 | O(N) 两遍 | O(1) | 三角递推+3×3 求解 |
| SineFit4 | O(I·N) | O(1) | 高风险/高 CPU，只做窄带局部搜索 |
| LockIn | O(N) 两遍 | O(1) | 已知相干参考时很有价值 |

## Simple 与 RAM-saving 两种风格

小白/比赛初期优先 Simple：每个语义阶段有清楚数组名，例如 `raw_codes -> voltage_v -> spectrum -> magnitude`，方便观察和排错。

RAM-saving 只在 map 证明需要后做：

- RemoveDC、Window、GainCorrection 可按各自 API 原地处理。
- FIR/IIR 支持 input=output，但 state 必须独立且跨块保存。
- Median/Hampel 不能让输入输出重叠，因为邻域仍要读原数据。
- FFT 使用复数原地工作区；不要同时建立多个 4096 float 数组。
- 任何复用必须写清“最后一次读取”和“第一次覆盖”的时间点，DMA 仍在写时绝不能覆盖。

## 栈与全局数组

大型帧数组建议由应用显式静态分配并集中列出，不要在函数局部栈上声明 1024/2048 点数组，也不要让算法模块偷偷持有大型全局区。本文模块内部仅使用小型标量/小矩阵；大型 output/workspace/state 都由调用者提供。

最终 RAM 结论必须来自目标 CCS/TI Arm Clang 构建的 map 和实测栈水位，不可把 PC 可运行等同 MCU 资源已验证。
