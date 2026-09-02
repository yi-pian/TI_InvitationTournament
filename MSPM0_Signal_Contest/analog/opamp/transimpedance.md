---
id: analog.opamp.transimpedance
title: 跨阻放大器设计卡
kind: design_card
aliases: [跨阻放大器, 光电流放大, TIA]
tags: [opamp, transimpedance, current_input]
summary: 把传感器电流变成电压并用反馈电容控制噪声增益和稳定性。
status: ENGINEERING_GUIDE
---

# 跨阻放大器设计卡

## 核心公式

理想 `Vout=Vref-IinRf`。先由最大电流和可用摆幅选 `Rf≤Vout,pk/Imax`。传感器/输入/PCB总电容会使噪声增益上升，Rf 并联 Cf 用于稳定与限带；精确 Cf 应结合运放 GBW、总电容和目标相位裕度计算/仿真。

## 示例

Imax=10µA，希望相对 Vref 最大摆幅 1V，先取 `Rf≈100kΩ`。若带宽 10kHz，简单输出极点初值 `Cf≈1/(2πRf·10k)≈159pF`，但这不是完整稳定性设计，必须用阶跃/噪声增益验证。

## 选型与 PCB

低 Ib/in、足够 GBW；反相节点极短、保持清洁，必要时 guard。光电二极管偏置、极性和暗电流按器件资料。

## 调试

先用已知电阻/电流源注入，遮光测暗电平，再逐步增加输入。振荡查总电容/Cf/布局；输出贴轨查极性、Vref、Rf 与最大电流。

