---
id: analog.quick.impedance
title: 阻抗与 50Ω 30 秒速查
kind: quick_reference
aliases: [50欧速查, 负载效应速查]
tags: [quick_reference, impedance]
summary: 快速判断面板幅值、多个负载和输入输出阻抗测量。
status: ENGINEERING_GUIDE
---

# 阻抗与 50Ω 30 秒速查

```text
Vload=Vth·RL/(Rs+RL)
Rparallel=1/(Σ1/Ri)
```

- 发生器50Ω显示1Vpp→50Ω负载约1Vpp→1MΩ约2Vpp。
- 两个50Ω并联=25Ω，不是“各自50Ω所以不变”。
- 一个源多路：高阻可并，50Ω/大电容用分配/独立buffer。
- 测Zin：`Rs·V2/(V1-V2)`；测Zout：`RL(V0/VL-1)`。
- 差2倍先查终端与探头倍率，不先改软件。

