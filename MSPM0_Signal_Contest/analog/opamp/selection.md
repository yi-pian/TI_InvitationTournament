---
id: analog.opamp.selection
title: 电赛运放参数需求到器件要求
kind: design_card
aliases: [运放怎么选, 运放选型, 高速运放选择]
tags: [opamp, device_selection]
summary: 先算 GBW、SR、范围、负载和误差，再选择器件类别而非背型号。
status: ENGINEERING_GUIDE
---

# 电赛运放参数需求到器件要求

## 五个硬门槛

1. `GBW ≥ fmax×noise_gain×margin`，初筛 margin 常取 10。
2. `SR ≥ 2πfmaxVout,pk×margin`。
3. 输入共模和输出摆幅在 min/max 供电、负载下合法。
4. 输出电流覆盖电阻/电容负载与多路 fanout。
5. Vos、Ib、噪声、THD、CMRR/PSRR进入总误差预算。

## 500 kHz、2 Vpp 输入、增益 10 示例

理想输出 20 Vpp，先发现摆幅需求已很高：若没有至少约 ±12V 线性输出余量，方案先失败。噪声增益按拓扑为 10 或 11；经验 GBW 约 50～55MHz。输出峰值 10V，理论 SR `≈31.4V/µs`，建议更高。最后才从高速、高摆幅、相应供电且能驱动目标负载的器件中选 exact 型号。

## 类别选择

- 精密 DC/低频：低 Vos、低漂移、低 1/f 噪声。
- 高源阻：FET/CMOS 输入、低 Ib 和低电流噪声。
- MHz 放大/ADC 驱动：高 GBW/SR、低失真、指定容性负载稳定。
- 低压单电源：核对真实 RRI/RRO 范围与负载，不只看标签。

## 比赛建议

优先使用已经备货、封装可焊、供电方便且有官方评估电路的器件。具体推荐前重新查 datasheet；禁止用相邻型号参数替代。

