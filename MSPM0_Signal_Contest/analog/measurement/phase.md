---
id: analog.measurement.phase
title: 相位差与通道延时测量卡
kind: measurement_recipe
aliases: [相位差怎么测, 通道延时标定, FFT相位]
tags: [measurement, phase, delay, dual_channel]
summary: 用双通道时间差、FFT/相关或同步检波测相位，并扣除前端与通道延时。
status: DRAFT_RECIPE
---

# 相位差与通道延时测量卡

## 目标与接线

同一参考源分到 CH1/ADC1 与 DUT→CH2/ADC2，通道共用时钟并尽量同步采样。测量前先做 `thru`：两通道接同一信号。

## 仪器设置

信号源正弦、频率稳定；示波器两通道相同探头/带宽/耦合，触发 CH1。避免一个通道AC coupling、另一个DC coupling。

## 步骤

1. thru 扫目标频率，保存通道相位/延时基线。
2. 接入 DUT，在稳态后采同一帧 Vin/Vout。
3. 单音可用时间差、FFT复数相位或 Lock-in；宽带/非正弦可用互相关。
4. 解包裹后减去 thru/fixture 基线。

## 公式与示例

`Δφ=360°fΔt`。100kHz 下 Δt=250ns，对应 9°。固定 50ns 通道延时在100kHz是1.8°、1MHz是18°，所以不能用一个固定“度数”全频段校正，应存延时或频率响应。

## 正常与异常

同源 thru 应接近校准残差；相位随频率平滑。突然±360°跳变是 wrap；随幅值变可能是阈值法迟滞/过驱；随通道交换改变说明通道误差。

## 比赛快速版

先同源校准→双通道同步→多周期平均→相位unwrap→减通道/fixture。正式 Recipe：`phase_delay.md` 与 `multichannel_delay_compensation.md`。

