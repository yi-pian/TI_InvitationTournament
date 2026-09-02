# MODULE CARD

MODULE: THD

CATEGORY: DSP / Distortion

功能：`sqrt(sum(H2..Hm energy)/H1 energy)`，输出 ratio 与 percent。

输入：Harmonic result；输出 THD ratio/%。

是否原地处理：不适用。

依赖：Harmonic。

典型用途：2~5 或更多谐波总失真。

不要用于：THD 前低通删谐波、把非谐波噪声也叫 THD。

计算量 LOW O(H)，RAM O(1)。

Benefits：定义明确、能量比公共标度相消。Trade-offs：只包含配置阶数；前端频响会偏置高阶。

可连接：`Harmonic -> THD -> percent`。

状态：PC_VERIFIED；理论11.18034%，BASIC误差9.54e-6%，COMP误差0.001387%。
