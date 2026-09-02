# MODULE CARD

MODULE: Normalized Cross Correlation

CATEGORY: DSP / Similarity and Delay

功能：计算 -L~+L 归一化互相关，输出最大正相关与最大绝对相关 lag。

输入：等长 A/B、max_lag、输出数组；输出 coefficient curve 与 lag。

是否原地处理：不适用。

依赖：`sqrtf`。

典型用途：相似非正弦波延迟/相位、模板对齐。

不要用于：大 L/N 的实时 M0+ 链未预算、DC 未去、周期信号多个等价峰未限制范围。

计算量 HIGH，O(N·(2L+1))；输出 `4*(2L+1)`。

Benefits：不要求纯正弦；归一化后尺度变化影响小。Trade-offs：计算重、整数 lag、边界重叠长度变化。

状态：PC_VERIFIED；延迟4 sample峰1.0通过。
