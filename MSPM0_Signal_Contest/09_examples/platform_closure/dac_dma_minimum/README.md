# DAC DMA Minimum Example

【COMPILE-VERIFIED EXAMPLE】

- 真实源码：[main.c](main.c)
- 模块说明：[DAC DMA README](../../../06_generator/dac_dma/README.md)
- Platform 说明：[DAC DMA Platform Adapter](../../../08_applications/common/dac_dma_platform_adapter/README.md)
- SysConfig：`PROFILE_03_DAC_GENERATOR`，Timer/Event/DMA1/DAC0。
- callback 绑定：`SignalDACPlatform_Start`、`SignalDACPlatform_Stop`。
- 验证：`tools/build_platform_closure.ps1` 执行 SysConfig、compile、final link；未上板。
