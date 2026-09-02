# 外设公共 API 参考与冻结基线

本文件覆盖 40 个外设模块的 public header。完整类型和参数以对应 `.h` 为准；这里给出比赛重组时需要掌握的入口、输出和限制。

## 统一约定

- 所有可失败操作返回 `signal_result_t`，调用者必须检查，不以全局 `errno` 传错。
- 模块成熟度由 `signal_module_status_t` 返回；运行状态与成熟度不是同一概念。
- 数据和工作区由调用者提供；模块不做动态分配。
- BSP 的 `*_t` 大多保存 `context + callback`，因此硬件实例由应用/adapter 决定。
- 数组函数必须同时传指针和长度；输入输出是否可写由 `const` 明确。

## BSP API

| Header | Public API | 关键输入/输出 |
|---|---|---|
| `adc/signal_adc.h` | `SignalADC_ValidateConfig`, `SignalADC_ReadRaw`, `SignalADC_GetBspModuleStatus` | 配置；通过回调输出单个 `uint16_t raw` |
| `comparator/signal_comparator.h` | `ValidateConfig`, `Apply`, `GetModuleStatus` | 阈值/迟滞配置；Apply 调平台回调 |
| `dac/signal_dac.h` | `VoltageToRaw`, `WriteRaw`, `GetModuleStatus` | 电压、位数、Vref → raw；写 raw |
| `dma/signal_dma.h` | `ValidateTransfer`, `Start`, `Stop`, `GetModuleStatus` | `source/destination/count/width`；启动/停止回调 |
| `gpamp/signal_gpamp.h` | `ValidateConfig`, `Apply`, `GetModuleStatus` | GPAMP 配置；平台 Apply |
| `gpio/signal_gpio.h` | `Write`, `Read`, `Toggle`, `GetModuleStatus` | port、pin、bool level |
| `opa/signal_opa.h` | `CalculateGain`, `Apply`, `GetModuleStatus` | 电阻/拓扑配置 → gain；平台 Apply |
| `system_clock/signal_system_clock.h` | `Validate`, `CalculateTimerPeriod`, `GetModuleStatus` | timer clock、target rate → period ticks |
| `timer/signal_timer.h` | `SetRate`, `Start`, `Stop`, `ReadCount`, `GetModuleStatus` | Timer adapter、频率、count |
| `uart/signal_uart.h` | `Write`, `WriteString`, `Read`, `GetModuleStatus` | byte buffer/string；实际 I/O 由回调实现 |
| `vref/signal_vref.h` | `GetEffectiveVoltage`, `GetModuleStatus` | Vref 配置 → 有效参考电压 |

注意：BSP 中 `SignalADC_*` 与硬件专用 `adc_dma` 的 `SignalADC_*` 共享前缀，但头文件和用途不同。一个应用不要同时无区分地包含两个头；平台整合层应给实例对象或 wrapper 起清楚的应用名。该命名债务已冻结，比赛前不做大规模破坏性重命名。

## Acquisition API

| Header | Public API | 关键语义 |
|---|---|---|
| `adc_basic/signal_adc_basic.h` | `ReadBlock`, `GetModuleStatus` | 循环调用 BSP ADC，填调用者 buffer |
| `adc_continuous/signal_adc_continuous.h` | `Init`, `Start`, `Stop`, `SubmitFrame`, `GetModuleStatus` | 连续采集状态机；应用提交已完成 frame |
| `adc_dma/signal_adc_dma.h` | `Init`, `SetSampleRate`, `Start`, `Stop`, `IsFinished`, `GetStatus`, `GetBuffer`, `GetSampleCount`, `GetConfiguredTriggerRate`, `GetModuleMaturity` | 真正绑定 DriverLib 的单块 ADC_DMA；Start 每次重装 DMA |
| `adc_dual_sync/signal_adc_dual_sync.h` | `Deinterleave`, `GetModuleStatus` | 交织 `[A0,B0,...]` 拆成两个 buffer；不启动 ADC |
| `adc_pingpong_dma/signal_adc_pingpong_dma.h` | `Init`, `OnDmaComplete`, `Acquire`, `Release`, `GetModuleStatus` | 管理两个静态 frame 的所有权 |
| `adc_ring_buffer/signal_adc_ring_buffer.h` | `Init`, `Push`, `Pop`, `Count`, `Clear`, `GetModuleStatus` | 固定容量 FIFO；不分配内存 |
| `adc_timer_trigger/signal_adc_timer_trigger.h` | `Init`, `Start`, `Stop`, `GetModuleStatus` | 协调注入的 ADC/Timer callback |
| `timer_capture/signal_timer_capture.h` | `Delta`, `MeanPeriod`, `GetModuleStatus` | 处理 `uint32_t timestamps` 和 rollover；不含 ISR |
| `trigger_capture/signal_trigger_capture.h` | `Find`, `Extract`, `GetModuleStatus` | ADC 数据阈值/边沿触发和窗口提取 |

