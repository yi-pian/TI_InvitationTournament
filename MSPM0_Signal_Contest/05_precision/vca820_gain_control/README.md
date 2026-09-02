# VCA820 增益控制算法模块

本模块把目标输出峰峰值换算为 VCA820 控制电压，再换算为 DAC12 code。它只处理公式和限幅，不访问 DAC、GPIO、DDS 或 SysConfig；应用层在成功返回后调用 `DL_DAC12_output12()`。

复制到工程 `modules/`：`signal_vca820_gain_control.c/.h` 和公共 `signal_algorithm_status.h`。本模块不需要 SysConfig。先复制 `README_MINIMAL_EXAMPLE.c` 的配置结构和调用形状，再由应用把返回的 `dac_code` 写入已经配置好的 DAC。

配置中的 `dds_vpp_v` 是未经过 VCA820 的 DDS 峰峰值；`gain_max`、控制电压范围和 DAC 满量程必须按实际电路校准。返回 `SIGNAL_ALGORITHM_OK` 后才使用输出 code；参数错误时不要写 DAC。`vctrl0_v`、`vctrl_slope_v` 保留在配置中用于记录题目校准点，当前公式沿用 24_A 母版的 VCA820 反解公式，确保功能不变。

没有改动模块 `.c/.h`，板级验证状态为 `NOT_RUN`。
