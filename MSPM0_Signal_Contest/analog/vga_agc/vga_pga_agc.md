---
id: analog.vga_agc.concepts
title: VGA、PGA、AGC 与自动量程的区别
kind: knowledge
aliases: [VGA和AGC区别, PGA, 自动量程]
tags: [vga, pga, agc, auto_range]
summary: 区分可变增益器件、离散增益器件、闭环控制和量程状态机。
status: ENGINEERING_GUIDE
---

# VGA、PGA、AGC 与自动量程的区别

- VGA：增益由模拟控制电压连续/近连续改变。
- PGA：数字控制离散档位，增益可重复但切档有瞬态。
- AGC：检测输出→控制器→VGA/PGA→再次检测的闭环系统。
- 自动量程：在衰减、增益、参考或多通道之间切换，目标是避免削顶并提高有效分辨率，不一定连续控制。

## 最小闭环

```text
ADC幅值/削顶检测 → MCU误差与状态机 → DAC/数字码 → VGA/PGA → 等待建立 → 再测量
```

VGA本身不会知道目标 1.2Vpp，也不会自动稳定输出。

## 防振荡

- 目标设死区，如 1.14～1.26Vpp 内不调。
- 控制步长/速率限制；每次更新后等待模拟建立和完整测量帧。
- 切档使用上下不同阈值并要求连续 K 帧确认。
- 削顶立即降增益；升增益要更保守。

正式算法链链接 `MSPM0_Signal_Contest/00_docs/measurement_recipes/automatic_gain.md` 和 `auto_range.md`。

