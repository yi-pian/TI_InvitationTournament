---
id: analog.signal_generator.amplitude_termination
title: 信号发生器幅值、Offset 与终端设置
kind: instrument_guide
aliases: [信号发生器怎么设置, High-Z和50欧, offset怎么设]
tags: [signal_generator, amplitude, offset, termination]
summary: 把发生器面板幅值转换为实际负载端幅值，并在负载与偏置边界内安全注入。
status: ENGINEERING_GUIDE
---

# 信号发生器幅值、Offset 与终端设置

## 设置顺序

1. 先断开 DUT，选择波形和低频。
2. 明确 Load=High-Z 或 50Ω；这通常改变显示换算，不一定切换内部真实50Ω源阻。
3. 设 Vpp/Vrms 单位与 Offset，检查 `Vmax=Offset+Vpp/2`、`Vmin=Offset-Vpp/2`。
4. 接目标终端，在负载处用示波器实测，再连接 DUT。

## 典型例

设置 1Vpp、50Ω模式：接50Ω示波器约1Vpp；接1MΩ示波器约2Vpp。详见 `../impedance/generator_scope_50ohm.md`。

## 保护被测电路

单电源 ADC/运放不能接负半周：先设适当正 Offset 或经前端偏置。发生器地通常与保护地相连；连接浮地/非隔离高压系统可能短路。

## 常见错误

- 面板选 Vrms，文档却按 Vpp 计算。
- Offset+幅值超过发生器在当前负载的输出合规范围，波形自己先削顶。
- 用三通同时接两个50Ω负载，使实际端口变25Ω。

