# PROFILE_06_FULL_SIGNAL

当前最大资源组合：双 ADC、DAC DMA、Comparator→Timer Capture、UART 同时存在。

| 链路 | 资源 |
|---|---|
| ADC A | PA25/ADC0.2, DMA0, TIMG0 Event1 |
| ADC B | PA17/ADC1.2, DMA2, TIMG0 Event2 |
| DAC | PA15/DAC0, DMA1, TIMG6 Event3 |
| Frequency | PA27/COMP0 Event4 → TIMG7 Capture |
| Debug | UART0 PA10/PA11, 115200 |

DMA0/1/2 三个 Full Channel 全部占用。需要 OPA/GPAMP 时先删除不需要的链路并重新跑 SysConfig，不要在此 profile 上直接猜 pin。

## Frequency 链的低频注意事项

本 Profile 复用与 P05 相同的模数时间戳算法，ZERO interrupt 只负责超时，不会把 overflow 数扩展进时间戳。使用默认快时钟/短 Period 时，不能靠增加超时次数测 10 Hz。

配置时在 CCS 中双击 `profile.syscfg`。先进入 `SYSCTL` -> `Clock Tree` 记录实际 `BUSCLK`；Frequency 链进入 `TIMER-CAPTURE` -> `SIGNAL_CAPTURE` -> `Basic Configuration` -> `Clock Configuration`，设置 `Timer Clock Source`、`Clock Divider`、`Clock Prescaler`、`Timer Mode`、`Desired Timer Period`，再在 `Capture Configuration` 选择 `Capture Source = Trigger`，在 `Event Configuration` 核对 COMP publisher 与 capture subscriber 的 channel。ADC/DAC 链分别进入 `ADC12` -> `Basic Configuration` -> `Sampling Mode Configuration` -> `ADC Conversion Memory Configurations` -> `ADC Conversion Memory 0 Configuration`，以及 `DAC12` -> `Event Configuration`/`DMA Configuration`；Timer publisher、ADC subscriber、DAC subscriber 必须按表中 Event1/2/3/4 一一对应。10 Hz 要求 Capture period 大于 100 ms；修改后点击 Generate，使用页面显示的 `Calculated Timer Clock`、`Actual Timer Period` 和生成的 `LOAD + 1` 同步应用的 `timer_hz`、`counter_modulus`。不要直接编辑 `.syscfg` 文本或生成文件。

P06 中 Frequency 使用 TIMG7，DAC 使用 TIMG6；在图形界面调整 Capture 时不要误改 DAC Timer。保存后重新检查 Event4、COMP0、TIMG7、DMA 和 IRQ 资源冲突。仓库中的历史 generated/build 文件只代表生成当时的配置，不能替代当前 CCS 图形页和本次重新生成结果。

状态：SysConfig/compile/link PASS；全系统并行吞吐、ISR 优先级和实板模拟性能尚未验证。
