---
id: analog.opamp.noninverting_amp
title: 同相放大器设计卡
kind: design_card
aliases: [同相放大, 非反相放大]
tags: [opamp, amplifier, high_input_impedance]
summary: 在保持高输入阻抗时设置正增益，并完成范围、带宽和偏置检查。
status: ENGINEERING_GUIDE
---

# 同相放大器设计卡

## 什么时候使用

信号源阻抗较高、输出要同相，且所需闭环增益 ≥1。

## 核心公式

```text
Av = 1 + Rf/Rg
noise_gain = Av
fBW ≈ GBW/Av
```

单电源以 `Vref` 为基准时，Rg 下端接低阻 Vref，输出为 `Vout=Vref+Av(Vin-Vref)`。

## 元件选择示例

增益 5：取 `Rg=2.49kΩ`、`Rf=10kΩ`，实际增益约 5.016。电阻过大增加热噪声和偏置误差，过小增加输出负载；常从 kΩ～几十 kΩ起选。精度由电阻比公差决定，优先 0.1% 匹配网络。

## 输入输出范围

逐端点计算 Vin 与 Vout；单电源交流信号必须围绕 Vref。再算 `SR=2πfVout,pk` 和 GBW 余量。

## 调试

示波器 CH1=Vin、CH2=Vout，DC coupling；先低频小信号验证增益和偏置。增益高频下降查 GBW，波形三角化查 SR，顶端/底端平直查摆幅，整体 DC 偏移查 Vref 与 Vos。

