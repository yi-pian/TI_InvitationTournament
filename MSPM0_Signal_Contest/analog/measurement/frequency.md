---
id: analog.measurement.frequency
title: 频率测量方法卡
kind: measurement_recipe
aliases: [频率怎么测, 计数法, 测周期法, 等精度测频]
tags: [measurement, frequency, timer, fft]
summary: 按频率范围、波形质量和精度选择周期法、计数法、倒数计数、过零或FFT。
status: DRAFT_RECIPE
---

# 频率测量方法卡

## 目标与接线

输入→保护/偏置→比较器迟滞→Timer Capture；或输入→ADC→过零/FFT。示波器并接高阻验证，不重复50Ω终端。

## 方法选择

| 频率 | 默认方法 | 说明 |
|---:|---|---|
| 1Hz | 周期/倒数计数 | 直接等完整周期；处理超时 |
| 100Hz | 多周期平均的周期法 | 时间戳分辨率充足 |
| 10kHz | Timer Capture/等精度 | 比较器边沿通常优先 |
| 100kHz | Capture/等精度 | 校准阈值和传播延迟 |
| 1MHz | 高速比较器+Capture/门控计数 | 查最小脉宽和Timer时钟 |
| 10MHz | 外部高速比较器/分频+计数 | MCU输入、路由和PCB常先成瓶颈 |

非正弦/频谱同时需要时用 FFT；低 SNR 单音可用自相关/插值，但计算量更高。

## 仪器设置与步骤

信号源给已知频率/幅值；示波器DC coupling、合适阈值、短地线。先验证整形每周期只有一对边沿，再记录多个时间戳，剔除超时/溢出并平均。

## 公式与示例

周期法 `f=M/Δt`（M个完整周期）。80MHz计时器测100kHz，单周期800 tick；若跨100周期，量化相对误差约降至单tick/80000。

## 异常与改进

读数倍频：一个周期多毛刺；减半：丢边沿；低频跳动：窗口太短；高频偏差：输入带宽/传播/Timer路由。正式 Recipe：`MSPM0_Signal_Contest/00_docs/measurement_recipes/frequency_period.md`。

## 比赛快速版

干净边沿先Capture，低频测多周期，高频门控计数；带噪先限带+迟滞；始终处理超时和溢出。

