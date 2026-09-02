---
id: analog.quick.measurement_formula
title: 测量公式 30 秒速查
kind: quick_reference
aliases: [测量公式速查, 电赛公式]
tags: [quick_reference, formula, measurement]
summary: 汇总幅值、功率、频率、相位、增益、SR、滤波和阻抗常用公式。
status: ENGINEERING_GUIDE
---

# 测量公式 30 秒速查

```text
正弦: Vrms=Vpp/(2√2)；任意波: Vrms=sqrt(mean(x²))
P=Vrms²/R；dBV=20log10(Vrms/1V)；dBm=10log10(P/1mW)
f=M/Δt；Δφ=360°fΔt
Gain_dB=20log10(Vout/Vin)
SRmin=2πfVp=πfVpp
fc(RC)=1/(2πRC)
V-3dB=Vpass/√2
Zin=Rs·V2/(V1-V2)
Zout=RL(V0/VL-1)
THD=sqrt(V2²+...+Vk²)/V1
```

所有公式先确认：正弦/任意波、RMS/峰值、负载、单位、DC是否去除、通道是否校准。

