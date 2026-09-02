# Waveform Classification Guide

当前仓库有测量 Primitive、FFT、Harmonic、Correlation、Sine Fit 和 AM **生成**模块，但没有经过数据集/板级验证的通用“未知波形分类器、AM/FM/CW 识别器或盲源分离器”。因此本文件是设计 Recipe 和 gap 边界，不把启发式伪装成稳定模块。

## 1. 分类前必须先形成特征链

```text
ADC DMA
→ ADC To Voltage
→ clipping/validity gate
→ DC、Vpp、RMS、AC RMS、crest factor
→ frequency/period consistency
→ duty、rise/fall、slope segments
→ Window + FFT + Magnitude + harmonic ratios
→ feature vector + confidence/reject
```

每一步存在的原因：时域特征区分电平/边沿，周期一致性确认稳定周期，频域特征区分基波与谐波结构，quality gate 避免 clipping/低 SNR 时硬猜。

## 2. 可作为起点的启发式

| 候选 | 可观察特征 | 容易混淆/失效 |
|---|---|---|
| DC/近 DC | AC RMS 很小、无稳定周期 | 极低频只采到短片段 |
| Sine | sine-fit residual 小、H2+低、crest 接近正弦 | 被滤圆的三角/低 SNR |
| Square/Pulse | 两个平台稳定、duty 有效、边沿快、奇次谐波丰富 | clipping 的正弦、带宽不足方波 |
| Triangle | 上下斜率段近线性、奇次谐波按高阶快速下降 | 低采样率锯齿/滤波方波 |
| Sawtooth | 单向长斜坡 + 快速复位、各次谐波 | trigger 不稳、复位边沿被滤除 |

这里的“接近”都需要按真实题目范围训练/标定阈值，不能从名字生成固定常数。

## 3. 推荐实现层次

- 短特征（crest factor、斜率符号计数等）：先写在本 Recipe/Application，标 `[RECIPE]`。
- 已有 Primitive：`sine_fit_3param/4param`、`harmonic`、`statistics`、`frequency_zero_cross`、`fft`，直接复用。
- 只有在多个题目重复、接口稳定、PC 数据集有 confusion matrix/边界测试后，才升级为正式 classifier `.c/.h`。

当前通用 waveform classifier：`MISSING GAP / DOCUMENTATION_ONLY`。

## 4. AM / FM / CW 识别

TI 2022 官方参考题要求对 AM/FM/CW 进行识别、测调制度、显示并输出解调信号。这比“对原始 ADC 做一次 FFT”复杂：高频前端/下变频或带通采样、carrier recovery、envelope/instantaneous phase、deviation estimation、classification confidence、demodulation filter 都需要题目频段对应的硬件和算法。

- 现有 `am_modulation` 是 AM 样本**生成**，不是识别/解调。
- 未找到正式 FM demodulator、modulation classifier、validated dataset。
- 当前状态：`MISSING GAP`。拿到具体题目后先定 RF/IF 架构和采样方案，不能直接承诺内部 ADC 覆盖载频。

## 5. Signal separation

相关、FFT、滤波和 Lock-In 可以在已知频率/参考条件下提取某分量；这不等于通用 blind source separation。当前：

- 已知目标频率弱信号：使用 `[MODULE] lock_in` 或 sine fit，已有 Recipe。
- 频带可分：FFT 识别后用 FIR/IIR，需题目滤波指标。
- 两源盲分离/未知调制混合：`MISSING GAP`，没有验证 Primitive/Application。

## 6. 采样、RAM、验证

- Fs 必须覆盖最高有用频率/边沿带宽并有 anti-alias；至少采若干稳定周期，具体数量由最低频、响应时间和分类置信度决定。
- RAM 基线是 raw `2N` + float `4N` + FFT complex `8N` + magnitude `~2N`；可通过 in-place/分阶段复用降低峰值。
- PC 验证必须包含多幅值、多频率、DC offset、噪声、clipping、滤波后波形和“不属于任何已知类”；报告 confusion matrix 和 reject rate。
- Board 验证必须包含真实前端带宽/失真；没有硬件数据不能写 BOARD_VERIFIED。

