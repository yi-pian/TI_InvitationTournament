# Comparator + Timer Capture Minimum Example

【COMPILE-VERIFIED EXAMPLE】

- 真实源码：[main.c](main.c)
- 模块说明：[Comparator README](../../../01_bsp/comparator/README.md)、[Timer Capture README](../../../02_acquisition/timer_capture/README.md)
- Platform 说明：[MSPM0G3507 Platform Adapter](../../../08_applications/common/mspm0g3507/README.md)
- SysConfig：`PROFILE_05_FREQUENCY`，Comparator Event → Timer Capture/IRQ。
- 验证：`tools/build_platform_closure.ps1` 执行 SysConfig、compile、final link；未上板。

## 时钟参数必须与当前 SysConfig 一致

当前 P05 基线是 BUSCLK/1、32 MHz、2 ms；因此本例默认把 `timer_hz` 设为 `CPUCLK_FREQ`，并把 `counter_modulus` 设为生成的 `SIGNAL_CAPTURE_INST_LOAD_VALUE + 1U`。这只在 Capture Timer 的实际时钟确实等于 CPUCLK 时成立。

如果测 10 Hz，必须先把 Timer 周期扩展到能覆盖相邻边沿。低频时钟是候选方向，但当前已验证 P05 仍是 `BUSCLK/1 + 2 ms`；现有截图和成功 `.syscfg` 没有确认 `LFXT -> LFCLK -> TIMER-CAPTURE`、Divider 或实际频率，因此这里不提供可照抄的具体 GUI 值。

CCS 图形配置路径：双击工程 `.syscfg`，左侧展开 `SYSCTL` -> `Clock Tree` 记录当前 `BUSCLK`；再进入 `TIMER-CAPTURE` 实例 `SIGNAL_CAPTURE` -> `Basic Configuration` -> `Clock Configuration`，依次选择 `Timer Clock Source`、`Clock Divider`、`Clock Prescaler`、`Timer Mode`，填写 `Desired Timer Period`。回到同一实例的 `Capture Configuration` 选择 `Capture Source = Trigger`，在 `Event Configuration` 核对 subscriber；页面右侧以 `Calculated Timer Clock`、`Actual Timer Period` 为准。P05 参考值是 `BUSCLK / 1 / 1 / 2 ms`，10 Hz 时把目标周期扩展到大于 100 ms，并用生成的 `LOAD + 1` 更新 `counter_modulus`。

补图并生成成功后，使用 GUI 显示的实际 Timer counter clock 更新 `timer_hz`，使用生成的 `LOAD+1` 作为模数，再确认 Timer period 大于 100 ms。不要把候选 `LFCLK /2` 或某个计算频率当作已验证值。

ZERO ISR 只做超时，不扩展时间戳。默认 2 ms 配置只适用于输入周期小于 2 ms；增加 `timeout_overflows` 不能让 10 Hz 正确。更完整的边界与计算见 [Timer Capture README](../../../02_acquisition/timer_capture/README.md)。
