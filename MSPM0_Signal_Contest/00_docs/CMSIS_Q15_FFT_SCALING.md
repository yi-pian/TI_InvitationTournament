# CMSIS Q15 FFT 缩放与幅值恢复

## 1 为什么必须看这份文档

Q15 FFT 很容易出现“频率 bin 正确、幅值全错”。原因不是 FFT 失效，而是三层标尺同时存在：输入满量程、CMSIS 每级缩放、窗函数相干增益。

## 2 本库实际调用的是什么

当前 Q15 backend 使用 CMSIS-DSP V1.10.0 的复数前向 CFFT：

```c
arm_cfft_q15(instance, data, 0U, 1U);
```

实数输入先填成 `real=x[n], imag=0`。上层仍调用原来的 `SignalFFT_ForwardReal()`，Recipe 不出现 `arm_cfft_q15()`。

## 3 输入如何归一化

wrapper 先扫描输入复数分量的最大绝对值 `M`，再做：

```text
q15_input ≈ trunc(x / M × 32767)
```

这样可以尽量使用 Q15 动态范围。若所有输入为 0，直接返回全 0。非有限数在转换前拒绝。

代价是扫描一次 O(N)，以及 Q15 量化。公开 API 需要 `-fno-strict-aliasing`，因为 wrapper 在调用者的 float complex 输出区中复用前半部分作为 Q15 工作区。

## 4 CMSIS 内部缩放

固定点 CFFT 每一级都会缩小，避免蝶形加法溢出。N 点前向结果整体约带 `1/N` 缩放。常见输出小数位趋势如下：

| N | 需要恢复的位数 `log2(N)` | Q15 输出格式趋势 |
|---:|---:|---|
| 512 | 9 | 约 10.6 |
| 1024 | 10 | 约 11.5 |
| 2048 | 11 | 约 12.4 |
| 4096 | 12 | 约 13.3 |

这里的“10.6”表示符号/整数部分变多，小数位只剩约 6 位。N 越大，单个输出 bin 的定点量化越明显。

## 5 本库怎样保持旧 API 语义

旧 Reference FFT 输出是不除 N 的复数 DFT。为避免上层 Magnitude、WindowGainCorrection、THD 全部改写，Q15 wrapper 做：

```text
float_output ≈ q15_output_integer × M × N / 32767
```

因此后级仍按旧规则使用，不要再额外乘一次 N。

## 6 Hann 后恢复 0.5 V peak

Hann 窗后的单边正弦峰值公式：

```text
amplitude_peak = 2 × |X[k]| / sum(window[n])
```

PC 真值测试：N=1024、正弦 peak=0.5、精确落 bin、Hann。

| Backend | 恢复幅值 V peak | 绝对误差 V |
|---|---:|---:|
| Reference C | 0.499998628 | 0.000001372 |
| CMSIS Q15 | 0.500131542 | 0.000131542 |
| CMSIS Q31 | 0.499998777 | 0.000001223 |
| CMSIS F32 | 0.499998747 | 0.000001253 |

Q15 幅值链已真实 PC 验证，但误差高于 Q31/F32。它仍通过 backend benchmark 的比赛级容差，不等于“与 float 完全相同”。

## 7 为什么 Q15 不是本轮稳定默认

Q15 在 6 类信号、4 种点数的 backend benchmark 中为 `26/0`，但套用原来更严格的 THD/Phase 回归时：

- THD：11.183106% 对 11.180341%，绝对差约 0.002766 个百分点；
- Phase：30.001083° 对 30.000000°，绝对差约 0.001083°。

两项分别超过旧测试阈值，所以稳定 Competition 默认选择 CMSIS Q31。Q15 保留为明确接受该误差后的性能候选，真实 cycle 仍为 `PENDING_BOARD`。

## 8 排查幅值错误

依次检查：

1. ADC 零点是否先去除；
2. Q15 满量程 `M` 是否正确；
3. 是否使用了错误 FFT 长度；
4. 是否把 CMSIS 的 `1/N` 又除了一次；
5. 是否使用单边谱的系数 2；DC 和 Nyquist 不能乘 2；
6. 是否除以 `sum(window)` 或等价的 coherent gain；
7. 信号是否非相干，能量是否散到邻近 bin；
8. 是否发生 ADC 或 Q15 饱和。
