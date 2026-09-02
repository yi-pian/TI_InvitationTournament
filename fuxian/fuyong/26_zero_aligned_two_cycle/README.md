# 26_zero_aligned_two_cycle

固定显示指定周期数、并让 X 轴左右边界落在同一相位（上升过零点）的复用代码。

它组合了 `11_zero_cross_frequency` 的过零插值和 `23_trigger_capture` 的窗口截取思路。复制 `zero_aligned_two_cycle.c` 中的结构体与函数到目标工程；再在绘图时从 `start_sample` 均匀插值到 `end_sample`。

周期数由文件顶部的 `ZERO_ALIGNED_CYCLE_COUNT` 决定：设置为 `3U` 就显示三个周期。显示 N 个周期需要 N+1 个上升过零点；若采样缓冲区容不下它们，函数会返回 `false`，调用端应保留原始整帧显示作为回退。

对于双通道相位测量，应只用 X 通道确定这个公共时间窗，X 的左右端点会是 0 V，Y 维持真实相位差。若分别将 X、Y 对齐至自己的零点，两个波形都能从 0 开始，但相位关系会被改变。
