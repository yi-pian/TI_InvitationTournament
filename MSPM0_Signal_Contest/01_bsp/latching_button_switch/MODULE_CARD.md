# MODULE CARD: Latching Button Switch

| 项目 | 内容 |
|---|---|
| 目录 | `01_bsp/latching_button_switch` |
| 层级 | BSP / 外部输入设备 |
| 作用 | 读取机械自锁开关，首次同步物理状态并输出 ON/OFF 转换事件 |
| 输入/输出 | GPIO 逻辑回调 → stable_on/turned_on/turned_off |
| 主头文件 | `signal_latching_button_switch.h` |
| 依赖 | `signal_status.h`；1 个 GPIO input，通常启用 pull-up |
| SysConfig | 需要：Digital Input + pull-up；不要求 IRQ |
| RAM | 动态分配 0；每实例几十字节 |
| 状态 | `MODULE_STATUS_BUILD_VERIFIED` |
| 构建证据 | PC mock PASS；TI Arm Clang `-Wall -Werror` PASS；44 模块聚合链接 PASS |
| 硬件声明 | 本轮未连接真实自锁开关，不高于 BUILD_VERIFIED |
| 唯一源码 | 应用链接本目录 `.c`；LED 驱动不属于本输入模块 |
