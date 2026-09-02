---
id: analog.filter.case_500k_1m
title: 10kHz～500kHz 目标与 1MHz 干扰的滤波案例
kind: design_card
aliases: [1MHz干扰怎么滤, 500kHz目标40dB抑制]
tags: [filter, case_study, anti_alias]
summary: 说明目标带边缘与干扰仅相差2倍时，为什么需要高阶/更高采样率或频率搬移。
status: ENGINEERING_GUIDE
---

# 10kHz～500kHz 目标与 1MHz 干扰的滤波案例

## 指标

目标 10k～500kHz，1MHz 干扰需抑制≥40dB。若把 Butterworth 通带边缘设 `fp=500kHz`、允许 `Ap=1dB`，阻带 `fs=1MHz`、`As=40dB`：

```text
n ≥ log10((10^4-1)/(10^0.1-1)) / (2log10(2)) ≈ 7.62
```

至少 8 阶。这个结果说明“只差一个倍频程却要40dB”对模拟低通很苛刻；两阶或四阶不足。

## 可行策略

1. 8阶 Butterworth/Chebyshev 分四个二阶节，验证相位与元件敏感性。
2. 若允许通带纹波，Chebyshev/Elliptic 可降阶，但相位/群延迟更差。
3. 提高采样率不能改变 1MHz 在模拟前端造成的饱和；但可让数字滤波接手更多选择性。
4. 若只测已知频率单音，先用较温和模拟滤波防饱和，再用 Lock-in/同步检波；正式算法见 `MSPM0_Signal_Contest/00_docs/measurement_recipes/weak_signal_amplitude.md`。

## 运放

每节检查 GBW 与 Q；8阶不是把四个普通 RC 随便串联。先单节扫频，再级联；目标 500kHz 处的总衰减必须计入校准。

