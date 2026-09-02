---
id: analog.opamp.offset_bias_noise
title: 运放失调、偏置电流和噪声预算
kind: knowledge
aliases: [运放失调, 输入偏置电流, 运放噪声]
tags: [opamp, offset, bias_current, noise]
summary: 估算 DC 误差和积分噪声何时会吞掉小信号或自动量程余量。
status: ENGINEERING_GUIDE
---

# 运放失调、偏置电流和噪声预算

## DC 误差快算

```text
Vout_offset ≈ Vos × noise_gain + Ib+×Req+×gain_path - Ib-×Req-×gain_path
```

精确符号由拓扑决定；赛场先用绝对值上界。高阻传感器中，`Ib×Rsource` 可能比 Vos 更大。

## 噪声快算

输入电压噪声密度 `en`、电流噪声 `in` 与电阻热噪声共同积分：

```text
Vn_rms ≈ noise_gain × sqrt((en² + (in·Req)² + 4kTReq) × ENBW)
```

一阶低通的等效噪声带宽约 `1.57fc`，不是简单等于 fc。

## 现场做法

- 弱信号先限制模拟带宽，再加增益；不要把全带宽噪声一起放大。
- 低频 DC/桥式测量优先低 Vos/漂移；高源阻优先低 Ib/in；宽带弱信号优先低 en 且有足够 GBW。
- 输入短接到与正常源阻相同的等效阻抗，测输出噪声；不要直接短路后宣称整个系统噪声。

## 常见失败

换成“更高速”运放后噪声变大，常因带宽扩大或输入电流噪声不适合高源阻，并非器件损坏。

