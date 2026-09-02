# ADC_DMA 两级准入测试计划

当前不进入任何测量/DSP/波形发生正式模块。LEVEL 1 使用板载 TMP6131/PB24/ADC0.5；LEVEL 2 回到 PA25/ADC0.2 动态输入。

| 用例 | 配置/输入 | 观察量 | 通过标准 | 当前状态 |
|---|---|---|---|---|
| T01 SysConfig | 默认配置 | 严格生成、Event.dot | 无 warning/error；Timer publisher 1 -> ADC subscriber 1 | 已通过 |
| T02 默认编译 | 100 kSPS/N=1024 | TI Clang、link | `-Wall -Werror` 无错误，完整链接 | 已通过 |
| T03 编译矩阵 | N=256/512/1024/2048/4096 × Fs=100/200/500 kSPS | 15 个固件配置 | 全部编译和链接成功 | 已通过 |
| T04 TMP6131 路径 | J9 1-2、J13 ON、PB24/ADC0.5 | raw min/max/mean | 12-bit、非全 0/4095、mean 1000..3100 | 待用户实板 |
| T05 五档 N | 256/512/1024/2048/4096 | 0xFFFF 哨兵 | 每档完整覆盖 | 待用户实板 |
| T06 重复启动/WFE | 每档 100 blocks | CCS 验收变量 | 共 500 次 Start->Done，WFE 均完成 | 待用户实板 |
| T07 LFXT/FCC | 32768 Hz LFXT 对 SYSOSC | FCC count/估算频率 | SYSOSC 在 31.2..32.8 MHz；触发率约 100 kSPS | 待用户实板 |
| T08 UART CSV | XDS110 UART0 115200 | 1025 行 CSV | header 正确、INDEX 0..N-1、raw 0..4095 | 待用户实板 |
| T09 PC 绘图 | UART CSV | Python 图与统计 | count/min/max/mean 与 CCS 一致 | 脚本语法/解析已验证；真实 CSV 待实板 |
| T10 LEVEL 2 环回 | PA15/DAC_OUT -> PA25/ADC0.2 | samples/cycle | 100k/1k=100，200k/1k=200，500k/10k=50 | 等待至少一根杜邦线 |

T04~T07 通过后状态为 `BOARD-ONLY VERIFIED`，但 PA25 动态模拟输入仍未验证。T10 或外部已知动态信号通过后才是 `FULL HARDWARE VERIFIED`。
