---
id: analog.quick.adc_frontend
title: ADC 前端 30 秒速查
kind: quick_reference
aliases: [ADC前端速查]
tags: [quick_reference, adc]
summary: 快速检查范围、偏置、保护、抗混叠、driver、采样与校准。
status: ENGINEERING_GUIDE
---

# ADC 前端 30 秒速查

```text
输入范围/offset → 衰减与偏置 → 运放共模/摆幅
→ 过压限流/钳位 → 抗混叠 → ADC driver/RC
→ Fs/fmax点数 → ENOB/参考 → 码值反算
```

- ±5V到0～3.3V示例：`Vadc=0.3Vin+1.65V`，留端点余量。
- ADC输入有采样电容；降Fs/延长采样后变准＝驱动建立问题。
- 模拟滤波防混叠/饱和，数字滤波不能救已折叠信号。
- 先示波器确认ADC脚无削顶，再看算法。

