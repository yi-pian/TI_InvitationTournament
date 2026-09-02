---
id: analog.recipe.automatic_gain
title: VGA/PGA 自动增益系统设计卡
kind: design_recipe
aliases: [AGC题, 程控增益, 输出稳幅]
tags: [contest_recipe, agc, vga, pga]
summary: 从增益范围、检测、DAC/数字控制、标定和状态机实现稳定自动增益。
status: DRAFT_RECIPE
---

# VGA/PGA 自动增益系统设计卡

## 输入与输出

Vin最小/最大、频带、crest factor；目标Vout窗、settling时间、允许过冲和稳态误差。

## 原理与公式

`Grequired=Vtarget/Vin`，dB域相减更适合线性dB VGA。ADC检测→控制器→DAC/PGA→增益器件→再次检测。

## 元件选择

连续宽范围用VGA，稳定离散档用PGA/衰减器；控制DAC分辨率必须小于允许增益步进。器件带宽、输入阻抗、噪声和输出范围覆盖全增益。

## 参数例

30mVpp～600mVpp到1.2Vpp需约6.02～32.04dB，详见 `../vga_agc/auto_gain_30mv_600mv.md`。

## 接口与电源

DAC控制端先缓冲/滤波但不能让环路过慢；SPI/PGA切档与ADC帧状态同步。模拟地与DAC参考稳定。

## 调试顺序

固定增益逐点标定→开环按LUT设定→低速闭环→输入阶跃→加削顶快速退档→全频率/温度校准。振荡按 `../troubleshooting/agc_oscillation.md`。

## 替代

动态范围不大时固定增益更可靠；两三档范围优先模拟开关/PGA，不为“自动”强上连续VGA。

