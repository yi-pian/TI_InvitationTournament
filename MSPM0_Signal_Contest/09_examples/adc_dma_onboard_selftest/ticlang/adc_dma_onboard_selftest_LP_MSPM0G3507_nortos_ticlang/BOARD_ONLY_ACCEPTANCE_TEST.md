# ADC_DMA 两级实板验收

## 1. 状态边界

| 状态 | 必须证明什么 | 当前硬件能否完成 |
|---|---|---|
| `BOARD-ONLY VERIFIED` | TMP6131 在 ADC0.5 上采样正常；五档 N 各 100 帧；DMA 无哨兵残留；重复 Start 与 WFE 正常；LFXT/FCC 时钟交叉检查通过 | 能，仅需开发板、USB、Windows、CCS |
| `FULL HARDWARE VERIFIED` | PA25/ADC0.2 动态模拟输入、100/200/500 kSPS 和 samples/cycle 经已知动态波形验证 | 不能；至少等待一根母对母杜邦线做 DAC->ADC 环回，或外部信号源 |

通过 LEVEL 1 后仍必须明确写：`PA25 dynamic analog input not verified`。不能把 TMP6131 的慢速近直流数据当作 PA25 带宽或高速采样率验证。

## 2. 官方依据与板卡设置

- [LP-MSPM0G3507 User's Guide（SLAU873D）](https://www.ti.com/lit/ug/slau873d/slau873d.pdf)：2.3.7.2 说明板载 TMP6131、10 kΩ 上拉和 J9 路由；J9 1-2 把温敏信号直接送到 PB24，室温约 1.6 V；J13 给温敏/模拟区供电。
- 同一 User's Guide 的 pinout 将 PB24 标为 `A0_5`，因此 SysConfig 使用 `ADC0 / DL_ADC12_INPUT_CHAN_5 / PB24`，不是凭经验选择通道。
- 本机 SDK `examples/nortos/LP_MSPM0G3507/demos/out_of_box` 也把 Thermistor 列为 `ADC0_5 / PB24`，并明确 J9 1-2 是直接 ADC 路径。
- 同一官方 Thermistor 实现为 10 kΩ 分压源使用约 3.25 us ADC sample time；板载 self-test 与 UART dump 沿用该建立时间，而不是 PA25 高速 Demo 的 62.5 ns。
- [MSPM0G350x Datasheet](https://www.ti.com/lit/ds/symlink/mspm0g3507.pdf) 和 [MSPM0 G-Series TRM](https://www.ti.com/lit/ug/slau846d/slau846d.pdf) 用于核对 ADC、SYSOSC、LFXT 和 FCC 的硬件定义。
- 本机 SDK 官方例程 `driverlib/sysctl_frequency_clock_counter` 使用 `DL_SYSCTL_startFCC()`、`DL_SYSCTL_isFCCDone()`、`DL_SYSCTL_readFCC()`，统计两个 LFCLK 上升沿周期内的 SYSOSC 周期数。本测试只使用这些真实存在的 API。

板卡断电后确认：

```text
J9  : 1-2（TMP6131 直接到 PB24）
J13 : ON（模拟区供电）
Y1  : 板载 32.768 kHz 晶振，默认已焊接；PA3/PA4 不需要外接线
```

## 3. 五档 N、每档 100 帧

程序不修改底层 `signal_adc_dma.c`，而是把 N 作为 `SignalADC_Start(buffer, N)` 的参数依次传入：

```text
256 -> 100 frames
512 -> 100 frames
1024 -> 100 frames
2048 -> 100 frames
4096 -> 100 frames
```

每帧开始前把有效区写为 `0xFFFF`。ADC 是 12-bit，合法范围仅为 0..4095，所以帧末任何大于 4095 的值都意味着 DMA 没有覆盖完整缓冲区。每次 `Start` 还会由正式模块重新准备 Timer count、DMA source/destination/size、DMA enable、ADC DMA、ADC conversion、中断标志和模块状态。

LEVEL 1 通过时：

```text
g_acceptance_complete     true
g_acceptance_pass         true
g_completed_sizes         5
g_completed_blocks        100
g_total_completed_blocks  500
g_sentinel_residue_count  0
g_wfe_completed_blocks    500
g_failed_block            0xFFFFFFFF
g_failed_sample_count     0
g_last_result             SIGNAL_RESULT_OK
g_module_status           MODULE_DONE
g_selftest_failure        SELFTEST_FAILURE_NONE
```

若程序停在前面的失败断点，`g_failed_sample_count` 是失败的 N，`g_failed_block` 是该档 0..99 的帧号，`g_selftest_failure` 给出失败阶段。

## 4. 板内采样时钟交叉估算

SysConfig 采用 SDK 官方 FCC 配置：

```text
reference = LFXT -> LFCLK = 32768 Hz
source    = SYSOSC nominal 32 MHz
window    = 2 reference periods
```

计算为：

```text
measured_sysosc_hz = fcc_count * 32768 / 2
timer_count         = round(32000000 / requested_rate)
estimated_trigger   = measured_sysosc_hz / timer_count
```

32 MHz 的理想 `g_fcc_count` 约为 1953。测试沿用 TI 官方例程的 ±2.5% 窗口，即 `g_measured_sysosc_hz` 必须在 31.2..32.8 MHz。

这里必须区分两项：

- `g_configured_trigger_rate_hz`：由名义 Timer 时钟与 LOAD 推导的配置值；默认是精确的 100000。
- `g_estimated_trigger_rate_hz`：用 LFXT/FCC 估计 SYSOSC，再结合已知 BUSCLK/1/1 和 Timer LOAD 推算的板内估计值。

FCC 并没有直接探测 ADC 采样孔径，也不能证明每个 Timer Event 都产生了有效动态样本。它是独立时基的板内交叉检查，不是示波器实测；正式 ADC_DMA 模块不依赖 FCC。

注意：SDK 生成的 `SYSCFG_DL_init()` 会等待 `DL_SYSCTL_CLK_STATUS_LFXT_GOOD`。如果下载后连第一个测试断点都到不了，暂停 CPU 检查是否停在该等待循环，并重新上电；这与 ADC_DMA `Start` 不是同一故障。

## 5. CCS Graph 查看 g_adc_buffer

程序在最终 `__BKPT(0)` 停住后，不要继续运行：

1. 先看 `g_current_sample_count`，成功结束时为 4096。
2. CCS Classic 选择 `Tools -> Graph -> Single Time`。
3. `Start Address` 填 `g_adc_buffer` 或 `&g_adc_buffer[0]`。
4. `DSP Data Type` 选 `16 bit unsigned integer`。
5. `Acquisition Buffer Size` 和 `Display Data Size` 都填 `4096`；查看较早失败档时填 `g_failed_sample_count`。
6. `Index Increment` 填 1，其他采样率字段可填 1；横轴在本测试中只是 Sample Index。

若当前 CCS 界面没有 Graph 菜单，用 Memory Browser 输入 `&g_adc_buffer[0]`，选择 16-bit unsigned 显示并查看相同长度。TMP6131 是慢信号，曲线接近水平并有少量 ADC 噪声是正常的；它不应该被解读为波形发生器输出。

## 6. 板载自检故障排查

| 现象/失败码 | 可能原因 | 先查哪里 |
|---|---|---|
| 初始化阶段不前进 | LFXT 未进入 GOOD；板卡未重新上电 | 暂停 CPU，看 `SYSCFG_DL_SYSCTL_CLK_init`；USB 重新上电 |
| `CLOCK_REFERENCE` | FCC count 超出 ±2.5% | `g_fcc_count`、`g_measured_sysosc_hz`；确认用的是 self-test 工程的 SysConfig |
| `SENTINEL` | DMA 长度/目标地址/地址增量错误 | `g_sentinel_residue_count`、失败 N；DMA_CH0 fixed-to-block、half-word、single |
| `ALL_ZERO` | J13 未供电、TMP 节点被拉低、ADC 通道错误 | J13、J9 1-2、SysConfig ADC0.5/PB24 |
| `ALL_FULL_SCALE` | J9 错位、TMP 节点开路/拉高、ADC 饱和 | J9、J13、PB24 通道 |
| `MEAN_RANGE` | J9/J13 错、环境极端、模拟区异常 | `g_adc_raw_mean`；本测试宽松窗口 1000..3100 |
| `START` 或第二帧失败 | DMA/ADC 标志或状态未重装 | `g_failed_block`；单步 `SignalADC_Start()` 的 disable/clear/reload/enable 顺序 |
| 状态一直 RUNNING | Timer Event、ADC subscriber、DMA trigger 或 ADC IRQ 错 | Event.dot、ADC DMA_DONE、NVIC、`SIGNAL_ADC_INST_IRQHandler` |
| 停在 `__WFE` | DMA 没完成或 ADC DMA_DONE 中断没唤醒 | 暂停后看 `g_module_status`、ADC RIS/IIDX、DMA transfer size |
| 改 N 后异常 | 缓冲容量/CCS 显示长度不匹配 | 本例固定容量 4096；不要传入大于 4096 的测试值 |

## 7. 未来一根杜邦线的 LEVEL 2 方案（只写方案）

未来准备一根母对母杜邦线：

```text
PA15 / DAC_OUT -> PA25 / ADC0.2
```

同一块板已共地。届时另建 `09_examples/adc_dma_dac_loopback_selftest`，内部放一个 **TEST ONLY** 的固定查表 DAC 刺激程序，输出位于 0..VDDA 内、带约 1.65 V 偏置的已知周期波形。该程序不得放入 `06_generator`，不得演化成正式 DDS，也不得让 ADC_DMA 依赖 DAC。

环回验收依次检查：

| ADC Fs | 建议测试波形 | 理论 samples/cycle |
|---:|---:|---:|
| 100 kSPS | 1 kHz | 100 |
| 200 kSPS | 1 kHz | 200 |
| 500 kSPS | 10 kHz | 50 |

通过环回或外部已知信号后，才能把状态升级为 `FULL HARDWARE VERIFIED`。现在不开发 Timer Capture、RMS、Vpp、FFT、DDS 或 Frequency。
