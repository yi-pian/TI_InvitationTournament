---
id: analog.dac.reconstruction_filter
title: DAC/DDS 重建滤波器设计
kind: design_card
aliases: [重建滤波器, DAC镜像, DDS低通]
tags: [dac, dds, reconstruction, imaging]
summary: 在保留目标带宽的同时压低采样镜像、时钟馈通和量化台阶。
status: ENGINEERING_GUIDE
---

# DAC/DDS 重建滤波器设计

零阶保持 DAC 输出在 `kFs±fout` 附近有镜像，且包络带 sinc 下垂。DDS 还可能有相位截断、DAC杂散与参考时钟馈通。

## 设计步骤

1. 写目标最高输出 `fout,max`、更新/参考频率 `Fs`、允许通带衰减和首个需压制镜像。
2. 通带边缘高于 `fout,max`，阻带边缘低于最近不可接受镜像。
3. 用阶数公式选择 Butterworth/Chebyshev/Elliptic；相位敏感则选择 Bessel/低阶并做数字预补偿。
4. 把 DAC 输出阻抗、50Ω终端和后级输入计入。

## 注意

当 fout 接近 Nyquist，通带与镜像过近，任何模拟滤波器都很难同时平坦与高抑制；优先提高 Fs/参考时钟或换架构。

## 测量

频谱仪/示波器 FFT 同时看基波与 `Fs-fout` 镜像；普通示波器时域“像正弦”不代表 SFDR/THD 合格。

