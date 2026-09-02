---
id: analog.recipe.weak_signal_detector
title: 强干扰背景下弱单音检测设计卡
kind: design_recipe
aliases: [微弱信号检测, 同步检波, 锁相检测]
tags: [contest_recipe, weak_signal, lock_in, filtering]
summary: 用前端不饱和、带限增益与同步检测，从强噪声/邻频干扰中提取已知频率弱信号。
status: DRAFT_RECIPE
---

# 强干扰背景下弱单音检测设计卡

## 输入与输出

目标频率是否已知、最小幅值、最大干扰幅值/频率、所需响应时间与误差。动态范围先换成dB。

## 信号链

保护→模拟带通/陷波或预选→低噪声增益/可变增益→ADC→数字下变频/Lock-in→低通/幅相估计。

## 原理与公式

与同频sin/cos相乘后平均得到I/Q，幅值 `A≈2sqrt(I²+Q²)`（具体归一化按Recipe）；积分时间越长，等效噪声带宽越窄但响应越慢。

## 模拟前端

最强干扰不能让任何级饱和；若1MHz干扰对500kHz目标需40dB抑制，按 `../filter/case_500k_1m.md` 评估高阶滤波或适度模拟滤波+同步检测。

## 元件选择

按输入噪声、源阻、GBW/SR、动态范围与线性度选放大器；ADC看SFDR/ENOB。AGC必须在正式测量前锁定。

## 调试

先无目标测噪声底→只开目标测增益→只开干扰确认不饱和→两者同时→扫相位/频率偏差。误检查参考频偏/泄漏；幅值随积分窗跳查非整周期与锁定。

## 现有算法

链接正式 `weak_signal_amplitude.md`、`lock_in` 和 `sine_fit`，不在模拟卡复制API。

