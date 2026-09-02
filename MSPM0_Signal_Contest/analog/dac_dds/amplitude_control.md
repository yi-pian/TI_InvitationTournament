---
id: analog.dds.amplitude_control
title: DDS 输出幅度控制
kind: design_card
aliases: [DDS幅度可调, AD9850调幅, DDS输出幅度为什么固定]
tags: [dds, amplitude, vga, digital_pot]
summary: 比较数字缩放、VGA/PGA、衰减器、数字电位器和后级运放的幅度控制方式。
status: ENGINEERING_GUIDE
---

# DDS 输出幅度控制

## 为什么通常不能直接任意调

许多低成本 DDS 的频率/相位可编程，但内部 DAC 满量程由参考电流/电阻和供电决定，没有通用数字幅度寄存器。廉价模块还固定了负载和低通网络。

## 方案比较

- 软件 DDS+普通 DAC：直接缩放样值，简单，但小幅时有效位数/SNR下降。
- 外置 VGA：连续调幅、带宽高；必须标定控制电压→增益，另做 AGC闭环。
- PGA/继电器/模拟开关衰减器：档位稳定，适合量程；切换有瞬态。
- 数字电位器：适合低频/小信号；受端点电压、带宽、THD与码阶限制。
- 运放后级：固定增益/偏置/驱动；可与档位衰减组合。

## 调试

在多个频率、负载和幅度档逐点标定实际 Vpp；不要只在 1kHz 校准后假设 10MHz 相同。检测削顶、噪声底和切档毛刺。

