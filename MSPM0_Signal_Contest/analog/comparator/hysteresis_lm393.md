---
id: analog.comparator.hysteresis_lm393
title: LM393 上拉与施密特迟滞设计
kind: design_card
aliases: [LM393边沿慢, 比较器迟滞怎么算, 上拉电阻怎么选]
tags: [comparator, lm393, hysteresis, open_collector]
summary: 为开集输出选择上拉，并用正反馈建立大于噪声的上下阈值差。
status: ENGINEERING_GUIDE
---

# LM393 上拉与施密特迟滞设计

## 输出为什么慢

LM393 类常为开集/开漏输出，上升沿由 `Rpullup×(Cload+Cprobe)` 充电决定；下降沿由晶体管下拉。没有上拉不会得到可靠高电平。

## 上拉初选

低电平电流 `I≈(Vpullup-Vol)/Rpullup` 必须小于器件保证下拉能力；上升时间近似 `tr≈2.2RpullupCload`（10%～90%）。例如 3.3V、总电容 100pF、希望 tr<200ns，则 `R<909Ω`，但电流约 3.6mA；若器件/功耗不允许，只能放宽速度或减小电容。

## 迟滞

给输出到非反相端正反馈时，上/下阈值由输出高低电平分别代入电阻分压求得：

```text
Vth = (Vin_ref/Rref + Vout/Rfb) / (1/Rref + 1/Rfb)
ΔVhys = Vth_high - Vth_low
```

具体符号随输入端选择而变；必须用真实 `VOH/VOL` 与上拉电压。让 ΔVhys 大于阈值处峰峰噪声并留余量，但过大将引入时间/相位误差。

## 调试

CH1看模拟输入、CH2看输出；用短地线。慢上升先减 C/上拉阻值，毛刺先增加适量迟滞和输入带限；相位测量要标定上下阈值造成的时间偏移。

