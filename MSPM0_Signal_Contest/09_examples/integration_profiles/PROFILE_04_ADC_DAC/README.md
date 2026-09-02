# PROFILE_04_ADC_DAC

把单 ADC 和 DAC generator 放入同一工程，但使用独立 Timer/Event/DMA：ADC0/PA25 使用 TIMG0+Event1+DMA0，DAC0/PA15 使用 TIMG6+Event3+DMA1。

- 默认 ADC/DAC rate：均为 100 kHz configured
- UART0：PA10/PA11，115200，仅 debug
- 用途：未来 PA15→PA25 杜邦线环回、控制/测量闭环原型
- 状态：SysConfig/compile/link PASS；没有杜邦线时不宣称环回已验证
