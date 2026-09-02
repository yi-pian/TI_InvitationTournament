---
id: analog.recipe.sweep_analyzer
title: 自动扫频/网络分析仪设计卡
kind: design_recipe
aliases: [网络分析, 频率响应测试仪, 自动扫频]
tags: [contest_recipe, sweep, bode, impedance]
summary: 用DDS激励、同步Vin/Vout测量、thru校准与对数扫频得到幅相和阻抗响应。
status: DRAFT_RECIPE
---

# 自动扫频/网络分析仪设计卡

## 输入与输出

扫频范围、频点数、幅值、DUT负载、增益/相位误差、允许总时间；输出Bode/截止/谐振/模型参数。

## 信号链

DDS→输出缓冲/已知源阻→DUT；参考通道测Vin，响应通道测Vout→同步幅相→thru/fixture校准→曲线。

## 核心公式

`H(f)=Vout_complex/Vin_complex`；`dB=20log10|H|`；相位=`arg(H)`。阻抗可由已知Rs与复电压比求。

## 频点与稳定

对数扫频，拐点/谐振附近加密；每点等待至少若干DUT时间常数与若干周期，低频等待往往决定总时间。自动量程改变后重新等待并标记无效帧。

## 器件/接口

DDS幅值、通道增益/延时和fixture随频率校准。双ADC同步优先；单通道方案只能依赖重复切换和源稳定，误差更大。

## 调试

先短接DUT做thru应得到0dB/校准后0°→接已知RC验证fc与相位→再测未知。曲线不平查源/通道频响；相位线性斜坡查延时；谐振乱跳查稳定时间/SNR。

## 现有入口

硬件组合参考 `08_applications/sweep_analyzer`；绝对幅相仍为Board `NOT_RUN`。正式 Recipe：`frequency_response.md`、`frequency_response_compensation.md`。

