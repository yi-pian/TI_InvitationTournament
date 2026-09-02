---
id: analog.recipe.opamp_tester
title: 运放参数自动测试仪设计卡
kind: design_recipe
aliases: [运放参数测试题, 测GBW SR CMRR PSRR]
tags: [contest_recipe, opamp, measurement]
summary: 把GBW、SR、Iq、Vos、Ib、摆幅、CMRR和PSRR拆成可切换测试网络与分级验证流程。
status: DRAFT_RECIPE
---

# 运放参数自动测试仪设计卡

## 输入与输出

待测运放供电/封装/稳定增益范围；输出GBW、SR+/SR-、Iq、Vos、Ib、摆幅、CMRR/PSRR及不确定度/限制。

## 原理与测试网络

DDS/方波/可调DC→模拟开关选择闭环增益、负载、共模和电源扰动→DUT→量程/双ADC→对应Measurement Recipe。不同参数不能共用一个万能接法。

## 关键公式

`GBW≈noise_gain·f-3dB`（单极点近似）；`SR=ΔV/Δt`；`Pq=V+I++|V-|I-`。Vos/Ib/CMRR/PSRR见 `../measurement/opamp_dc_parameters.md`。

## 元件选择

测试源/ADC/开关的带宽、失真和漏电要优于DUT；标准电阻0.1%或更好。高速SR路径少用高电容模拟开关，µV/pA测量要考虑板漏电。

## 供电/接口

可调双电源需限流、极性保护和稳定；切换前把激励归零。MCU状态机记录当前网络、量程、校准版本与失败状态。

## 调试

先用已知好运放逐个手动测试→验证开关网络不改变结果→自动化→重复性/温漂。普通电赛仪器难以给datasheet级Ib/CMRR/PSRR，报告限制而非假精度。

## 现有入口

历史 `fuxian/24_A` 仅作需求证据；当前Example状态 `NOT_RUN`。正式测量链见 `../measurement/opamp_gbw.md`、`opamp_slew_rate.md`、`opamp_dc_parameters.md`。

