---
id: analog.recipe.frequency_phase_meter
title: 频率与相位测量仪设计卡
kind: design_recipe
aliases: [频率相位测量题, 双通道相位计]
tags: [contest_recipe, frequency, phase, comparator]
summary: 组合比较器/Timer与双通道ADC，在宽频段内测频并校准相位延时。
status: DRAFT_RECIPE
---

# 频率与相位测量仪设计卡

## 输入与输出

两路波形的频率、幅度、DC、相位范围、SNR与最高误差；输出Hz和degree/radian，说明刷新时间。

## 原理

干净边沿：保护/偏置→迟滞比较器→Timer Capture测频/时间差。任意波/带噪：双通道同步ADC→过零、FFT相位或互相关。

## 核心公式

`f=M/Δt`；`Δφ=360°fΔt`。固定通道延时应以秒标定，再随频率换算相位。

## 元件与范围

比较器按输入范围、失调、延迟/dispersion与输出接口；ADC按同步性、Fs和SNR。输入小且噪声大时先限带/增益，不能无限加迟滞。

## 接口与资源

Capture链占COMP/Event/Timer；ADC链占双ADC/Timer/DMA/RAM。组合前用资源表决定主路径，不同时堆两套默认链。

## 调试

先同源同相输入→测0°残差与频率→交换通道→90°/180°已知相位→扫频建立延时表。倍频查毛刺，丢边查带宽，上下跳360°查wrap。

## 电赛实例与替代

1Hz～10MHz宽频测量采用分段方法，不让单一FFT或单一门控覆盖全部。链接 `../measurement/frequency.md`、`phase.md` 和现有 frequency/phase Recipe。

