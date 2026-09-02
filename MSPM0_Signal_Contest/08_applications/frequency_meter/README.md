# Frequency Meter A/B/C

> **REFERENCE ASSEMBLY EXAMPLE**：展示三条已拼装频率链，不负责替你选择方法。

## 本 Application 的实现层级

| 功能 | 类型 | 当前实现 |
|---|---|---|
| Method A 硬件测频 | B Complex Hardware Module | Comparator/Event/Timer Capture 完整链 |
| Method B/C 采样帧 | B Complex Hardware Module | ADC DMA |
| Method B 时域测频 | C Algorithm Module | Remove DC、Zero Cross、Interpolation、Multi Cycle Average |
| Method C FFT 测频 | C Algorithm Module | Window、FFT、Magnitude、Peak、Parabolic |
| 完整组合 | E Application Reference | 三种后端统一输出 `frequency_hz` |

```text
A: COMP0 → Event4 → TIMG6 Capture → forward timestamps → MeanPeriod
B: ADC DMA → Voltage → RemoveDC → ZeroCross → Interpolation → MultiCycleAverage
C: ADC DMA → Voltage → RemoveDC → Hann → FFT → Magnitude → Peak → Parabolic
```

- Status：A/B/C 均 `BUILD_VERIFIED`；Board=`PENDING_BOARD`。
- Config：`signal_config.h` 中 `SIGNAL_FREQUENCY_METHOD` 及共用参数。
- Hardware profile：A=P05，B/C=P01。
- Projectspec：`frequency_meter_a_round1_*`, `frequency_meter_b_round1_*`, `frequency_meter_c_q31_*`。
- 重点学习：同一个统一结果 `frequency_hz` 如何由不同硬件/算法链产生。

Capture 向下计数转换集中在应用 Adapter；不要把寄存器值直接交给 MeanPeriod。FFT C 默认 Q31，应用不直接调用 CMSIS。

## 使用 CCS SysConfig 图形界面

本应用的硬件配置全部在 CCS 中双击工程 `.syscfg` 后通过 SysConfig 图形界面完成。`PROFILE_05_FREQUENCY` 和 `PROFILE_01_ADC_CAPTURE` 是资源与字段参考，不要求手工编辑 `.syscfg` 文本；生成的 `ti_msp_dl_config.c/.h` 只用于核对实例名、实际时钟和 LOAD，不直接修改。

- Method A：在 `SYSCTL`、`SIGNAL_CAPTURE`、`SIGNAL_COMP` 图形页配置时钟、周期、Capture Trigger、Comparator 和 Event。
- Method B/C：在 `SIGNAL_SAMPLE_TIMER`、ADC、DMA 和 Event 图形页配置硬件路由；采样率由应用调用模块时设置，但 `timer_clock_hz` 必须等于图形页显示的 Timer 实际计数时钟。
- 每次保存 SysConfig 后 Clean/Build，并重新核对 `SIGNAL_CAPTURE_INST_LOAD_VALUE` 或采样 Timer 的生成 LOAD。

## 三种方法的最低频率边界

### Method A：Timer Capture

当前 Adapter 和 `SignalTimerCapture_MeanPeriod` 只保存 Timer 周期内的模数时间戳，能处理一次边界回绕，不能恢复相邻边沿之间的多次完整回绕。必须满足：

```text
Tinput < (LOAD + 1) / timer_hz
finput > timer_hz / (LOAD + 1)
```

`SIGNAL_CAPTURE_COUNTER_MODULUS` 必须等于当前工程生成的 `SIGNAL_CAPTURE_INST_LOAD_VALUE + 1U`，不能另写一个与 SysConfig 不一致的常量。`SIGNAL_CAPTURE_TIMEOUT_OVERFLOWS` 只决定多久超时，不会扩展时间戳。

测 10 Hz 时，先在 CCS SysConfig 图形界面把 Capture Timer 的无歧义周期扩展到大于 100 ms。当前已验证 P05 只有 `BUSCLK/1 + 2 ms`；`LFXT/LFCLK` 路径、Divider 和实际 Timer clock 尚未由现有截图及成功 `.syscfg` 确认，所以不能在这里给出固定的 Clock Source/Divider 值。

CCS 图形配置路径：双击工程 `.syscfg`，进入 `SYSCTL` -> `Clock Tree` 记录实际 `BUSCLK`；Method A 再进入 `TIMER-CAPTURE` -> `SIGNAL_CAPTURE` -> `Basic Configuration` -> `Clock Configuration`，设置 `Timer Clock Source`、`Clock Divider`、`Clock Prescaler`、`Timer Mode` 与 `Desired Timer Period`，然后在 `Capture Configuration` 选择 `Capture Source = Trigger`。Method B/C 进入 `TIMER` -> `SIGNAL_SAMPLE_TIMER` -> `Basic Configuration` -> `Clock Configuration` 与 `Event Configuration`，将 Timer publisher 与 ADC subscriber 配成同一 channel。始终以页面显示的 `Calculated Timer Clock`、`Actual Timer Period` 和生成 `LOAD` 回算 `timer_clock_hz`、`sample_rate_hz`；10 Hz 时 Capture period 必须大于 100 ms。

补图并生成成功后，让 `SIGNAL_CAPTURE_CLOCK_HZ` 等于 GUI 显示的实际 Timer counter clock，模数继续使用生成的 `LOAD+1`；`SIGNAL_CAPTURE_TIMEOUT_OVERFLOWS` 只按所需超时时间设置，不能修复多次回绕。

8 个时间戳覆盖 7 个周期，10 Hz 时约 0.7 s 得到一次结果。若要更快刷新，可减小时间戳数量，但抖动会增大。

### Method B：过零/插值

一帧必须至少包含两个同方向过零点，实际建议包含 3～10 个周期：

```text
frame_time = sample_count / sample_rate_hz
建议 frame_time >= 3 / expected_min_frequency_hz
```

默认 `100 kHz × 1024 点` 只有 10.24 ms，连一个 10 Hz 周期都装不下。10 Hz 可从 `sample_rate_hz=1000 Hz、N=1024` 起步，此时窗口 1.024 s，约含 10 个周期。降低采样率前还要确认最高输入频率、Nyquist 条件和模拟抗混叠滤波。

### Method C：FFT

除窗口必须覆盖低频周期外，还要检查 FFT bin 间隔：

```text
delta_f = sample_rate_hz / N
```

默认 `100 kHz / 1024` 的 bin 间隔约 97.66 Hz，不能用于 10 Hz。`1000 Hz / 1024` 的 bin 间隔约 0.977 Hz，可作为 10 Hz 初始配置；若精度要求更高，应继续降低 `Fs`、增大 `N` 或使用峰值插值，同时核对 RAM。

## 10 Hz 推荐选择

| 条件 | 推荐方法 | 起始配置 |
|---|---|---|
| 能由比较器稳定整形成边沿 | Method A | 先验证可用低频/分频路径，使 Capture period > 100 ms；8 timestamps |
| 需要保留波形或同时测幅值 | Method B | Fs=1 kHz、N=1024、带迟滞/插值 |
| 波形复杂且希望观察频谱 | Method C | Fs=1 kHz、N=1024 或更大，检查 RAM |

这些是 10 Hz 的起始参数，不替代题目的最高频率、精度、刷新时间和信噪比检查。若同一设备要从 10 Hz 覆盖到很高频率，优先做分档配置，而不是强迫一套 Timer/采样参数覆盖全量程。
