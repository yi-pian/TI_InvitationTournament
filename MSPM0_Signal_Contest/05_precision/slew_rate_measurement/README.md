# 压摆率边沿时间算法模块

本模块把一帧已经换算为电压的方波样本，按 20% 到 80% 阈值寻找上升/下降边沿，并输出平均上升时间和下降时间。压摆率 `0.6 * Vpp / time` 仍由 24_A 应用根据题目输出缩放计算。

复制 `signal_slew_rate.c/.h` 和 `signal_algorithm_status.h` 到工程 `modules/`，再从 `README_MINIMAL_EXAMPLE.c` 复制配置和调用。模块不需要 SysConfig，不访问 ADC/DMA/GPIO。调用必须发生在采样完成且输入数组只读时。

参数 `low_ratio/high_ratio` 建议为 0.20/0.80；`sample_rate_hz` 必须是真实 ADC 采样率。结果计数为零时返回 `SIGNAL_ALGORITHM_INSUFFICIENT_DATA`，不能把 0 us 当成有效边沿。模块 `.c/.h` 冻结，板级状态 `NOT_RUN`。
