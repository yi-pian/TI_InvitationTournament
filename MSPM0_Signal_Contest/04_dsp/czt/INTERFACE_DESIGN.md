# Interface Design（不可调用）

计划 config：input_count、output_count、sample_rate_hz、start_frequency_hz、frequency_step_hz、scale strategy。

计划 API：调用者传 real/complex input、complex output、明确大小的 complex workspace 和预计算计划；不允许 malloc 或隐藏 4K 数组。

输出必须说明 FFT 符号、幅值标度和频率映射。
