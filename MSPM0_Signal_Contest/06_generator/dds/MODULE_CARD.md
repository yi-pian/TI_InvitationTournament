# MODULE CARD: dds

| 项目 | 内容 |
|---|---|
| 目录 | `06_generator/dds` |
| 层级 | 波形生成层 |
| 作用 | 用 32 位相位累加器和 2^n 波表生成任意配置频率。 |
| 输入/输出 | 公开结构或调用者缓冲区；无隐藏数据通道 |
| 依赖 | `signal_status.h` |
| RAM | 模块内动态分配 0；数组/工作区由调用者提供，具体大小由 API 的 count/capacity 决定。 |
| 状态 | `MODULE_STATUS_BUILD_VERIFIED` |
| 独立测试 | 纳入 `10_tests/pc` 全库构建；关键数值路径还应按题目范围补充向量和误差上限。 |
| 硬件声明 | 无寄存器、引脚或 SysConfig 依赖，可在 PC 上独立测试。 |
| 移除 | 删除本目录源文件及上层引用；平台实例按需从 SysConfig 移除 |
