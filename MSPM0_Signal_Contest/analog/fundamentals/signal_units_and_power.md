---
id: analog.fundamentals.signal_units_power
title: Vpp、Vp、Vrms、dBV、dBm 与功率换算
kind: knowledge
aliases: [Vpp换算, RMS换算, dBm怎么算, dBV]
tags: [amplitude, rms, power, 50ohm]
summary: 在明确波形与负载条件后完成电压、功率和对数单位换算。
status: ENGINEERING_GUIDE
---

# Vpp、Vp、Vrms、dBV、dBm 与功率换算

## 先问两个条件

- 波形是不是正弦？只有正弦才有 `Vrms = Vpp/(2√2)`。
- 电压是负载端电压还是信号源开路电压？dBm 必须给负载阻抗。

## 核心公式

正弦波：

```text
Vp = Vpp / 2
Vrms = Vp / √2 = Vpp / (2√2)
P = Vrms² / R
dBV = 20 log10(Vrms / 1 V)
dBm = 10 log10(P / 1 mW)
```

任意采样波形：`Vrms = sqrt(sum(x[n]²)/N)`；若要 AC RMS，先减去均值。方波、脉冲和含谐波波形不能从 Vpp 直接推出 RMS。

## 示例

50Ω 负载上测得 2 Vpp 正弦：`Vrms=0.707 V`，`P=10 mW`，因此 `10 dBm`；`dBV≈-3.01 dBV`。

## 怎么测

- 信号源设正弦、1 kHz、目标幅度，并明确 Load=50Ω 或 High-Z。
- 示波器用 DC coupling、正确探头倍率；低频可开 20 MHz limit 降噪。
- 若计算 dBm，确认 50Ω 终端确实存在并使用终端处实测 Vrms。

## 错误结果排查

- 恰好差 2 倍：先查信号源 50Ω/High-Z 与终端。
- RMS 与 Vpp 关系不符：查削顶、DC offset、谐波和波形类型。
- dBm 差 3 dB：查电压是 RMS 还是峰值；功率比 2 倍就是约 3.01 dB。

