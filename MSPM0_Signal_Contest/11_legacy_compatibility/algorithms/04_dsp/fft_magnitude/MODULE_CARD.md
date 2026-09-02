# MODULE CARD

MODULE: FFT Magnitude

CATEGORY: DSP / Spectrum

功能：把 complex FFT 的 0~N/2 转为原始 `sqrt(re²+im²)`。

输入：N complex；输出：N/2+1 float raw magnitude。

是否原地处理：NO。

依赖：`hypotf`、complex 类型。

典型用途：Peak/Harmonic/THD 的谱幅前端。

不要用于：直接把 raw magnitude 当 Vpeak；负频率复相位分析。

计算量：O(N/2)，每 bin 一次 hypotf。

RAM：输出约 `4*(N/2+1)`。

Benefits：与 FFT、增益校正分离。Trade-offs：丢失相位；额外 magnitude buffer。

可连接：`FFT -> Magnitude -> GainCorrection/Peak`。

状态：PC_VERIFIED；冲激和正弦链通过。
