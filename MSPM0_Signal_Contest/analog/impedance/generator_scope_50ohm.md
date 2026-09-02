---
id: analog.impedance.generator_scope_50ohm
title: 信号发生器 50Ω/High-Z 与示波器终端
kind: knowledge
aliases: [1Vpp变2Vpp, 信号源50欧, 示波器1M与50欧]
tags: [signal_generator, oscilloscope, termination, 50ohm]
summary: 解释发生器显示幅值与实际端口幅值为何可能相差两倍。
status: ENGINEERING_GUIDE
---

# 信号发生器 50Ω/High-Z 与示波器终端

## 现象

发生器显示 1 Vpp，接 1MΩ示波器测到约 2 Vpp；接 50Ω示波器又是 1 Vpp。

## 原因

常见发生器内部是理想源串 50Ω。“50Ω模式显示 1 Vpp”表示厂商假定末端有 50Ω并已替你换算；内部开路等效电压约 2 Vpp。接 1MΩ后几乎没有分压，所以看到约 2 Vpp。

## 连接表

| 发生器幅值显示模式 | 示波器输入 | 典型实际值 |
|---|---|---|
| 50Ω，显示 1 Vpp | 50Ω | 约 1 Vpp |
| 50Ω，显示 1 Vpp | 1MΩ | 约 2 Vpp |
| High-Z，显示 1 Vpp | 1MΩ | 约 1 Vpp |
| High-Z，显示 1 Vpp | 50Ω | 取决于机型换算，通常约 0.5 Vpp；以终端实测为准 |

## 仪器设置

- 低频面包板/高阻输入：发生器 Load=High-Z，示波器=1MΩ/×10。
- 传输线/射频模块：发生器=50Ω，末端=50Ω，避免中间重复终端。
- Offset 也受同一分压关系影响；连接后在负载端复核。

## 排错

幅度恰好两倍/一半时依次检查发生器 Load、示波器输入、外置终端、三通和探头倍率。不要用软件乘 2 掩盖接线条件。

