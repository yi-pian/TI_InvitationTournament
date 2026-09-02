---
id: analog.filter.order_estimation
title: 从通带与阻带指标估算滤波器阶数
kind: knowledge
aliases: [滤波器阶数怎么算, 需要几阶滤波器]
tags: [filter, order, attenuation]
summary: 用响应族公式估算最低阶数，再按元件公差、运放和级间加载留余量。
status: ENGINEERING_GUIDE
---

# 从通带与阻带指标估算滤波器阶数

## Butterworth 低通

给通带边缘 `fp`、最大衰减 `Ap`，阻带 `fs`、最小衰减 `As`：

```text
n ≥ log10((10^(As/10)-1)/(10^(Ap/10)-1)) / (2 log10(fs/fp))
```

向上取整。若只用渐近线粗估，每阶远离截止后约 20dB/dec，但接近截止时不能只靠斜率。

## 现场流程

1. 把“目标最高频率”变成允许的 fp/Ap，不要把 fc 直接等于目标边界。
2. 把干扰频率/抑制度变成 fs/As。
3. 先算响应族最低阶数，再检查相位/群延迟要求。
4. 分成二阶节，最高 Q 节最敏感，使用 1%或更好元件并预留调试点。

## 失败原因

算出的数学阶数满足但实测不够，常因截止频率放置错误、运放 GBW/Q 增强不足、元件公差、源负载或前级已经饱和。

