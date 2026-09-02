---
id: analog.filter.sallen_key_vs_mfb
title: Sallen-Key 与 MFB 二阶节选择
kind: design_card
aliases: [Sallen-Key, MFB滤波器, 二阶有源滤波]
tags: [filter, sallen_key, mfb, opamp]
summary: 按输入阻抗、Q、增益、反相和运放要求选择二阶有源拓扑。
status: ENGINEERING_GUIDE
---

# Sallen-Key 与 MFB 二阶节选择

## 快速选择

- Sallen-Key：非反相、输入阻抗高、结构直观；高 Q 时对运放 GBW和元件比敏感，输出直接耦合回网络，源阻不可忽略。
- MFB：反相、较容易实现较高 Q，求和节点便于设增益；输入阻抗由电阻决定，公式更依赖拓扑与设计表。

## 不能通用抄值

二阶节的 `f0`、Q、增益公式取决于具体低通/高通电路和元件命名。必须选定可信设计表或工具，再把结果带回 SPICE/实测；不要混用不同图的 R1/R2/C1/C2 公式。

## 运放要求

高 Q 会放大对有限开环增益的敏感性。初筛至少让 GBW 远高于 `f0×噪声增益×Q`，严格值按厂商设计指南/仿真。SR 按最大内部/输出峰值计算。

## 调试

单独调每个二阶节：测通带增益、f0/Q、相位，再级联。峰化过高先查元件装错与 Q 节顺序，非先改数字滤波。

