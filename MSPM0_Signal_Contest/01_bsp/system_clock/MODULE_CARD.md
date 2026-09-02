# MODULE CARD: system_clock

| 项目 | 内容 |
|---|---|
| 目录 | `01_bsp/system_clock` |
| 层级 | BSP 适配层 |
| 作用 | 校验时钟树参数并把目标事件率换算成整数 Timer 周期。 |
| 输入/输出 | 公开结构或调用者缓冲区；无隐藏数据通道 |
| 依赖 | `signal_status.h` |
| RAM | 模块内动态分配 0；数组/工作区由调用者提供，具体大小由 API 的 count/capacity 决定。 |
| 状态 | `MODULE_STATUS_BUILD_VERIFIED` |
| 独立测试 | 纳入 `10_tests/pc` 全库构建；关键数值路径还应按题目范围补充向量和误差上限。 |
| 硬件声明 | 通用代码不绑定 SysConfig 实例名；接到 MSPM0 时由独立平台适配器使用生成宏。当前状态不代表对应外设已实板验证。 |
| 移除 | 删除本目录源文件及上层引用；平台实例按需从 SysConfig 移除 |
