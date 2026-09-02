---
id: analog.recipe.signal_source
title: 可编程信号源/DDS 设计卡
kind: design_recipe
aliases: [信号发生器题, DDS信号源, 任意波形发生]
tags: [contest_recipe, dds, dac, signal_source]
summary: 从频率/波形/幅度指标选择软件DDS、外置DDS或高速DAC，并设计重建和输出驱动。
status: DRAFT_RECIPE
---

# 可编程信号源/DDS 设计卡

## 输入与输出要求

列波形、fmin/fmax/步进、Vpp/offset、负载、THD/SFDR、扫频速度与相位连续性。

## 原理与核心公式

软件/硬件相位累加→DAC→重建低通→幅度控制→输出buffer/50Ω驱动。`FTW=round(fout·2^N/fclk)`；`SR≥2πfVpk`。

## 选择

低中频任意波且每周期点数足：内部DAC+DMA；kHz～低MHz正弦：AD9833类；更高频：AD9850类/更高速DDS；真正任意高速波形：高速DAC+存储/FPGA。

## 参数示例

1MSPS DAC 输出100kHz只有10点/周期，适合性取决于THD/滤波；500kHz只有2点/周期，不作为高质量正弦方案。

## 连接、阻抗与电源

DDS模块输出幅度/偏置先实测；后级保持线性，50Ω源需考虑串联终端和双倍开路电压。时钟、DAC和运放分别去耦，输出SMA有连续回流。

## MCU接口

软件DDS占Timer/DMA/DAC/RAM；外置DDS只写控制字但仍需SPI/GPIO与更新时序。exact API见当前头文件/设备卡。

## 调试与故障

先固定低频/高阻→验证频率字→测DAC裸输出→加滤波→加幅度/负载。频率对幅度错查模块滤波；频谱杂散查时钟/电源/布局；高幅高频削顶查SR/摆幅。

## 替代与实例

只需方波优先Timer；只需DC优先DAC。链接 `../dac_dds/` 与现有 `08_applications/dds_generator`、`sweep_analyzer`。

