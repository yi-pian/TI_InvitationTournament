# MODULE CARD: spectrum_analyzer

| 项目 | 内容 |
|---|---|
| 目录 | `08_applications/spectrum_analyzer` |
| 层级 | 应用组合层 |
| 作用 | 组合窗、FFT、幅度谱、峰值和抛物线插值。 |
| 输入/输出 | 公开结构或调用者缓冲区；无隐藏数据通道 |
| 依赖 | `signal_fft_peak.h`、`signal_status.h`、`signal_types.h` |
| RAM | 调用者提供 8N bytes FFT 区和约 2N bytes 单边幅度谱；模块内动态分配 0。 |
| 状态 | `MODULE_STATUS_BUILD_VERIFIED` |
| 独立测试 | 纳入 `10_tests/pc` 全库构建；关键数值路径还应按题目范围补充向量和误差上限。 |
| 硬件声明 | 无寄存器、引脚或 SysConfig 依赖，可在 PC 上独立测试。 |
| 移除 | 删除本目录源文件及上层引用；平台实例按需从 SysConfig 移除 |
