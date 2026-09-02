# Integration Glue Module Card

- 正式源码：`../signal_integration.c`、`../signal_integration.h`
- 定位：应用层 Glue/Adapter；复用正式算法，不是第二套算法实现。
- 输入：ADC raw 或 float buffer、N/Fs、调用者 workspace。
- 输出：meter、spectrum、THD、dual-phase result structs。
- SysConfig：自身不需要；上游采集按 P01/P02/P06 配置。
- Buffer：无动态内存；部分处理原地覆盖 voltage/FFT/magnitude workspace。
- 典型上游：ADC DMA、Dual ADC Platform。
- 典型下游：UART/TFT/应用结果层。
- 详细说明：[README.md](README.md)
