---
id: analog.troubleshooting.high_frequency_drop
title: 高频幅度越来越低故障树
kind: troubleshooting
aliases: [高频幅度下降, 频率高波形变小]
tags: [troubleshooting, bandwidth, high_frequency]
summary: 逐级区分源、RC、运放GBW/SR、ADC驱动、探头、传输线和PCB寄生。
status: ENGINEERING_GUIDE
---

# 高频幅度越来越低故障树

1. **源端**：直接在DUT输入端测，不信面板；查源负载与电缆。
2. **每一级扫频**：记录输入/输出比，找到第一个开始下降的级。
3. **无源 RC**：把源阻、探头电容、ADC电容加入 `fc=1/(2πRC)`。
4. **运放**：小幅仍降查GBW；只大幅降/三角化查SR。
5. **ADC**：延长采样时间/降Fs若改善，查建立与driver。
6. **示波器**：×10、关20MHz limit、短地；确认探头带宽。
7. **传输/PCB**：50Ω终端、stub、回流、连接器与长飞线。

修复必须作用于第一个失真级；后级数字增益只会把噪声一起放大。

