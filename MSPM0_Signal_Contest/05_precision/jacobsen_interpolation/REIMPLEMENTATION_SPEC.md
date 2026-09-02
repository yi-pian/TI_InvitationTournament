# Jacobsen clean reimplementation specification

状态：`SPECIFICATION_COMPLETE`。类型：`SOURCE_LOST → CLEAN_REIMPLEMENTATION`；不是旧源码恢复。

## Contract

- 输入：矩形窗、前向符号为 `exp(-j2πkn/N)` 的复数 FFT；离散最大峰 `k` 及左右邻 bin；Fs、N。
- 输出：`fractional_bin`、`interpolated_bin`、`frequency_hz`。
- 限制：只用于远离 DC/Nyquist 的孤立单音；中心 bin 必须不小于左右邻 bin；不声称适配 Hann/Hamming/Blackman。
- 复杂度：O(1) 时间和 RAM；无状态、无动态分配；失败时结果不变。

公式采用 Jacobsen/Kootsookos 的三复数 bin estimator：

```text
δ = Re{(X[k-1]-X[k+1])/(2X[k]-X[k-1]-X[k+1])}
f = (k+δ) Fs/N
```

来源：[Jacobsen and Kootsookos, Fast, Accurate Frequency Estimators, IEEE Signal Processing Magazine, 2007](https://www.ericjacobsen.org/Files/FastFreqEstimators_SPTnT.pdf)。

拒绝条件：空指针、边界峰、非有限复数、零/非最大中心、奇异分母或 `|δ|>0.5001`。旧 `SignalJacobsen_Interpolate` 的签名已丢失；新 `SignalJacobsen_Process` 为 breaking clean API。

验证：已知 fractional-bin 复指数生成三点 DFT，扫描 N/峰位/δ；Python double oracle 与 C float 同输入比较；另测零谱、边界、NaN。历史 `BUILD_VERIFIED` 不继承。
