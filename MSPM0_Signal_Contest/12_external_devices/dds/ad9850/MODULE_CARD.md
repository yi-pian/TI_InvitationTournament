# AD9850 Module Card

| 项目 | 内容 |
|---|---|
| 类别 | External DDS / clock generator |
| 正式源码 | `ad9850.c/.h` |
| MSPM0 平台层 | `ad9850_mspm0_platform.c/.h` |
| 主接口 | 4-wire GPIO serial：W_CLK、FQ_UD、DATA、RESET |
| 输入 | reference clock Hz、output frequency Hz、5-bit phase code、power-down |
| 输出 | AD9850 模拟 DDS 输出；具体幅度/滤波由 IC/模块板决定 |
| 依据 | Analog Devices AD9850 Datasheet Rev. H；本地旧 8051/MSPM0 工程只作历史接法参考 |
| 状态 | Core `PC_VERIFIED`；core/platform `BUILD_VERIFIED`；`BOARD_VERIFIED` 尚无新库证据 |
| 重要限制 | 典型模块板晶振、供电、输出网络不统一，使用前必须核对实物 |

