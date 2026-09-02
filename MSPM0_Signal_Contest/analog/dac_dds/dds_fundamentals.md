---
id: analog.dds.fundamentals
title: DDS 频率字、相位累加器与 DAC 的关系
kind: knowledge
aliases: [DDS原理, DDS频率控制字, 相位累加器]
tags: [dds, phase_accumulator, ftw, dac]
summary: 说明 DDS 如何从参考时钟、频率字、相位到 DAC 样值，并给出量化与镜像边界。
status: ENGINEERING_GUIDE
---

# DDS 频率字、相位累加器与 DAC 的关系

N 位相位累加器每个参考时钟加 FTW：

```text
fout = FTW × fclk / 2^N
FTW = round(fout × 2^N / fclk)
```

高位相位查正弦表/算法得到幅值码，DAC 变成模拟台阶，再由重建滤波器抑制镜像。

## 为什么 DDS 已带 DAC 还可能需要独立 DAC

DDS 内部 DAC通常为固定架构，适合正弦/相位合成；独立 DAC用于：

- 独立控制幅度、偏置或 VGA 控制电压。
- 产生任意波形或第二路同步模拟量。
- 获得不同分辨率、输出范围、低频精度或负载驱动。

若只需 DDS 正弦幅度可调，通常用外部 VGA/PGA/衰减器，而不是再用 DAC 重新“生成同一正弦”。

## 失败点

高 fout 虽数学可设，但谱纯度受相位截断、DAC、时钟抖动和滤波限制；模块上的晶振与滤波器也会限制实际频率与幅度。