`SignalADC_GetConfiguredTriggerRate()` 返回 `timer_clock_hz / timer_count` 的整数配置推导值。它不是示波器、RTC 或其他独立参考实测出的物理采样率。

## Generator API

| Header | Public API | 关键语义 |
|---|---|---|
| `am_modulation` | `SignalAMModulation_Apply` | 对调用者数组做 AM 组合 |
| `arbitrary_wave` | `SignalArbitraryWave_ResampleLinear` | 任意波表线性重采样 |
| `dac_dc` | `SignalDACDC_SetVoltage` | 调 BSP DAC 写一个 DC 电压 |
| `dac_dma` | `SignalDACDMA_Init/Start/Stop` | 保存 DMA start/stop callback 和运行状态 |
| `dac_wave_table` | `Validate`, `NormalizedToRaw` | 波表合法性；[-1,1] → DAC raw |
| `dds` | `Init`, `SetFrequency`, `Next`, `Fill`, `GetConfiguredFrequency` | 相位累加器；频率为配置推导值 |
| `frequency_sweep` | `Generate` | 生成频点序列，不负责真实 DAC 输出 |
| `sawtooth` | `Generate` | 填锯齿 wave table |
| `sine` | `Generate` | 填正弦 wave table |
| `square` | `Generate` | 填方波 wave table |
| `triangle` | `Generate` | 填三角 wave table |

## Signal frontend API

| Header | Public API | 输出 |
|---|---|---|
| `comparator_threshold` | `SignalComparatorThreshold_MakeConfig` | `signal_comparator_config_t` |
| `comparator_zero_cross` | `SignalComparatorZeroCross_MakeConfig` | 虚地过零配置 |
| `gpamp_buffer` | `SignalGPAMPBuffer_MakeConfig` | GPAMP buffer 配置 |
| `gpamp_gain` | `SignalGPAMPGain_MakeConfig` | 最接近请求增益的 GPAMP 配置 |
| `opa_buffer` | `SignalOPABuffer_MakeConfig` | OPA follower 配置 |
| `opa_dac_bias` | `SignalOPADACBias_Calculate` | 为目标输出计算 DAC bias |
| `opa_inverting` | `SignalOPAInverting_MakeConfig` | 反相配置及实际 gain |
| `opa_noninverting_pga` | `SignalOPANoninvertingPGA_MakeConfig` | 非反相配置及实际 gain |
| `opa_to_adc` | `SignalOPAToADC_CheckRange` | 输入/增益/bias 与 ADC 范围检查 |

这些模块输出“配置意图”或计算结果；若没有 BSP DriverLib adapter，调用它们不会自动改变片上模拟外设。

## API 冻结规则

1. `01_bsp`、`02_acquisition`、`06_generator`、`07_signal_frontend` 的 public header 自本基线起冻结。
2. 允许添加不破坏 ABI 的新函数，但不能静默修改现有函数参数、结构体字段顺序、枚举数值或语义。
3. 必须修改时，先在 `BREAKING_CHANGE.md` 写迁移方案，再更新 API hash manifest，并同步所有 Demo/profile/template。
4. `signal_types.h` 当前 frame 长度类型是 `size_t`，不是需求草案中的 `uint32_t`。为避免破坏现有算法任务，冻结现状；跨边界时要求 `count <= UINT32_MAX`。
5. 自动检查入口为 `tools/check_peripheral_api_freeze.ps1`。
