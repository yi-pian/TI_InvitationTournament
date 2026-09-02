# MODULE CARD

MODULE: Autocorrelation

CATEGORY: DSP / Periodicity

功能：计算 lag0~L 归一化自相关，并在用户频率先验范围找周期峰。

输入：samples/count/maxlag/workspace；输出 correlation、period lag、frequency。

是否原地处理：不适用。

依赖：`sqrtf`。

典型用途：失真周期信号频率、低 SNR 周期性检测。

不要用于：min_lag 不合理、单次瞬态、频率快速变化。

计算量 HIGH O(NL)，输出 `4(L+1)`。

Benefits：不依赖阈值或正弦形状。Trade-offs：整数 lag、lag0附近宽峰、倍频/分频歧义。

状态：PC_VERIFIED；周期32/Fs32000得到1000 Hz。
