---
id: analog.dac.output_buffer
title: DAC 电压/电流输出与缓冲设计
kind: design_card
aliases: [DAC输出缓冲, 电流输出DAC, 电压输出DAC]
tags: [dac, buffer, i_v, load]
summary: 区分电压型和电流型 DAC，按负载、建立、摆幅和稳定性设计输出级。
status: ENGINEERING_GUIDE
---

# DAC 电压/电流输出与缓冲设计

## 先识别输出形式

- 电压输出 DAC：内部已有 I/V，仍可能需要运放隔离低阻/容性负载。
- 电流输出 DAC：必须按 datasheet 用负载电阻/跨阻放大器把电流变电压，注意 compliance range。
- R-2R 裸网络：输出阻抗、开关毛刺和逻辑供电直接影响线性，不等同于精密 DAC。

## 缓冲器要求

输入范围覆盖 DAC 输出；Vos/噪声/失真满足目标；SR 与 full-scale settling 满足更新；输出可驱动负载且对电容稳定。高速 DAC 常需差分 I/V 和专用变压器/放大器，不用慢精密运放硬接。

## 示例

3.3V、12bit 电压 DAC 驱动 1kΩ 负载，满量程约需 3.3mA，若 DAC 推荐仅高阻或约 mA 级，必须缓冲。理想 LSB `3.3/4096≈0.806mV`，但 INL、参考和运放误差决定实际精度。

## 调试

先高阻测 DAC 本体阶梯/DC，再接缓冲和负载；输出一组 0%、25%、50%、75%、100%码做线性标定。满幅跳变不到位查负载/建立，振铃查运放稳定性与探测方式。

