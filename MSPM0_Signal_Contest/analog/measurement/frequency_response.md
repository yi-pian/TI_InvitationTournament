---
id: analog.measurement.frequency_response
title: 幅频与相频扫频测量卡
kind: measurement_recipe
aliases: [扫频, Bode图, 频率响应测量, -3dB截止]
tags: [measurement, sweep, bode, frequency_response]
summary: 用DDS、双通道幅相测量、对数扫频与thru校准获得可信Bode图。
status: DRAFT_RECIPE
---

# 幅频与相频扫频测量卡

## 接线

DDS/信号源→DUT；CH1/ADC1 测 DUT 实际输入，CH2/ADC2 测输出。若只能单通道，先做 thru 并承认时变源幅度误差。

## 仪器设置

正弦、小到不削顶且大于噪声；High-Z/50Ω条件全程一致。示波器两通道DC coupling、相同探头与带宽。

## 步骤

1. 先直通扫频，保存源+线缆+通道基线。
2. 采用对数频点，拐点附近加密；每次改频等待若干时间常数/周期。
3. 同步测 `Vin_rms/Vout_rms` 与相位。
4. `Gain=Vout/Vin`，`Gain_dB=20log10(Gain)`；减去thru基线。
5. 以低频/通带增益为基准找下降3.0103dB点。

## 示例

通带增益 5.00V/V，-3dB目标为 `5/√2=3.536V/V`，不是绝对 0.707V/V。

## 异常

曲线随激励幅度变：DUT非线性/SR；高频乱跳：SNR/点数/稳定时间不足；相位整体斜坡：固定通道延时；幅值把源下垂当DUT：未测Vin。

## 比赛快速版

双通道→先thru→对数扫频→每点等待→测比值与相位→相对通带找-3dB。链接正式 `frequency_response.md` 和 `frequency_response_compensation.md`。

