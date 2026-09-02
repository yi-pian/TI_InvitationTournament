# UART Minimum Example

【COMPILE-VERIFIED EXAMPLE】

- 真实源码：[main.c](main.c)
- 推荐方式：[DriverLib UART 说明](../../../00_docs/TI_DRIVERLIB_BEGINNER_GUIDE.md)
- SysConfig：`PROFILE_07_BASIC_IO`，UART0 TX PA10、RX PA11、115200、internal loopback off。
- 运行时：逐字节调用 `DL_UART_Main_transmitDataBlocking(SIGNAL_UART_INST, byte)`；不链接 BSP UART 或 Platform Adapter。
- 验证：`tools/build_platform_closure.ps1` 执行 SysConfig、compile、final link；未上板。
