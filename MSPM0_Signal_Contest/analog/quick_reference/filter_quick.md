---
id: analog.quick.filter
title: 滤波器 30 秒速查
kind: quick_reference
aliases: [滤波器速查, 阶数速算]
tags: [quick_reference, filter]
summary: 由fp/fs/Ap/As快速确定类型、阶数、运放和验证点。
status: ENGINEERING_GUIDE
---

# 滤波器 30 秒速查

```text
RC: fc=1/(2πReqC)
一阶: 远离fc后约20dB/dec/阶
Butterworth阶数:
n≥log10((10^(As/10)-1)/(10^(Ap/10)-1))/(2log10(fs/fp))
```

- 脉冲/相位：Bessel。
- 通用幅值：Butterworth。
- 近阻带且容许纹波：Chebyshev。
- 极窄过渡且可校准幅相：Elliptic。

必查：源/负载、每个二阶节Q、运放GBW/SR、元件公差、ADC前仍需模拟抗混叠。先单节扫频再级联。

