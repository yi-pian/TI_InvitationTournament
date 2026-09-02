---
id: analog.recipe.thd_analyzer
title: 失真度/频谱分析仪设计卡
kind: design_recipe
aliases: [THD测量装置, 频谱分析题, 谐波分析仪]
tags: [contest_recipe, fft, thd, spectrum]
summary: 设计低失真量程前端、抗混叠和FFT能量计算，避免前端与信号源失真污染结果。
status: DRAFT_RECIPE
---

# 失真度/频谱分析仪设计卡

## 输入与输出

基波范围、Vpp、最大谐波阶数、THD/SNR/SFDR误差与动态范围。要求 Hk 时必须满足 `k·fmax < Fs/2` 并有前端带宽。

## 信号链

低失真保护/衰减/增益→抗混叠→低抖动ADC→去DC/窗/FFT→多bin谐波能量→THD/SNR/SFDR。

## 核心公式

`THD=sqrt(Σk=2..K Vk²)/V1`。窗需做相干增益与ENBW修正；谐波主瓣积分而非单bin。

## 器件选择

前端自身THD和噪声须优于被测目标；ADC看ENOB/SFDR而非只看位数；参考/时钟进入误差。不要在ADC前滤掉要测的谐波。

## 参数示例

测100kHz基波到H5，Fs必须>1MSPS只是Nyquist底线；为滤波和bin定位通常需更高Fs/合适N。N由分辨率 `Δf=Fs/N` 与RAM共同决定。

## 调试

先短路/终端测噪声底→低失真源直通测系统残余THD→逐档量程→再接DUT。幅度变时THD突升查削顶/SR；换窗变化大查非相干泄漏。

## 接口与实例

复用 `08_applications/harmonic_thd_analyzer` 只作Build参考；新算法来自正式库 `harmonic_quality.md`，不复制Contest旧算法。

