# MODULE CARD: dds_generator

| 项目 | 内容 |
|---|---|
| 目录 | `08_applications/dds_generator` |
| 层级 | 应用组合层 |
| 作用 | 组合正弦波表和 DDS 初始化。 |
| 输入/输出 | 公开结构或调用者缓冲区；无隐藏数据通道 |
| 依赖 | `signal_dds.h` |
| RAM | 模块内动态分配 0；数组/工作区由调用者提供，具体大小由 API 的 count/capacity 决定。 |
| 状态 | `MODULE_STATUS_BUILD_VERIFIED` |
| 独立测试 | 纳入 `10_tests/pc` 全库构建；关键数值路径还应按题目范围补充向量和误差上限。 |
| 硬件声明 | 无寄存器、引脚或 SysConfig 依赖，可在 PC 上独立测试。 |
| 移除 | 删除本目录源文件及上层引用；平台实例按需从 SysConfig 移除 |
