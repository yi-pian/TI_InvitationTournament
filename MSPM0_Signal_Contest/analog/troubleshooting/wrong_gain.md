---
id: analog.troubleshooting.wrong_gain
title: 放大倍数不对故障树
kind: troubleshooting
aliases: [增益不对, 放大倍数偏小]
tags: [troubleshooting, gain, opamp]
summary: 从电阻、源阻、输入/输出负载、GBW、SR、摆幅和测量终端定位增益错误。
status: ENGINEERING_GUIDE
---

# 放大倍数不对故障树

## 快速顺序

1. 断电实测反馈/输入电阻和焊点，检查数量级与电阻码。
2. 画包含信号源50Ω、耦合电容、输入电阻和负载的完整网络。
3. 低频小幅测增益；若正确，高频才错→GBW/RC/探头。
4. 保持频率降低幅度；若恢复→SR/摆幅/过流/非线性。
5. 断开负载；若恢复→输出驱动或负载终端。
6. 同时测 Vin、V+、V-、Vout 的 DC 和 AC。

## 典型特征

恰好一半：50Ω终端；固定比例偏差：电阻/源阻；随频率下降：GBW/RC；波峰平：摆幅；三角化：SR；随机跳：接触/振荡。

