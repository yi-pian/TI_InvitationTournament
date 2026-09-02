---
id: analog.power.decoupling
title: 模拟信号链去耦与电源滤波
kind: design_card
aliases: [100nF怎么放, 运放去耦, ADC电源滤波]
tags: [power, decoupling, ldo, dcdc]
summary: 用本地高频去耦、级间储能和低噪声稳压控制电源阻抗与回流面积。
status: ENGINEERING_GUIDE
---

# 模拟信号链去耦与电源滤波

## 三个时间尺度

- 100nF 低ESR陶瓷：每个IC每条电源脚，数毫米内，最小环路，处理高频瞬态。
- 1µF：本地中频储能/参考去耦，按器件稳定性要求。
- 10µF或更大：分区/连接器入口的低频负载变化；不是越大越好，LDO稳定性和启动需核对。

## LDO 与 DC/DC

DC/DC效率高但有开关纹波/磁场；敏感模拟链常用 DC/DC→LC/磁珠（需阻尼）→低噪声LDO。磁珠不是万能隔离，错误LC会共振。

## PCB

电容先接电源脚再接低阻地平面；不要用长细线把“100nF”接到远处。ADC参考、运放与DDS时钟的回流不要穿过屏幕/数字开关电流路径。

## 验证

示波器短地/同轴测电源脚附近，DC coupling看跌落，AC coupling看纹波；同时观察负载切换。普通长地探头会夸大尖峰。

