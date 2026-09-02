# MODULE CARD: Button

| 项目 | 内容 |
|---|---|
| 目录 | `01_bsp/button` |
| 层级 | BSP / 外部或板载输入设备 |
| 作用 | 读取普通瞬时按键，输出消抖后的 pressed/released/stable 状态 |
| 输入/输出 | GPIO 逻辑回调 → `signal_button_event_t` |
| 主头文件 | `signal_button.h` |
| 依赖 | `signal_status.h`；1 个 GPIO input，通常启用 pull-up |
| SysConfig | 需要：Digital Input + pull-up；不要求 IRQ |
| RAM | 动态分配 0；每个实例几十字节 |
| 状态 | `MODULE_STATUS_BUILD_VERIFIED` |
| 构建证据 | PC mock PASS；TI Arm Clang `-Wall -Werror` PASS；44 模块聚合链接 PASS |
| 硬件声明 | 本轮未读取真实按钮，不高于 BUILD_VERIFIED |
| 唯一源码 | 应用链接本目录 `.c`，多按键只创建多个实例 |
