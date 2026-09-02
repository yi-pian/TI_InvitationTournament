# Parameter Modify Guide

先改应用自己的 `08_applications/<app>/signal_config.h`。本表只回答“去哪改”；涉及硬件资源的项目转到 `SYSCONFIG_MODIFY_GUIDE.md`。

| 我要改变什么 | 文件 | 参数 | SysConfig | 会影响什么 |
|---|---|---|---:|---|
| ADC sample rate Fs | 各 ADC 应用 `signal_config.h` | `SIGNAL_SAMPLE_RATE_HZ`；Replay 用 `SIGNAL_CAPTURE_SAMPLE_RATE_HZ`；Template 用 `SAMPLE_RATE_HZ` | 通常否；若 Timer period 未由代码重配则是 | Timer 可实现值、samples/cycle、记录时间、FFT bin、deadline |
| ADC FIFO nominal Fs | 使用 `adc_fifo_dma` 的应用 config | 传给 `signal_adc_fifo_dma_config_t.nominal_sample_rate_hz` 的值 | **改 ADC clock/resolution/sample time 时是** | 该值不设置硬件，只决定时间、频率、压摆率换算；必须等于 P08 conversion period 的倒数或实测校准值 |
| sample count N | 同上 | `SIGNAL_SAMPLE_COUNT`；Replay 用 `SIGNAL_CAPTURE_SAMPLE_COUNT`；Template 用 `SAMPLE_COUNT` | 否 | RAM、记录时间、FFT 分辨率、处理延迟 |
| FFT size | Template/config | `FFT_SIZE`，当前必须等于 sample count | 否 | complex workspace 和 magnitude 大小；必须 full link |
| ADC channel index 注释/逻辑 | 应用 config | `SIGNAL_ADC_CHANNEL_INDEX`, `SIGNAL_ADC_A/B_CHANNEL_INDEX`, `ADC_CHANNEL` | **是** | 还必须在相应 ADC SysConfig instance 改 input channel 和 pin |
| ADC VREF | 应用 config | `SIGNAL_ADC_VREF_V`, `SIGNAL_ADC_A/B_VREF_V`, `ADC_VREF_V` | 外部/内部参考源变化时是 | raw→V 标度和全部幅值结果 |
| input gain/attenuation | 应用 config | `SIGNAL_INPUT_SCALE`, A/B variants | 模拟路径变化时可能是 | 电压、RMS、THD amplitude |
| input offset correction | 应用 config | `SIGNAL_INPUT_OFFSET_V`, A/B variants, `SIGNAL_OFFSET_V` | 否 | raw→V；不要和 RemoveDC 混为一件事 |
| frequency search range | FFT/Analyzer/Template config | `SIGNAL_EXPECTED_FREQ_MIN/MAX_HZ`, `EXPECTED_FREQ_MIN/MAX_HZ` | 否 | peak 搜索 bin；THD 还受 H5/Nyquist 约束 |
| zero-cross hysteresis | Meter/Frequency/Analyzer config | `SIGNAL_ZERO_CROSS_HYSTERESIS_V` 或 Template variant | 否 | crossing 抖动与漏检 |
| window | Template config 或组装 glue | `WINDOW_TYPE`；具体算法用 `signal_window_type_t` | 否 | 泄漏、coherent gain、峰宽 |
| FFT Backend | projectspec/CLI compile define | `SIGNAL_FFT_BACKEND=0/1/2/3` | 否 | Flash、数值、library/include；默认 Q31=`2` |
| 特殊标量 Math Backend | projectspec/CLI compile define | `SIGNAL_MATH_BACKEND=0/1/2` | MATHACL resource 需重新检查 | 只影响旧兼容层的 sqrt/div/sin/cos/atan2；普通 DSP 默认 CMSIS，不要把 Reference=`0` 当新工程默认 |
| number of spectrum peaks | Spectrum/Analyzer config | `SIGNAL_SPECTRUM_PEAK_COUNT`, `SIGNAL_PEAK_COUNT` | 否 | 输出长度和峰扫描 |
| harmonic bin radius | THD/Analyzer config | `SIGNAL_HARMONIC_BIN_RADIUS`, `SIGNAL_HARMONIC_RADIUS` | 否 | 谐波能量范围；不能重叠或越界 |
| phase known frequency | Phase/Analyzer config | `SIGNAL_KNOWN_FREQUENCY_HZ`, `SIGNAL_KNOWN_PHASE_FREQUENCY_HZ` | 否 | FFT bin/period→phase 换算 |
| correlation max lag | Phase/Analyzer config | `SIGNAL_MAX_CORRELATION_LAG` | 否 | correlation RAM/CPU；必须 `<N` |
| trigger level | Replay/Template config | `SIGNAL_TRIGGER_LEVEL_CODE`, `TRIGGER_LEVEL` | 软件查找否；硬件 comparator trigger 是 | 触发位置；Replay 单位为 ADC code |
| trigger hysteresis/edge | Replay config | `SIGNAL_TRIGGER_HYSTERESIS_CODE`, `SIGNAL_TRIGGER_EDGE` | 否 | 重复触发与边沿方向 |
| DDS frequency | DDS config | `SIGNAL_DDS_FREQUENCY_HZ`; Template `DDS_FREQUENCY_HZ` | 否 | phase step、每周期点数、repeat 边界 |
| DDS peak amplitude | DDS/Sweep config | `SIGNAL_DDS_AMPLITUDE_PEAK_V`; Template `DDS_AMPLITUDE_V` | 否 | DAC 量程；必须满足 offset±amplitude |
| DDS offset / phase | DDS/Sweep config | `SIGNAL_DDS_OFFSET_V`, `SIGNAL_DDS_PHASE_DEG` | 否 | DAC 余量和初相 |
| DAC update rate | DDS/Sweep config | `SIGNAL_DAC_UPDATE_RATE_HZ` | 通常否，代码计算 Timer load | 输出带宽、Timer 精度、DDS 镜像 |
| DDS table / DMA block | DDS/Sweep config | `SIGNAL_DDS_TABLE_COUNT`, `SIGNAL_DDS_DMA_BUFFER_COUNT` | 否 | RAM、填充时间、repeat 周期闭合 |
| sweep start/stop/points | Sweep config | `SIGNAL_SWEEP_START/STOP_FREQ_HZ`, `SIGNAL_SWEEP_POINT_COUNT` | 否 | 结果数组和总时间；step 与 points 保持一致 |
| settling time | Sweep config | `SIGNAL_SWEEP_SETTLING_TIME_US` | 否 | 每点测试时间 |
| replay table count | Replay config | `SIGNAL_REPLAY_TABLE_COUNT` | 否 | DAC update rate、RAM、重采样误差 |
| feature/profile | `signal_features.h` | `SIGNAL_PROCESSING_PROFILE` 或 `SIGNAL_CONTEST_PROFILE` | Phase/双 ADC 资源变化时可能是 | 编译进来的算法、Flash/RAM/CPU |

## 改完后的联动检查

- 改普通 ADC DMA Fs：检查 Timer 实际 configured rate，并重新计算 `N/Fs`、`Fs/N`、samples/cycle。
- 改 ADC FIFO 满速 Fs：在 SysConfig 改 ADC clock/resolution/sample time，读取 conversion period，再同步 nominal Fs；不能只改 C 中数字。
- 改 N：重新检查所有 workspace capacity 和 `.map`；FFT N 必须是 2 的幂。
- 改 VREF/scale/offset：用已知 DC 电压重新校准 raw→V。
- 改最高频率：检查 Nyquist；THD 的最高谐波也必须低于 `Fs/2`。
- 改 DDS amplitude/offset：检查 `0 ≤ offset-amplitude` 且 `offset+amplitude ≤ DAC VREF`。
- 改 Backend：保持应用 API 不变，重新 full build、PC truth 与 map 对比。

具体模块配置结构和参数合法范围以该模块 README/`.h` 为准。
