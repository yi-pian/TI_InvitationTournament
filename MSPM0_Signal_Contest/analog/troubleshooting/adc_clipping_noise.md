---
id: analog.troubleshooting.adc_clipping_noise
title: ADC 码值削顶、跳动与通道串扰故障树
kind: troubleshooting
aliases: [ADC采样不准, ADC削顶, ADC噪声大]
tags: [troubleshooting, adc, clipping, settling]
summary: 从范围、参考、驱动建立、采样时间、地、时钟和校准定位ADC异常。
status: ENGINEERING_GUIDE
---

# ADC 码值削顶、跳动与通道串扰故障树

| 现象 | 快速测试 | 最可能原因/修复 |
|---|---|---|
| 长时间0/满码 | 示波器量ADC脚 | 过范围、偏置错、钳位导通 |
| 降Fs后更准 | 延长采样时间 | 源阻/采样电容未建立，加buffer/RC优化 |
| 切通道后首点错 | 丢弃/延长首采样 | 通道切换残留电荷 |
| 所有码一起漂 | 测VREF/VDDA | 参考/供电噪声或温漂 |
| 屏幕刷新时跳 | 停刷新对照 | 地回流/电源串扰 |
| 只某幅值段弯曲 | 慢DC斜坡测试 | 前端非线性/ADC INL/保护器件 |

必须按 `ADC code→引脚电压→前端反算→物理量` 分层检查，不能只改最后比例系数。

