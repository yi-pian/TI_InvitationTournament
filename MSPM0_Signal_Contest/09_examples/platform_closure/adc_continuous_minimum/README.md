# ADC Continuous Minimum Example

【COMPILE-VERIFIED EXAMPLE】

- 真实源码：[main.c](main.c)
- 模块说明：[ADC Continuous README](../../../02_acquisition/adc_continuous/README.md)
- Platform：本模块的 callback 是业务帧消费者，不是 DriverLib glue；真实采集帧通常来自 ADC Ping-Pong DMA。
- SysConfig：此最小 API 示例复用 `PROFILE_07_BASIC_IO`，但 `SignalADCContinuous_*` 本身不占用外设。
- 验证：由 `tools/build_platform_closure.ps1` 执行 SysConfig、compile 和 final link；未上板。
