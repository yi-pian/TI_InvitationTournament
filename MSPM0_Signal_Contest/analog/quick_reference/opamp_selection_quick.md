---
id: analog.quick.opamp_selection
title: 运放选型 30 秒速查
kind: quick_reference
aliases: [运放速查, GBW SR速查]
tags: [quick_reference, opamp]
summary: 30秒完成GBW、SR、范围、负载和误差五项淘汰。
status: ENGINEERING_GUIDE
---

# 运放选型 30 秒速查

```text
noise_gain = 同相增益；反相时 = 1+Rf/Rin
GBW初筛 ≥ fmax × noise_gain × 10
SR最低 = 2πfmaxVout,pk = πfmaxVpp
Iout,pk ≥ Vpk/Rload + Cload·max(dV/dt)
```

然后查 datasheet：单位增益稳定/最小稳定增益→输入共模→输出摆幅（当前负载）→供电→Vos/Ib/en/in/THD→容性负载。

症状：高频小=GBW/RC；大信号三角=SR；顶部平=摆幅；接负载坏=输出电流/稳定性；DC偏=Vos/Ib/Vref。

