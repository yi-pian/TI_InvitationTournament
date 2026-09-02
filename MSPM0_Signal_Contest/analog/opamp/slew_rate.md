---
id: analog.opamp.slew_rate
title: 压摆率 SR 的设计检查
kind: knowledge
aliases: [压摆率, SR公式, 500kHz 4Vpp需要多大SR]
tags: [opamp, slew_rate, large_signal]
summary: 用输出峰值与最高频率计算正弦最低压摆率，并区分它与小信号带宽。
status: ENGINEERING_GUIDE
---

# 压摆率 SR 的设计检查

## 核心公式

正弦 `v=Vp sin(2πft)` 的最大斜率：

```text
SR_required = 2π f Vp = π f Vpp
```

建议再留 2～5 倍余量给工差、失真和阶跃。SR 够不代表 GBW 够；两项都要满足。

## 示例表

| f | Vpp | 理论最低 SR |
|---:|---:|---:|
| 10 kHz | 4 V | 0.126 V/µs |
| 100 kHz | 4 V | 1.26 V/µs |
| 500 kHz | 4 V | 6.28 V/µs |
| 1 MHz | 2 V | 6.28 V/µs |
| 5 MHz | 2 V | 31.4 V/µs |
| 10 MHz | 2 V | 62.8 V/µs |

## 现场判断

正弦高频后变成近似三角波、上升/下降斜率固定且增大输入无助于变快，通常是 SR 限制。若只是幅值按频率平滑下降而波形仍近正弦，更像 GBW/RC 带宽。

## 测量入口

实际测 SR 见 `../measurement/opamp_slew_rate.md`；上升 SR+ 与下降 SR- 必须分别测，不能只报告一个边沿。

