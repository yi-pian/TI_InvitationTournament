---
id: analog.vga_agc.auto_gain_30mv_600mv
title: 30mVpp～600mVpp 输入稳定到 1.2Vpp
kind: design_card
aliases: [30mV到600mV自动增益, 输出稳定1.2Vpp]
tags: [agc, vga, calibration, case_study]
summary: 计算所需增益范围，给出测量、控制、标定、死区与保护策略。
status: DRAFT_RECIPE
---

# 30mVpp～600mVpp 输入稳定到 1.2Vpp

## 增益范围

```text
Gmax = 1.2/0.03 = 40 V/V = 32.04 dB
Gmin = 1.2/0.6 = 2 V/V = 6.02 dB
```

实际需为器件误差、频响和余量预留约数 dB，且 ADC 满量程要高于 1.2Vpp。

## 信号链

保护/可选衰减→VGA/PGA→抗混叠→ADC→稳健 Vpp/RMS→MCU控制→DAC/数字接口→增益器件。

## 控制策略

1. 上电先低增益，避免未知大信号削顶。
2. 测量有效帧；若削顶立即快速降增益。
3. 目标窗口 1.14～1.26Vpp 内保持。
4. 窗外按 dB 误差限步更新，例如每次≤2dB；等待建立 1～数帧。
5. 连续 3 帧同方向超窗才升增益，降增益可更快。

## 标定

在多个频率和控制码实测 `gain_dB(code,f)`，保存单调 LUT/拟合版本。不要只用 datasheet 典型斜率。量程切换帧标为无效，不进入正式结果。

## 正常/异常

正常：输入阶跃后有限时间单调进入目标窗。来回摆动：死区太小、步长太大或未等建立；小信号达不到：Gmax/噪声底不足；大信号仍削顶：前级或VGA输入先饱和。

