# PROFILE_01_ADC_CAPTURE

单路 PA25/ADC0.2 block capture 基线。ADC0 由 TIMG0 通过 Event1 触发，DMA_CH0 写 RAM；UART0 PA10/PA11 为可删除的 debug 输出。

- 默认周期：10 us（100 kHz configured trigger rate）
- 固定资源：ADC0、DMA_CH0、TIMG0、Event1、PA25、PA10、PA11
- 可直接适配：`02_acquisition/adc_dma`
- 状态：SysConfig/compile/link PASS；此 profile 未单独烧板

不要把名称中的 CAPTURE 误解为 Timer edge capture。
