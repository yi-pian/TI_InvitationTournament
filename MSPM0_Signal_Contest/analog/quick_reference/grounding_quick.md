---
id: analog.quick.grounding
title: 供电接地 30 秒速查
kind: quick_reference
aliases: [接地速查, 去耦速查]
tags: [quick_reference, ground, power]
summary: 快速排查去耦、回流、台式仪器地和大负载干扰。
status: ENGINEERING_GUIDE
---

# 供电接地 30 秒速查

- 每电源脚100nF贴脚；分区1µF；入口/负载10µF按稳定性。
- 连续地平面；数字/屏幕/DC-DC回流不穿微弱输入。
- 低频大电流可星形回电源入口；高速不要随意切地平面。
- 示波器地夹/发生器地通常连保护地，连接前确认。
- 拔USB/屏幕噪声消失＝回流/供电耦合，不等于算法问题。
- 电源打嗝先查过流/短路/交叉调节，不盲加电容。

