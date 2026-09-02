# PROFILE_03_DAC_GENERATOR

TIMG6 通过 Event3 驱动 DAC0 FIFO HWTRIG0，DMA_CH1 以 Full Channel repeat-single 模式提供半字波表，输出 pin 为 PA15。

- 默认更新周期：10 us（100 kSPS configured）
- 固定资源：DAC0、PA15、DMA_CH1、TIMG6、Event3、UART0
- MFPCLK gate：已由 SysConfig 明确启用
- 状态：SysConfig/compile/link PASS；DAC 幅度、失真和实际更新率未实板验证
