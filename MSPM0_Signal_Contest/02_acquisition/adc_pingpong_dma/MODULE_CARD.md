# MODULE CARD: adc_pingpong_dma

| 项目 | 内容 |
|---|---|
| 目录 | `02_acquisition/adc_pingpong_dma` |
| 层级 | 采集层 |
| 作用 | 管理双缓冲 DMA 的 ready、release 和 overrun 状态。 |
| 输入/输出 | 公开结构或调用者缓冲区；无隐藏数据通道 |
| 依赖 | `signal_status.h` |
| RAM | 调用者提供 2×N×2 bytes 原始缓冲区；控制结构为常数大小。 |
| 状态 | `MODULE_STATUS_BUILD_VERIFIED` |
| 独立测试 | 纳入 `10_tests/pc` 全库构建；关键数值路径还应按题目范围补充向量和误差上限。 |
| 硬件声明 | 通用代码不绑定 SysConfig 实例名；接到 MSPM0 时由独立平台适配器使用生成宏。当前状态不代表对应外设已实板验证。 |
| 移除 | 删除本目录源文件及上层引用；平台实例按需从 SysConfig 移除 |
