---
id: analog.recipe.amplitude_meter
title: 宽动态幅值/RMS 测量仪设计卡
kind: design_recipe
aliases: [幅值测量仪, RMS表, 10k到500k幅值]
tags: [contest_recipe, amplitude, rms, adc]
summary: 从保护、自动量程、抗混叠、ADC驱动到稳健Vpp/RMS和校准的完整链。
status: DRAFT_RECIPE
---

# 宽动态幅值/RMS 测量仪设计卡

## 什么时候使用

题目要求 10kHz～500kHz 等范围内自动测 Vpp、峰值或 RMS，输入跨度较大。

## 输入与输出

输入先写 min/max Vpp、DC offset、频率、源阻和过压；输出写单位、允许误差、刷新时间。不要默认全是正弦。

## 原理与核心公式

保护→多档衰减/PGA/VGA→偏置→抗混叠→ADC driver→ADC→前端反算→Vpp/RMS。`Vrms,ac=sqrt(mean((x-mean(x))²))`。

## 元件/芯片选择

量程比决定档位；运放按 GBW/SR/范围/噪声；ADC按 Fs/fmax、ENOB、范围和通道。±输入先映射，不直连。

## 参数示例

30mVpp～6Vpp 是200:1；只靠12bit ADC，小信号仅约20码满幅，不够。用×1/×10/×100等量程让ADC利用率保持约20%～80%。

## 连接、阻抗与带宽

高阻输入先缓冲；50Ω题按终端设计。每档都要独立标定增益、offset和频响。模拟滤波防饱和/混叠，数字算法不修复削顶。

## 供电与接口

VGA由DAC/数字接口控制；切档 GPIO/模拟开关有唯一owner。ADC DMA帧完成后才处理，过渡帧丢弃。

## PCB、调试与故障

接口保护和量程开关靠近入口，ADC RC靠近引脚。先固定一档→逐档标定→自动切档→再加稳健算法。偏小查终端/带宽；跳档查迟滞；小信号噪声大查模拟带宽和增益。

## 替代与电赛实例

窄范围可固定增益；只有正弦可用拟合/Lock-in提高精度。链接 `../measurement/vpp_rms.md` 与正式 `vpp.md`、`rms_ac_dc.md`、`auto_range.md`。

