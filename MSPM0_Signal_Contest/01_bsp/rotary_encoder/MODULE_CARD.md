# MODULE CARD: Rotary Encoder

| 项目 | 内容 |
|---|---|
| 目录 | `01_bsp/rotary_encoder` |
| 层级 | BSP / 人机输入 |
| 作用 | 把机械旋转编码器 A/B 电平序列变成 `-1/0/+1` 步进，并对可选按压键消抖 |
| 输入/输出 | 3 个 GPIO 电平回调 → step、累计 position、按键事件、非法跳变计数 |
| 主头文件 | `signal_rotary_encoder.h` |
| 依赖 | `signal_status.h`；A/B 两个 GPIO input，可选 SW GPIO input |
| SysConfig | 需要 GPIO input 与和实物相符的 pull-up/pull-down；默认先轮询 |
| RAM | 动态分配 0；每实例为常数大小 |
| 状态 | `MODULE_STATUS_BUILD_VERIFIED` |
| 构建证据 | PC 正反转、非法跳变、按键去抖和边界测试 PASS；TI Arm Clang 在真实 TFT SysConfig profile 下完整链接 PASS，细节见 README |
| 硬件声明 | 未读取真实编码器，不能写 BOARD_VERIFIED |
