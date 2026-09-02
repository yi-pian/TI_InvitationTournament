# ADC_DMA LEVEL 2 动态模拟验收 Demo

本工程需要外部动态模拟信号与测量条件，当前不属于仅靠开发板/USB/CCS 可完成的 LEVEL 1。无仪器时先运行相邻目录的 `adc_dma_onboard_selftest`；本工程保留给未来 `FULL HARDWARE VERIFIED`，不要用 TMP6131 的慢信号替代动态 samples/cycle 验证。

本工程只验收：

```text
PA25 / ADC0.2 -> Timer Trigger -> Event -> ADC0 -> DMA -> RAM Buffer
```

不使用板载 OPA2365，不包含 FFT、RMS、Vpp 或频率算法。`min/max/mean`、哨兵检查和 100 帧计数全部位于 `main.c`，只是验收统计，不是正式 measurement 模块。

## 10 分钟运行

1. 在 CCS 导入 `ticlang/signal_adc_dma_demo_LP_MSPM0G3507_nortos_ticlang.projectspec`。
2. 信号发生器设置为正弦波 1 kHz、1.0 Vpp、Offset 1.65 V、Hi-Z；OUT 接 PA25/J1.2，GND 与 LaunchPad 共地。
3. 默认配置在 `signal_config.h`：100 kSPS、1024 点、连续 100 帧。
4. Build、Download、Run。前 99 帧不会停；完成第 100 帧后停在 `__BKPT(0)`。
5. 在 CCS Expressions 查看下列变量，并按 `HARDWARE_ACCEPTANCE_TEST.md` 记录结果。

| 变量 | 通过时的含义 |
|---|---|
| `g_acceptance_complete` | `true` |
| `g_acceptance_pass` | `true` |
| `g_completed_blocks` | `100` |
| `g_validation_failure` | `VALIDATION_FAILURE_NONE` |
| `g_last_result` | `SIGNAL_RESULT_OK` |
| `g_last_module_status` | `MODULE_DONE` |
| `g_configured_trigger_rate_hz` | 默认 `100000` |
| `g_adc_raw_min/max/mean` | 最后一帧原始码统计 |
| `g_adc_buffer` | 最后一帧的 1024 个 `uint16_t` 样本 |

## 参数只改一处

```c
#define SIGNAL_SAMPLE_RATE_HZ           (100000U)
#define SIGNAL_SAMPLE_COUNT             (1024U)
#define SIGNAL_ACCEPTANCE_BLOCK_COUNT   (100U)
```

支持通过配置测试 N=256/512/1024/2048/4096 和 Fs=100/200/500 kSPS，不需要修改 `signal_adc_dma.c`。这些宏也可由编译器 `-D` 覆盖。

## 采样率 API 的准确含义

`SignalADC_GetConfiguredTriggerRate()` 返回由 Timer 计数时钟、divider、prescaler 和整数 LOAD 推导出的**配置事件触发率**。它不是示波器实测值，也不能单独证明 ADC 没有丢触发。

默认 32 MHz Timer 时钟下：100/200/500 kSPS 分别使用 count=320/160/64，均可整除。物理验证方法是测量 PA12/J4.34 上的 TIMG0_CCP0 验收输出，并同时检查 RAM 波形周期点数。PA12 只由本 Demo 使用；正式 ADC_DMA 模块不依赖它。

完整测试步骤、理论值、示波器接法、100 帧判据和故障排查表见 [HARDWARE_ACCEPTANCE_TEST.md](HARDWARE_ACCEPTANCE_TEST.md)。

## 当前验证级别

- 当前 CCS SysConfig 1.28.0 严格生成：通过。
- 当前 TI Arm Clang 5.1.1.LTS，`-O2 -Wall -Werror` 编译并完整链接：通过。
- N/Fs 编译链接矩阵：见硬件验收文档。
- 实板 100 帧、示波器和信号发生器结果：当前条件无法完成。未来 DAC 环回或外部信号实测前，不能宣布 `FULL HARDWARE VERIFIED`。
