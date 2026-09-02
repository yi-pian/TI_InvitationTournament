---
id: analog.measurement.impedance_rlc
title: 输入/输出阻抗与 RLC 参数测量卡
kind: measurement_recipe
aliases: [测输入阻抗, 测输出阻抗, 测RLC, 阻抗测量]
tags: [measurement, impedance, rlc]
summary: 用标准电阻和同步幅相比值求复阻抗，并通过open/short/load校准fixture。
status: DRAFT_RECIPE
---

# 输入/输出阻抗与 RLC 参数测量卡

## 输入阻抗

信号源→标准串联电阻 Rs→DUT；同步测源侧 V1 与 DUT 端 V2：

```text
Zin = Rs · V2 / (V1 - V2)
```

若只测幅值只能得近似模值；RLC/相位必须使用复数幅相。

## 输出阻抗

保持输入不变，测近似开路输出 V0，再接已知 RL 测 VL：

```text
Zout = RL · (V0/VL - 1)
```

不要让“开路测量”被示波器50Ω终端破坏。

## 仪器设置与步骤

正弦扫频、小信号；双通道同探头/终端。Rs 选与未知阻抗同量级以获得灵敏度。先 open/short/known-load 校准线缆、开关和通道幅相，再测 DUT。

## RLC

得到 `Z=R+jX` 后：电感区 `L=X/(2πf)`，电容区 `C=-1/(2πfX)`；实际器件还含 ESR/寄生，跨频点拟合比单点更可靠。谐振点由 X过零/幅值极值确定，Q需按串/并模型选公式。

## 异常

结果随Rs变化大：源/负载/通道校准不完整；高频乱跳：fixture寄生；测电容出现“负电感”等非物理结果：相位符号/通道延时错误。

## 比赛快速版

Rs≈未知值→双通道复比值→open/short/load→多频点拟合，不用单点幅值硬猜RLC。

