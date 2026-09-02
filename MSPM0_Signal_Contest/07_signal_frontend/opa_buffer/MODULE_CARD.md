# MODULE CARD: opa_buffer

| 项目 | 内容 |
|---|---|
| 目录 | `07_signal_frontend/opa_buffer` |
| 层级 | 模拟前端配置层 |
| 作用 | 生成 OPA 电压跟随器配置。 |
| 输入/输出 | 公开结构或调用者缓冲区；无隐藏数据通道 |
| 依赖 | `signal_opa.h` |
| RAM | 模块内动态分配 0；数组/工作区由调用者提供，具体大小由 API 的 count/capacity 决定。 |
| 状态 | `MODULE_STATUS_BUILD_VERIFIED` |
| 独立测试 | 纳入 `10_tests/pc` 全库构建；关键数值路径还应按题目范围补充向量和误差上限。 |
| 硬件声明 | 通用代码不绑定 SysConfig 实例名；接到 MSPM0 时由独立平台适配器使用生成宏。当前状态不代表对应外设已实板验证。 |
| 移除 | 删除本目录源文件及上层引用；平台实例按需从 SysConfig 移除 |
