---
id: analog.opamp.gbw
title: 用 GBW 估算闭环带宽
kind: knowledge
aliases: [GBW怎么算, 运放带宽, 闭环带宽]
tags: [opamp, gbw, bandwidth, noise_gain]
summary: 用噪声增益而非只看信号增益估算运放闭环带宽与误差余量。
status: ENGINEERING_GUIDE
---

# 用 GBW 估算闭环带宽

## 核心关系

对近似单主极点、单位增益稳定运放：

```text
f_closed,-3dB ≈ GBW / noise_gain
```

同相放大噪声增益 `1+Rf/Rg`；反相放大信号增益是 `-Rf/Rin`，噪声增益仍是 `1+Rf/Rin`。多极点、去补偿运放和复杂反馈不能只用这个近似。

## 选型余量

只要求到 `fmax` 仍大致有增益，可初筛 `GBW ≥ fmax×noise_gain×10`。10 倍是经验余量，约让单极点幅值误差较小；严格幅相指标应从允许误差反算并查闭环曲线。

## 示例

500 kHz、反相增益 -10：噪声增益 11，经验所需 `GBW≈55 MHz`，不是 5 MHz。再检查 SR、输出摆幅、负载和稳定性。

## 怎么测

小信号正弦避免 SR：例如 100 mVpp；固定输入，按对数扫频，双通道同步测 Vin/Vout。低频增益为 `A0`，下降到 `A0/√2` 即 -3 dB 带宽。

## 常见失败

- 大信号测得带宽随幅度变化：已经受 SR 限制。
- 把闭环 -3 dB 直接叫 GBW：仅在单极点近似下乘以噪声增益才可估算。
- 反相电路忘记噪声增益的“+1”。

