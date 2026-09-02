---
id: analog.opamp.input_output_range
title: 运放输入共模与输出摆幅检查
kind: knowledge
aliases: [输入共模范围, 输出摆幅, 轨到轨]
tags: [opamp, common_mode, output_swing, supply]
summary: 在选定供电、增益和负载下检查每个输入脚及输出是否处于合法范围。
status: ENGINEERING_GUIDE
---

# 运放输入共模与输出摆幅检查

## 为什么理论公式正确仍会削顶

闭环增益公式不保证输入级和输出级有工作余量。必须对最坏时刻计算：

- `Vcm=(V+ + V-)/2` 是否在 datasheet 的输入共模范围。
- `Vout_min/max` 在指定负载下是否落在输出摆幅保证范围。
- 输出电流 `|Vout-Vload|/Rload` 是否超过线性驱动能力。

“Rail-to-rail”也有负载、温度、供电和输入输出方向条件，不能理解为精确到电源轨。

## 单电源示例

3.3V 运放把以 1.65V 为中心的 1Vpp 信号放大 2 倍，理想输出 0.65～2.65V。若运放在当前负载下只保证 0.1～3.0V，摆幅有余量；还要检查输入共模是否覆盖输入与反馈节点。

## 调试

示波器同时看输入、两输入脚 DC 电位和输出；降低幅度或改为 ±电源后恢复，说明范围/摆幅问题。负载断开后恢复，说明输出电流或稳定性问题。

