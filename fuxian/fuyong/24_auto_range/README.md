# 24_auto_range

以每帧物理电压的最小值和最大值计算显示中心 `display_center_v`，再由半峰峰值计算软件显示半量程 `display_half_range_v`，并输出可连接到硬件 PGA/模拟开关的 `hardware_gain_index`。

`AUTO_RANGE` 输入 `voltage_samples[]`（V），输出显示中心、显示半量程和建议增益档。绘图换算必须使用 `(value - display_center_v) / display_half_range_v`；这样带 1.65 V ADC 偏置的交流信号也会居中并充分利用屏幕。它不直接配置 GPIO/PGA，硬件增益连接须按你的 SysConfig 与前端电路实现。
