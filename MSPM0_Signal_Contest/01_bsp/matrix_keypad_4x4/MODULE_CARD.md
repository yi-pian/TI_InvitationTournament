# MODULE CARD: Matrix Keypad 4×4

| 项目 | 内容 |
|---|---|
| 目录 | `01_bsp/matrix_keypad_4x4` |
| 层级 | BSP / 外部输入设备 |
| 作用 | 用 4 行+4 列 GPIO 扫描 16 键，输出消抖后的按下/释放/稳定状态 |
| 输入/输出 | 行驱动与列读取回调 → 16-bit key masks、symbol、ghost 提示 |
| 主头文件 | `signal_matrix_keypad_4x4.h` |
| 依赖 | `signal_status.h`；8 个 GPIO；应用提供扫描周期 |
| SysConfig | 需要：4 个初值高的输出、4 个内部上拉输入 |
| RAM | 动态分配 0；状态与事件仅几十字节 |
| 状态 | `MODULE_STATUS_BUILD_VERIFIED` |
| 独立测试 | PC mock 覆盖行列定位、消抖、press/release、keymap、ghost 提示并已 PASS；TI Arm Clang/聚合链接 PASS |
| 硬件声明 | 尚未连接真实键盘，不高于 BUILD_VERIFIED |
| 唯一源码 | 正式实现只在本目录，应用仅链接、不复制 |
