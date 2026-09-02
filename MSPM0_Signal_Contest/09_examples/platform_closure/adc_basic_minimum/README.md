# ADC Basic Minimum Example

【COMPILE-VERIFIED EXAMPLE】

- 真实源码：[main.c](main.c)
- 推荐方式：[DriverLib ADC 说明](../../../00_docs/TI_DRIVERLIB_BEGINNER_GUIDE.md)
- SysConfig：`PROFILE_07_BASIC_IO`，ADC0 channel 2/PA25、software trigger、MEM0 interrupt。
- 运行时：直接 start、等待 MEM0 result loaded、再 `DL_ADC12_getMemResult()`；不链接 ADC Basic、BSP ADC 或 Platform Adapter。
- 边界：此例只读一个样本；要采稳定的 `raw[N]`，使用正式 ADC DMA 模块。
- 验证：`tools/build_platform_closure.ps1` 执行 SysConfig、compile、final link；未上板。
