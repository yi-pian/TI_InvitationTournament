# MODULE CARD: adc_dual_sync

| 项目 | 内容 |
|---|---|
| 目录 | `02_acquisition/adc_dual_sync` |
| 层级 | 采集层 |
| 作用 | 把同步双通道交织数据拆成两个独立数组。 |
| 输入/输出 | 公开结构或调用者缓冲区；无隐藏数据通道 |
| 依赖 | `signal_status.h` |
| RAM | 模块内动态分配 0；数组/工作区由调用者提供，具体大小由 API 的 count/capacity 决定。 |
| 状态 | `MODULE_STATUS_BUILD_VERIFIED` |
| 独立测试 | 纳入 `10_tests/pc` 全库构建；关键数值路径还应按题目范围补充向量和误差上限。 |
| 硬件声明 | 通用代码不绑定 SysConfig 实例名；接到 MSPM0 时由独立平台适配器使用生成宏。当前状态不代表对应外设已实板验证。 |
| 移除 | 删除本目录源文件及上层引用；平台实例按需从 SysConfig 移除 |

24_C 用法：ADC0 DMA 保存模拟波形，ADC1 DMA 只作猝发 marker；两路由同一 Timer Event 同步触发，应用用三缓冲锁存完整 ADC0 猝发窗口。ISR 只切换 buffer，不做 FFT/TFT。
