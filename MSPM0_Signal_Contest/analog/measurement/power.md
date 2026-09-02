---
id: analog.measurement.power
title: 电压、电流与有功功率测量卡
kind: measurement_recipe
aliases: [功率怎么测, 有功功率, 相位功率]
tags: [measurement, power, voltage, current]
summary: 用同步双通道电压/电流采样计算真实有功功率并校准增益、偏置和延时。
status: DRAFT_RECIPE
---

# 电压、电流与有功功率测量卡

## 安全与接线

低压隔离系统：电压衰减通道与电流分流/传感通道→同步 ADC。涉及市电或非隔离高压时，普通示波器地夹不可随意连接；必须使用合规隔离探头/传感器和安全方案，本卡不替代高压安全设计。

## 步骤

1. 分别用标准源标定电压/电流增益和零点。
2. 同一信号做通道延时校准。
3. 同步采完整周期：`P=mean(v[n]i[n])`，`Vrms`、`Irms`分别平方平均。
4. `S=VrmsIrms`，功率因数 `PF=P/S`。

## 示例

正弦 Vrms=2V、Irms=0.5A、相位差60°，`P=2×0.5×cos60°=0.5W`，不能只用 Vrms×Irms 得有功功率。

## 异常

纯电阻 PF不接近1：通道延时/极性/偏置；零输入仍有功率：offset未去；大电流波形削顶：量程/传感器饱和。

## 比赛快速版

同步两路→校准增益/零点/延时→整周期平均 `vi`→同时报告Vrms、Irms、P、PF和量程。

