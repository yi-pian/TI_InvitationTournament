---
id: analog.opamp.bias_level_shift
title: 单电源交流信号偏置与电平转换
kind: design_card
aliases: [偏置电路, 电平抬升, 虚拟地, 单电源交流]
tags: [opamp, bias, level_shift, single_supply]
summary: 把双极性交流信号映射到 ADC/单电源运放合法范围并保持可反算。
status: ENGINEERING_GUIDE
---

# 单电源交流信号偏置与电平转换

## 目标

将 `Vin∈[Vin,min,Vin,max]` 线性变成 `Vadc=aVin+b`，并在 ADC 两端留 5%～10%裕量。

## 方法

1. AC coupling：串联电容隔 DC，后端用电阻把节点偏置到 Vref；高通 `fc=1/(2πReqC)`。
2. DC coupling：用反相加法器/差分放大器同时衰减与平移，可测原信号 DC。

## 例：±5V 到 0～3.3V

若目标映射到 0.15～3.15V：跨度 3.0V 对应输入跨度 10V，`a=0.3`，中心 `b=1.65V`，即 `Vadc=0.3Vin+1.65V`。用电阻网络实现后必须用实测两点/多点标定；前端还要限流、钳位和检查输入源阻。

## Vref

电阻分压后应缓冲或确保其交流阻抗足够低，并以 100nF+1µF 近端去耦。多个高增益级共用 Vref 时计算总 AC 回流，避免中点随信号摆动。

## 调试

先无输入测 Vref/输出中心，再输入最小、0、最大三点。中心错查 Vref；斜率错查电阻比/负载；上下不对称削顶查运放共模和输出摆幅。

