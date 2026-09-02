# ADC DMA Minimum

【COMPILE-VERIFIED EXAMPLE】

本工程只含最小应用层 [main.c](main.c)，正式采集实现仍唯一链接 `02_acquisition/adc_dma/signal_adc_dma.c`，SysConfig 复用 `PROFILE_01_ADC_CAPTURE`；没有复制模块源码。

- 模块说明：[ADC DMA README](../../../02_acquisition/adc_dma/README.md)
- Platform：ADC DMA 模块自身是正式 MSPM0 Timer/Event/ADC/DMA 落地实现，不需要另写 callback。
- 验证：`tools/build_platform_closure.ps1` 执行 SysConfig、compile、final link；未上板。
