---
id: analog.recipe.modulation_analyzer
title: AM/FM/ASK/FSK 识别与解调设计卡
kind: design_recipe
aliases: [调制解调题, AM FM ASK FSK识别, 调制参数估计]
tags: [contest_recipe, modulation, demodulation, agc]
summary: 由输入频带、动态范围和调制类型选择预选、AGC、IQ/包络/鉴频与参数估计链。
status: DRAFT_RECIPE
---

# AM/FM/ASK/FSK 识别与解调设计卡

## 输入与输出

载波/基带范围、调制度/频偏/码率、输入动态范围、邻道/噪声与识别时间。

## 最短信号链

保护/匹配→预选带通→LNA/VGA→ADC或混频到IF/IQ→载波/包络/瞬时频率特征→分类→参数估计/恢复。

## 模拟设计重点

预选防强邻道让前端饱和；AGC只把信号放入ADC范围，不抹掉ASK幅度特征，识别窗内可冻结增益。混频/本振关注镜像、LO泄漏和低通。

## 方法

AM/ASK：包络/幅度层级；FM/FSK：相位差分/瞬时频率或鉴频；最终算法需用现有FFT/相位/Lock-in primitive组合，不能凭标签判断。

## 调试

先单一无噪调制→逐步加频偏/幅度变化→加邻道/噪声→扫全动态范围。把每级频谱保存为验收点；误识别先查前端削顶/AGC变化，再查分类阈值。

## PCB/供电

本振/ADC时钟远离弱输入；连续地平面和50Ω链；VGA/混频器供电单独滤波。exact RF器件必须查官方datasheet。

