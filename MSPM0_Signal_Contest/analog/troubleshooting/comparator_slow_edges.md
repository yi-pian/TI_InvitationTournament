---
id: analog.troubleshooting.comparator_slow_edges
title: 比较器边沿慢、抖动与丢脉冲故障树
kind: troubleshooting
aliases: [LM393输出慢, 比较器抖动, 比较器丢边沿]
tags: [troubleshooting, comparator, pullup]
summary: 按输出类型、上拉RC、输入过驱、迟滞、带宽、负载和供电定位整形失败。
status: ENGINEERING_GUIDE
---

# 比较器边沿慢、抖动与丢脉冲故障树

1. 查输出是开漏/开集还是推挽；开漏必须上拉。
2. 估 `tr≈2.2RpullupCload`，减小上拉或电容并检查下拉电流。
3. 测输入阈值处斜率与过驱；小信号/慢正弦传播延迟更大。
4. 毛刺则加适量迟滞和模拟带限；迟滞过大又会产生相位/占空误差。
5. 查逻辑接收门限、供电去耦、探头负载和面包板寄生。
6. 高频丢脉冲还要查最小脉宽、恢复时间和Timer输入路由。

