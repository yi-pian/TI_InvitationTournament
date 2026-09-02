# ADC Timer Trigger Minimum Example

【COMPILE-VERIFIED EXAMPLE】

- 真实源码：[main.c](main.c)
- 模块说明：[ADC Timer Trigger README](../../../02_acquisition/adc_timer_trigger/README.md)
- Platform 说明：[MSPM0G3507 Platform Adapter](../../../08_applications/common/mspm0g3507/README.md)
- SysConfig：`PROFILE_01_ADC_CAPTURE`。
- callback 绑定：`arm_adc/disarm_adc` 分别使用 `SignalMSPM0G3507_ADC_Enable/Disable`；Timer callback 由 `SignalMSPM0G3507_Timer_Bind` 填入。
- 验证：由 `tools/build_platform_closure.ps1` 执行 SysConfig、compile 和 final link；未上板。

此示例只验证 callback、Timer/Event/ADC 配置与链接闭环，不读取一帧数据；N 点采集看相邻 `adc_dma_minimum`。
