---
id: analog.oscilloscope.settings
title: 示波器比赛现场设置
kind: instrument_guide
aliases: [示波器怎么设置, AC DC coupling, 20MHz limit, 探头倍率]
tags: [oscilloscope, settings, trigger, sampling]
summary: 根据测量目标设置耦合、输入阻抗、探头、带宽、采样率、存储深度和触发。
status: ENGINEERING_GUIDE
---

# 示波器比赛现场设置

| 设置 | 什么时候用 | 风险 |
|---|---|---|
| DC coupling | 幅值、offset、削顶、慢漂移 | 大DC上看小AC时分辨率不足 |
| AC coupling | 大DC上的小纹波 | 引入高通，低频幅相失真 |
| 20MHz limit | 低频幅值/纹波降噪 | 会隐藏高频毛刺和变慢方波 |
| 1MΩ | 高阻节点/普通探头 | 高频受探头电容影响 |
| 50Ω | 同轴传输/50Ω系统末端 | 低压源可能过载；不能接高压 |
| ×10探头 | 默认模拟测量 | 示波器通道倍率必须匹配 |
| ×1探头 | 极低幅低频、且可接受大电容 | 易把振荡器/高阻节点拖坏 |
| Average | 稳定重复噪声测量 | 会抹掉随机/偶发毛刺 |
| High Resolution | 提高垂直分辨感受 | 降有效带宽/时间分辨率，依机型 |

## 采样率和存储

至少让最高关心频率有足够样点；看波形常取≥10～20点/周期，看边沿按上升时间而非基波。长时间记录时 `采样率≈存储深度/时间窗`，拉长时基可能自动降采样造成混叠。

## 触发

从边沿触发开始，阈值放在波形中部；噪声触发用迟滞/高频抑制/holdoff而非无脑 average。单次毛刺用 Single 和足够预触发。

## 现场顺序

先 Default/Auto 找到波形→切 DC→确认探头/阻抗→设垂直居中且占6～8格→设2～5周期→边沿触发→按任务选择带宽/平均/FFT。

