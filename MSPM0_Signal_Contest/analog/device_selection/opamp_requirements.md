---
id: analog.selection.opamp_requirements
title: 从题目指标生成运放选型清单
kind: selection_guide
aliases: [运放参数需求, GBW SR选型计算]
tags: [device_selection, opamp, requirements]
summary: 把f、Vpp、增益、源阻、负载、供电和误差变成可核对datasheet的门槛列表。
status: ENGINEERING_GUIDE
---

# 从题目指标生成运放选型清单

## 填空表

```text
fmax =
Vin_min/max, Vout_min/max =
signal gain / noise gain =
source R / load R,C =
supply and common-mode =
allowed gain/phase/DC/noise/THD error =
```

## 计算

```text
GBW_min(initial) = fmax × noise_gain × 10
SR_min = 2πfmaxVout,pk
Iout,pk ≥ Vout,pk/Rload + Cload·max(dV/dt)
Vout_error_dc ≈ Vos×noise_gain + Ib×Req×gain
```

然后查：单位增益稳定/最小稳定增益、输入共模、输出摆幅(min/max条件)、输出电流、噪声密度与1/f、CMRR/PSRR、THD、建立时间、容性负载、供电和封装。

## 淘汰规则

任何硬门槛失败即淘汰；不要用“典型SR够”掩盖GBW失败，也不要用更高供电掩盖输入共模不合法。选出2个已备货方案，先在最坏幅频/负载条件台架验证。

