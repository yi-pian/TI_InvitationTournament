---
id: analog.opamp.inverting_summing_amp
title: 反相放大与加法器设计卡
kind: design_card
aliases: [反相放大, 加法器, 多路信号相加]
tags: [opamp, inverting, summing]
summary: 用虚地节点获得可控输入阻抗、反相增益和多路加权求和。
status: ENGINEERING_GUIDE
---

# 反相放大与加法器设计卡

## 什么时候使用

需要明确输入阻抗、反相、衰减/放大或把信号与 DC 偏置加权相加。

## 核心公式

以参考点 `Vref` 为虚地：

```text
Vout - Vref = -Rf Σ((Vi - Vref)/Ri)
Zin_i ≈ Ri
noise_gain = 1 + Rf/(R1||R2||...)
```

不能只按信号增益选 GBW；多输入并联后噪声增益可能更高。

## 示例

把 ±0.5V 信号反相 2 倍并抬到 1.65V：`Rin=10kΩ`、`Rf=20kΩ`，非反相端接低阻 1.65V。输出约 0.65～2.65V。

## 注意

- Vref 必须低噪声、低阻且充分去耦；电阻网络会把各路源阻抗带入误差。
- 大 Rf 与反相节点寄生电容形成极点，可在 Rf 并小电容稳定，但会限制带宽。
- 输入源断电时可能经 Rin 反向供电，必要时限流/开关隔离。

## 调试

分别只接一路验证权重，再叠加；反相节点应接近 Vref。输出不对先测每个 Ri、源阻抗和 Vref，不先改软件系数。

