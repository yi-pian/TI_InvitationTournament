---
id: analog.measurement.harmonic_thd
title: 谐波、THD、SNR 与 SFDR 测量卡
kind: measurement_recipe
aliases: [测THD, 失真度测量, 谐波幅值]
tags: [measurement, fft, thd, sfdr]
summary: 从低失真前端、相干采样/窗、幅值校正到多bin能量计算完整测量动态指标。
status: DRAFT_RECIPE
---

# 谐波、THD、SNR 与 SFDR 测量卡

## 接线与仪器

低失真信号源→被测链→ADC；示波器只作削顶/幅值检查。前端带宽必须覆盖要统计的最高谐波，且自身 THD 低于目标。

## 步骤

1. 选 Fs/N，使基波及 H2…Hk 低于 Nyquist；尽量相干采样，否则选窗。
2. 去 DC、乘窗、FFT并做窗口相干增益/ENBW校正。
3. 对每个谐波主瓣做 multi-bin 能量积分，不只取单bin。
4. `THD=sqrt(V2²+...+Vk²)/V1`；dB 用 `20log10(THD)`。
5. 明确 SNR 是否排除谐波，SFDR 的 spur 搜索是否排除 DC/主瓣。

## 示例

V1=1Vrms，V2=10mVrms，V3=5mVrms，则 `THD=sqrt(0.01²+0.005²)/1≈1.118%`，约 -39.0dB。

## 异常与改进

改变输入幅度后 THD突升：削顶/SR；换窗结果差异大：泄漏/幅值校正错误；高阶谐波变小：前端滤掉了被测量。正式链：`MSPM0_Signal_Contest/00_docs/measurement_recipes/harmonic_quality.md`。

## 比赛快速版

先证前端不失真→Hk低于Nyquist→窗增益+多bin→按定义报告，不把ADC/源失真当DUT。

