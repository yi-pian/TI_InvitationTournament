# MODULE CARD

MODULE: Spectral SNR

CATEGORY: DSP / Quality Metric

功能：目标 band 能量 / 分析范围内未排除 bin 能量，输出功率比与 dB。

输入：magnitude、signal/analysis ranges、可选 excluded ranges；输出 energy/ratio/dB。

是否原地处理：不适用。

依赖：`log10f`。

典型用途：FFT 单音 SNR，显式排除 DC/谐波。

不要用于：未排谐波却称严格 SNR；不同窗/带宽结果直接比较。

计算量 O(B·E) 最坏，RAM O(1) + 外部小 range 表。

Benefits：排除规则显式。Trade-offs：bin 噪声能量依赖窗、分析带宽和泄漏。

状态：PC_VERIFIED；100/6 能量真值 12.2184868 dB 通过。
