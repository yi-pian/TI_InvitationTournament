---
id: analog.quick.troubleshooting
title: 模拟故障 30 秒速查
kind: quick_reference
aliases: [模拟排错速查]
tags: [quick_reference, troubleshooting]
summary: 用先静态、后动态、逐级注入和二分法快速定位模拟链故障。
status: ENGINEERING_GUIDE
---

# 模拟故障 30 秒速查

```text
目检/断电阻值 → 芯片脚电源 → 静态偏置
→ 低频小信号逐级 → 真实负载 → 提频 → 提幅 → 自动控制
```

- 无输出：电源→共模→摆幅→反馈→负载→振荡。
- 增益错：电阻→源阻/终端→负载→GBW→SR→削顶。
- 高频小：RC→GBW→SR→ADC建立→探头→传输/PCB。
- 毛刺：探头地→电源地→比较器迟滞→数字时钟/DC-DC。
- ADC错：先量ADC脚与VREF，再查采样时间和反算。

只修改第一个偏离理论值的级。

