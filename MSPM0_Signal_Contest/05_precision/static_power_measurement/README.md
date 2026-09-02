# 静态功耗换算算法模块

本模块把分流电阻电压换算为电流和功耗：`I=Vshunt/R`，`P=I*Vsupply*rail_count`。ADC 采样、平均值和 DDS 关断仍由 24_A 应用负责；模块只做无副作用的标量计算。

复制 `signal_static_power.c/.h` 和 `signal_algorithm_status.h` 到工程 `modules/`，按 `README_MINIMAL_EXAMPLE.c` 调用。模块不需要 SysConfig。分流电阻、供电电压和电源轨数量必须填实测电路参数；返回成功后才显示输出。模块 `.c/.h` 冻结，板级状态 `NOT_RUN`。
