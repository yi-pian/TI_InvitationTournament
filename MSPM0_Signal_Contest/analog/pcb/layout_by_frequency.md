---
id: analog.pcb.layout_by_frequency
title: 电赛模拟 PCB 按频率分级布线
kind: design_card
aliases: [模拟PCB布线, 50欧走线, ADC时钟布局]
tags: [pcb, layout, return_path, adc, dds]
summary: 按低频、中频和1～20MHz分级关注反馈、回流、时钟、SMA、串扰和电源噪声。
status: ENGINEERING_GUIDE
---

# 电赛模拟 PCB 按频率分级布线

## 所有频率都要做

运放反馈环最短；去耦贴脚；模拟输入远离时钟/屏幕；每个连接器旁有明确地回流；测试点不把高阻节点拉成长天线。

## <100kHz

重点是漏电、50Hz、地环路、热电势、源阻和开关电源纹波。面包板可用于低阻低增益验证，但高阻/TIA/µV测量仍应清洁PCB。

## 100kHz～1MHz

反馈寄生、探头电容、运放GBW/SR、ADC采样回冲开始突出。使用连续地平面、短模拟链、SMA或短同轴；把ADC RC贴近引脚。

## 1～20MHz

按高速边沿而不只按正弦频率处理：控制阻抗、源/端接、最短回流、差分成对、时钟隔离、避免支路stub。DDS/DAC输出与ADC时钟分区，参考与模拟地不被数字输出总线穿越。

## 50Ω走线

阻抗由层叠、线宽、介质厚度和参考平面决定，不能凭“某个线宽”通用。短线在低频可不严格，但SMA到高速节点仍要连续参考、少过孔、无长stub。

