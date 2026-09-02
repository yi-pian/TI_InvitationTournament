# PROFILE_02_DUAL_ADC

同一 TIMG0 周期向 Event1/Event2 发布，使 ADC0.2/PA25 和 ADC1.2/PA17 各自触发，并分别由 DMA_CH0/DMA_CH1 写入 RAM。

- 默认周期：10 us（每通道 100 kSPS configured）
- 固定资源：ADC0、ADC1、DMA_CH0、DMA_CH1、TIMG0、Event1/2、PA25、PA17、UART0
- 数据整合：应用创建两个 frame；现有 `adc_dual_sync` 仅负责交织数据拆分
- 状态：SysConfig/compile/link PASS；双通道同步精度未实板标定
