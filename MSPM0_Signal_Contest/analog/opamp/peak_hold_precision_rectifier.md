---
id: analog.opamp.peak_hold_precision_rectifier
title: 精密整流与峰值保持设计卡
kind: design_card
aliases: [峰值保持, 精密整流, 包络检波]
tags: [opamp, rectifier, peak_detector]
summary: 在低幅信号中避开普通二极管压降，并按保持时间、下垂和带宽选参数。
status: ENGINEERING_GUIDE
---

# 精密整流与峰值保持设计卡

## 什么时候使用

MCU ADC 速度有限但只需包络/峰值，或输入小到普通二极管压降不可忽略。能高速完整采样时，数字 Vpp/RMS 往往更易校准。

## 核心关系

保持电容下垂近似 `ΔV≈Ileak·Thold/C`；释放电阻产生 `τ=RreleaseC`。τ 太小峰值掉得快，太大响应变慢。

## 选型

运放需有足够 GBW/SR、输入输出范围和快速过载恢复；二极管反向漏电、电容介质吸收、ADC 输入漏电共同决定误差。高频峰值保持常需专用高速器件。

## 调试

输入已知正弦，先看整流节点，再看保持电容。正常是快速充到峰值、按设计缓慢下降；充不上去查运放 SR/输出电流/二极管，下降过快查漏电与 C，释放太慢查 τ。

## 限制

峰值检波器的频响会混入扫频结果；测 -3dB 时必须先标定检波链或改用同步 ADC 幅值估计。

