---
id: analog.measurement.vpp_rms
title: Vpp 与 RMS 测量卡
kind: measurement_recipe
aliases: [测峰峰值, 测真有效值, 多频RMS]
tags: [measurement, vpp, rms]
summary: 按波形、噪声和谐波选择示波器、稳健Vpp、数字RMS或频域能量法。
status: DRAFT_RECIPE
---

# Vpp 与 RMS 测量卡

## 目标与场景

测周期信号峰峰值、总 RMS、AC RMS；适合示波器验收和 ADC 自动测量。

## 所需仪器与接线

信号源 OUT→前端输入，公共地；示波器 CH1 在前端输入、CH2 在 ADC 前。不得让示波器 50Ω意外成为被测负载。

## 仪器设置

- 信号源：目标波形/频率，Load 与真实终端一致，Offset 明确。
- 示波器：DC coupling；1MΩ/×10；带宽覆盖最高谐波；稳定触发；噪声观察时关闭过度 average。

## 测量步骤

1. 确认无削顶且采样覆盖整数个或多个周期。
2. 干净波形可用 max-min；有孤立错码用分位数/Hampel，但先确认尖峰不是被测对象。
3. 总 RMS 直接平方平均；AC RMS 先减均值。
4. 用示波器和已知输入交叉验证并做前端增益/偏置反算。

## 公式与示例

`Vpp=max-min`；`Vrms,total=sqrt(mean(x²))`；`Vrms,ac=sqrt(mean((x-mean(x))²))`。正交多频分量满足 `Vrms=sqrt(ΣVrms,k²)`。含谐波时不能用 `Vpp/(2√2)`。

例：1Vpk基波+0.2Vpk二次谐波，`Vrms=sqrt((1/√2)²+(0.2/√2)²)≈0.721V`；Vpp取决于相位，不可反推此值。

## 正常、异常与改进

正常：示波器与ADC反算在误差预算内。异常偏小查量程/带宽/负载，偏大查毛刺/过冲，RMS偏大查DC是否未去除。提高精度用同步采样、多周期平均、校准和正式 Recipe：`MSPM0_Signal_Contest/00_docs/measurement_recipes/vpp.md`、`rms_ac_dc.md`。

## 比赛快速版

先看削顶→确认50Ω/探头→取多周期→Vpp用稳健端点、AC RMS先去均值→按前端比例反算。

