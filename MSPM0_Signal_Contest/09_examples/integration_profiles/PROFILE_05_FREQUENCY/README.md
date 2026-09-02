# PROFILE_05_FREQUENCY

COMP0 在 PA27 接收输入边沿，经 Event4 送 TIMG6 Capture；UART0 用于调试输出。比较器当前使用 VDDA DAC reference 配置和输入滤波/迟滞。

- 固定资源：COMP0、PA27、Event4、TIMG6、UART0
- Capture clock：当前 Profile 基线为 BUSCLK 32 MHz、2 ms period（64000 ticks）
- 状态：SysConfig/compile/link PASS；比较器阈值、边沿和捕获中断尚未实板验证

## 重要边界：ZERO ISR 不扩展时间戳

当前 Capture ISR 中的 ZERO 分支只累计超时次数；它没有把 overflow 数拼入捕获时间戳。相邻边沿之间最多只能跨过一次 Timer 周期边界，因此当前 2 ms 基线要求输入周期小于 2 ms，也就是输入频率高于约 500 Hz。把 `timeout_overflows` 调大只会延后超时，不会让 10 Hz 结果正确。

应用参数必须满足：

```text
timer_hz = SysConfig 图形页显示的 Capture Timer 实际计数时钟
counter_modulus = 生成的 SIGNAL_CAPTURE_INST_LOAD_VALUE + 1
```

## 10 Hz 的 CCS SysConfig 候选方向（未验证）

不要直接编辑 `profile.syscfg` 文本，也不要修改生成的 `ti_msp_dl_config.c/.h`。10 Hz 要求 Capture Timer 的无歧义周期大于 100 ms；低频时钟或更大分频是候选方向，但本 Profile 当前成功生成和构建的基线仍是 `BUSCLK/1 + 2 ms`，不能声称已经验证 `LFXT/LFCLK` 路径、Divider 或具体频率。

CCS 图形配置路径：双击 `profile.syscfg`，左侧进入 `SYSCTL` -> `Clock Tree` 记录当前时钟；再进入 `TIMER-CAPTURE` -> `SIGNAL_CAPTURE` -> `Basic Configuration` -> `Clock Configuration`，依次设置 `Timer Clock Source`、`Clock Divider`、`Clock Prescaler`、`Timer Mode` 和 `Desired Timer Period`，在 `Capture Configuration` 选择 `Capture Source = Trigger`，在 `Event Configuration` 核对 subscriber channel。当前基线是 `BUSCLK / 1 / 1 / 2 ms`、`TIMG6`；若测 10 Hz，把 `Desired Timer Period` 调到大于 100 ms，并使用右侧 `Calculated Timer Clock`、`Actual Timer Period` 以及生成的 `SIGNAL_CAPTURE_INST_LOAD_VALUE + 1` 更新应用参数。

拿到截图并通过 GUI 生成成功后，再记录 `GUI field -> .syscfg property -> generated LOAD/clock evidence`。应用的 `timer_hz` 必须等于 GUI 显示的实际 Timer counter clock，模数使用生成的 `SIGNAL_CAPTURE_INST_LOAD_VALUE + 1`；最后用已知 10 Hz 方波实测，才能把候选方案升级为已验证配置。
