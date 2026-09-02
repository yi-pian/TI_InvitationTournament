# DAC DC Minimum Example

【COMPILE-VERIFIED EXAMPLE】

- 真实源码：[main.c](main.c)
- 推荐方式：[DriverLib DAC 说明](../../../00_docs/TI_DRIVERLIB_BEGINNER_GUIDE.md)
- SysConfig：`PROFILE_07_BASIC_IO`，DAC0/PA15、12 bit、VDDA/VSSA 参考、输出使能。
- 目标：`SYSCFG_DL_init()` 后直接调用 `DL_DAC12_output12(DAC0, 2048U)`，输出约半量程。
- 依赖：只有当前工程生成的 `ti_msp_dl_config.*` 与 MSPM0 SDK；不链接 DAC DC、BSP DAC 或 Platform Adapter。
- 验证：`tools/build_platform_closure.ps1` 执行 SysConfig、compile、final link；未上板。

`DAC0` 是当前 SysConfig 生成/确认的 DAC 实例；`2048U` 是 12-bit code，合法范围为 `0..4095`。修改输出只需改 `g_dac_code`。
