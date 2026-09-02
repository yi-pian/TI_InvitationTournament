# Wave Output MSPM0G3507

- 类别：发生层便捷封装
- 输入：频率 Hz、峰峰值 V、偏置 V
- 输出：MSPM0G3507 DAC0，通过 Timer/Event/DMA 连续输出
- 依赖：Sine、Square、Triangle、Sawtooth、DAC Wave Table、DDS、DAC DMA
- SysConfig：完全沿用 DAC DMA README
- RAM：由调用者提供波表和 DMA 输出缓冲
- 状态：BUILD_VERIFIED；实板输出质量取决于 DAC 更新率和模拟重建滤波器
