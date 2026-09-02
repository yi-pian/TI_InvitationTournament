---
id: analog.adc.input_drive
title: ADC 输入不是简单电阻：驱动与 RC 隔离
kind: knowledge
aliases: [ADC输入阻抗, ADC采样电容, 运放驱动ADC]
tags: [adc, sample_hold, driver, settling]
summary: 用采样电容的瞬态充电模型选择源阻、采样时间、缓冲器和隔离 RC。
status: ENGINEERING_GUIDE
---

# ADC 输入不是简单电阻：驱动与 RC 隔离

## 现象

万用表测电压正确，ADC 高速采样却偏低、通道间串扰或码值随采样时间变化。

## 原因

SAR/流水线 ADC 的采样开关周期性给内部电容充电，输入电流是脉冲而非恒定电阻电流。前端必须在 acquisition time 内建立到目标误差：

```text
误差约 ∝ exp(-Tacq/(Rsource_total·Csample_total))
```

N 位约需多个时间常数；精确要求按 exact datasheet 的驱动章节。

## 设计

- 降低源阻或延长采样时间。
- 用稳定的 ADC driver/运放缓冲；GBW、SR、输出恢复和容性负载都要满足。
- ADC 引脚前串 10～100Ω隔离开关回冲，近端并几十 pF～几 nF 作 charge bucket；数值必须兼顾抗混叠与建立。

## 调试

降低采样率/延长采样时间若误差明显改善，优先怀疑驱动建立。示波器用低电容探头看 ADC 引脚；普通 ×1 探头会加重问题。

