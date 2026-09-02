---
id: analog.troubleshooting.glitches
title: 波形毛刺与振铃故障树
kind: troubleshooting
aliases: [波形有毛刺, 边沿振铃, 数字串扰]
tags: [troubleshooting, glitch, ringing]
summary: 用双探测法和时间相关性区分测量伪影、电源地、比较器迟滞、时钟与开关电源串扰。
status: ENGINEERING_GUIDE
---

# 波形毛刺与振铃故障树

## 快速测试

1. 换成探头弹簧地/同轴；毛刺消失＝探测回路伪影。
2. 同时看模拟信号与电源；同步＝去耦/回流。
3. 改变ADC/DDS/屏幕刷新率；毛刺频率跟随＝数字串扰。
4. 比较器输入阈值附近多翻转＝迟滞/带限不足。
5. 关闭DC/DC或用线性电源；消失＝开关纹波/磁耦合。

## 修复

缩短回流、贴脚去耦、隔离数字与模拟路径、给比较器合适迟滞、给容性负载加隔离电阻、降低高di/dt回路面积。不要用示波器20MHz limit把真实毛刺“修没”。

