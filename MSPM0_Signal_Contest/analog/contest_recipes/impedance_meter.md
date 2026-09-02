---
id: analog.recipe.impedance_meter
title: 阻抗/RLC/谐振测量仪设计卡
kind: design_recipe
aliases: [阻抗测量题, RLC参数仪, 谐振测量]
tags: [contest_recipe, impedance, rlc, sweep]
summary: 用已知激励和复电压比测阻抗，做fixture校准、多频模型拟合与自动量程。
status: DRAFT_RECIPE
---

# 阻抗/RLC/谐振测量仪设计卡

## 输入与输出

阻抗范围、频率范围、串/并模型、R/L/C误差、Q和测试电平。高Q器件还要限定测试夹具和寄生。

## 原理

DDS→已知Rs/桥式网络→DUT→双通道同步测幅相→复阻抗→跨频模型拟合。`Z=Rs·Vdut/(Vsrc-Vdut)`。

## 元件选择

Rs按量程切换，使两测量电压都不接近噪声底或满量程；用低温漂标准电阻。模拟开关Ron/电容必须校准，高频夹具短且屏蔽。

## 接口与带宽

双ADC通道增益/延时先校准；DDS输出阻抗和缓冲纳入。自动量程切换后等待稳定并丢弃过渡帧。

## 调试

依次 open、short、known R、known C、known L；再扫频看非理想。测值随线缆姿态变说明fixture寄生；负R/负L异常先查相位符号与延时。

## 替代

单一窄范围低频可用RC时间常数/振荡法；宽频复阻抗优先同步幅相。链接 `../measurement/impedance_rlc.md`。

