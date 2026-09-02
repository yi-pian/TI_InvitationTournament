---
id: analog.adc.protection
title: ADC 输入限幅、ESD 与钳位保护
kind: design_card
aliases: [ADC输入保护, 过压钳位, ESD保护]
tags: [adc, protection, clamp, esd]
summary: 在不显著破坏带宽和失真的前提下限制故障电流与瞬态电压。
status: ENGINEERING_GUIDE
---

# ADC 输入限幅、ESD 与钳位保护

## 三层保护

1. 接口级 ESD/浪涌：低电容 TVS 或专用保护器，按最大能量和带宽选。
2. 限流：串联电阻限制内部/外部钳位电流。
3. 精密钳位：肖特基/低漏电器件或有源限幅，将节点限制在 ADC 安全范围附近。

## 快算

故障输入 `Vfault`、钳位约 `Vclamp`、允许钳位电流 `Iclamp,max`：

```text
Rseries ≥ (|Vfault|-Vclamp)/Iclamp,max
```

同时检查正常信号下 `Rseries` 与 ADC/外接电容的低通和建立误差。

## 常见错误

- 只靠 MCU 内部 ESD 二极管长期吸收过压。
- 钳位到 3.3V 轨导致断电反向供电或把数字噪声注入模拟输入。
- 选大电容 TVS 让 MHz 信号幅值/相位失真。

## 验证

先用限流电源逐步增加超范围输入，测钳位节点和电流；再在正常 min/max 频率检查增益、THD和输入电容。保护验证不等于可故意超绝对最大额定值。

