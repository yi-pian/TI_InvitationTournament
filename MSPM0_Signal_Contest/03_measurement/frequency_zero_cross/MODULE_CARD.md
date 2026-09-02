# MODULE CARD

MODULE: Zero Cross

CATEGORY: Measurement / Frequency Front-end

功能：查找信号穿过用户阈值的相邻样本对，支持上升、下降、双方向和滞回重新武装。

输入：`voltage_v[]`、`count`、`threshold_v`、`hysteresis_v`、方向、事件工作区。

输出：`signal_zero_cross_event_t[]` 及事件计数；索引单位 sample。

是否原地处理：NO；只读输入，写事件数组。

依赖：公共算法状态码。

典型用途：高 SNR 周期信号的频率/相位测量前端。

不要用于：低 SNR 大量假过零、阈值未知、严重多峰波形却假设每周期只有一次过零。

计算量：LOW，O(N)。

RAM：模块内部 O(1)；事件数组约 `sizeof(event)*capacity`，PC GCC 当前通常 12 字节/事件，目标编译器以 `sizeof` 为准。

Benefits：阈值和滞回明确；不假设中心永远为 0；不直接把整数样本当精确过零时间。

Trade-offs：噪声/谐波能改变过零；事件工作区可能不足；滞回太大可能漏检小信号。

可连接：`RemoveDC/Voltage -> ZeroCross -> ZeroCrossInterpolation -> MultiCycleAverage`。

状态：PC_VERIFIED。2026-08-07 通过有 DC、去 DC、滞回和事件计数测试；未实板验证。
