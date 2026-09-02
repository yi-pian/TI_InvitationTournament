# Coherent sampling clean reimplementation specification

状态：`SPECIFICATION_COMPLETE`。类型：`SOURCE_LOST → CLEAN_REIMPLEMENTATION`。

## Purpose and contract

为给定 `Fs`、记录长度 `N` 和期望输入频率，搜索整数周期数 `J`，使 `f_coherent=J·Fs/N` 最接近期望值。可要求 `gcd(J,N)=1`，使样本遍历尽可能多的相位位置。

- 输入：期望频率、Fs、N、允许 J 范围、是否互质。
- 输出：J、N、gcd、实际 coherent frequency、Hz/ppm 偏差。
- 实信号约束：`0<f<Fs/2`，`1≤J≤N/2`。
- 搜索：枚举合法 J，先最小绝对频差，完全相同时选较小 J；O(Jmax-Jmin+1)，O(1) RAM。

依据：[TI ADCPro User Guide SBAU128C](https://www.ti.com/lit/pdf/SBAU128) 对 coherent sampling 和互质周期/记录长度的说明，以及 IEEE 1241 的 `fi=J·fs/M` 条件。互质是相位覆盖要求，不是所有普通 FFT 测量的强制条件。

旧 `GCD/Nearest` 参数契约丢失。新 API 为 `SignalCoherentSampling_GCDU32` 与 `SignalCoherentSampling_FindNearest`，不承诺旧签名兼容。测试含精确/非精确目标、互质搜索、无候选、非法 Nyquist/记录长度。
