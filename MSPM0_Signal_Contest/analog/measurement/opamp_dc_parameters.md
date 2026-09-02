---
id: analog.measurement.opamp_dc_parameters
title: 运放静态功耗、失调、偏置、摆幅、CMRR 与 PSRR 测量卡
kind: measurement_recipe
aliases: [测运放静态功耗, 测输入失调, 测CMRR, 测PSRR]
tags: [measurement, opamp, dc_parameters]
summary: 给出普通实验室可行的近似测量与哪些指标难以达到datasheet级精度。
status: DRAFT_RECIPE
---

# 运放静态参数测量卡

## 静态功耗

输入置于合法共模、输出无负载静止；在每条电源线上串电流表/小采样电阻。双电源 `Pq=V+I+ + |V-|I-`；单电源 `Pq=Vsupply·I`。先扣除板上其他电路电流，避免示波器/负载让输出动态耗电混入。

## 输入失调

用高闭环增益 DC 电路放大 Vos，输入短接并匹配两输入看到的电阻，测输出后除以噪声增益。必须避开输出饱和和温漂；普通万用表/电阻漂移限制 µV级准确度。

## 输入偏置电流

在输入端切换已知大电阻，测输出变化反算 `Ib≈ΔV/(R·noise_gain)`。板面漏电、湿度和仪表输入电流常与 pA/nA 同量级，普通电赛板难以准确测超低 Ib。

## 输出摆幅

固定负载，缓慢增加低频输入，记录失真/增益偏离前的最大最小输出；同时监测输出电流与温升。必须报告供电、负载和频率。

## CMRR/PSRR

CMRR：两输入同相扫共模，测等效输入误差；PSRR：保持输入不变，对电源叠加小扰动并测输出。两者需要高动态范围、隔离良好的源和差分测量；普通台式环境只能做筛选级结果，不能轻易宣称 datasheet 精度。

## 比赛快速版

能稳定测的先做 Iq、Vos、摆幅；Ib/CMRR/PSRR明确写仪器与漏电/注入限制，并用已知好器件对照。

