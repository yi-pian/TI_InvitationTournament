# GPIO Minimum Example

【COMPILE-VERIFIED EXAMPLE】

- 真实源码：[main.c](main.c)
- 推荐方式：[DriverLib GPIO 说明](../../../00_docs/TI_DRIVERLIB_BEGINNER_GUIDE.md)
- SysConfig：`PROFILE_07_BASIC_IO`，`SIGNAL_GPIO/OUTPUT` 配置为 PA12 推挽输出、初始低电平。
- 运行时：直接调用 `DL_GPIO_setPins(SIGNAL_GPIO_PORT, SIGNAL_GPIO_OUTPUT_PIN)`；不链接 BSP GPIO 或 Platform Adapter。
- 验证：由 `tools/build_platform_closure.ps1` 执行 SysConfig、compile 和 final link；未上板。

运行语义：初始化后把 PA12 置高。硬件实测需要万用表或逻辑分析仪，Build PASS 不代表引脚电平已观察。
