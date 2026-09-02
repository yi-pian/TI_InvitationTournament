---
id: analog.filter.rc_basics
title: RC 低通、高通与带通的现场计算
kind: design_card
aliases: [RC低通, RC高通, RC带通, 截止频率]
tags: [filter, rc, cutoff]
summary: 在源阻和负载均计入后计算一阶 RC 截止频率、衰减与相位。
status: ENGINEERING_GUIDE
---

# RC 低通、高通与带通的现场计算

## 核心公式

一阶截止 `fc=1/(2πReqC)`。低通幅值 `|H|=1/sqrt(1+(f/fc)²)`；高通 `|H|=(f/fc)/sqrt(1+(f/fc)²)`。在 fc 处幅值为 0.707，即 -3.01dB。

Req 不是总等于标称 R：低通串联电阻要计入源阻，输出负载会改变分压；高通耦合电容看到的是两侧等效电阻之和/组合。

## 示例

R=3.3kΩ、C=1nF，理想 `fc≈48.2kHz`。若信号源再有 50Ω影响小；若负载也是 3.3kΩ，通带增益与有效极点都会变化，需用完整网络算。

## RC 带通

级联高通与低通，要求 `fL` 明显低于 `fH`；两级会互相加载时加缓冲。总相位是各级相位相加。

## 调试

固定小信号扫频，CH1/CH2 同步测输入输出。fc 应是相对通带基准下降 3dB，不是相对信号源面板值。

