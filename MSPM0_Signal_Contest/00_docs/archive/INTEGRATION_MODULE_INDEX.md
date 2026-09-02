# Integration Module Index

更新时间：2026-08-08。此索引依据当前公开头文件、最终 projectspec 源清单和实际完整链接结果，不依据旧计划推断。算法模块唯一来源为 `../MSPM0_Signal_Contest/`；本仓库旧同名算法目录不属于最终链接源集。

验证级别：`BUILD_VERIFIED` 表示至少进入一个 MSPM0G3507 完整应用链接；`PC_VERIFIED` 表示有 PC 真值测试；均不代表 `BOARD_VERIFIED`。

## 外设、采集、生成与 Adapter

| 模块 | 唯一路径 | 公开 API | 输入 | 输出 | 类型/单位 | 依赖 | 验证 |
|---|---|---|---|---|---|---|---|
| ADC DMA | `02_acquisition/adc_dma/` | `SignalADC_Init/Start/Stop/IsFinished/GetBuffer/GetSampleCount/GetConfiguredTriggerRate` | config、target buffer、N | buffer/count/configured Fs | `uint16_t` code、count、Hz | P01/P02/P04/P06 SysConfig | BUILD_VERIFIED |
| ADC Ring Buffer | `02_acquisition/adc_ring_buffer/` | `SignalADCRing_Init/Push/Pop` | storage、raw samples | ordered raw samples | `uint16_t` code | capacity=N+1 | PC_VERIFIED + BUILD_VERIFIED |
| Trigger Capture | `02_acquisition/trigger_capture/` | `SignalTrigger_Find/Extract` | raw、level/hysteresis/edge | trigger index、segment | code、sample index | ordered frame | BUILD_VERIFIED |
| Timer Capture | `02_acquisition/timer_capture/` | `SignalTimerCapture_Delta/MeanPeriod` | forward timestamp、modulus、timer Hz | ticks、frequency | tick、Hz | P05/P06 | BUILD_VERIFIED |
| Dual ADC platform Adapter | `08_applications/common/signal_dual_adc_platform.*` | `Init/Start/IsFinished/Stop/GetConfiguredRate` | A/B 独立 raw buffers、N、Fs | completion、actual configured Fs | two `uint16_t[]`、Hz | P02/P06 | BUILD_VERIFIED |
| DAC DMA wrapper | `06_generator/dac_dma/` | `SignalDACDMA_Init/Start/Stop` | samples、N、repeat、platform callbacks | module state | `uint16_t[]` code | platform Adapter | BUILD_VERIFIED |
| DAC DMA platform Adapter | `08_applications/common/signal_dac_dma_platform.*` | `Init/Start/Stop/IsOneShotFinished/GetConfiguredRate` | update rate、samples、repeat | DAC/DMA state、actual rate | code、Hz | P03/P04/P06 | BUILD_VERIFIED |
| DAC WaveTable | `06_generator/dac_wave_table/` | `SignalDACWaveTable_Validate/NormalizedToRaw` | normalized value、bits、amplitude/offset | DAC code | fraction → `uint16_t` | status | PC_VERIFIED + BUILD_VERIFIED |
| Sine table | `06_generator/sine/` | `SignalSine_Generate` | table、offset、amplitude、phase | lookup table | fraction/cycle/code | WaveTable | PC_VERIFIED + BUILD_VERIFIED |
| DDS | `06_generator/dds/` | `SignalDDS_Init/SetFrequency/Next/Fill/GetConfiguredFrequency` | table、f、update rate、phase | DAC stream/configured f | code、Hz、phase word | DAC DMA | PC_VERIFIED + BUILD_VERIFIED |
| Frequency Sweep | `06_generator/frequency_sweep/` | `SignalFrequencySweep_Generate` | start/stop/points/log flag | frequency array | Hz | status | BUILD_VERIFIED |
| Arbitrary Wave | `06_generator/arbitrary_wave/` | `SignalArbitraryWave_ResampleLinear` | source/destination arrays | resampled table | `uint16_t` code | workspace owned by caller | PC_VERIFIED + BUILD_VERIFIED |

硬件—算法帧契约：

```text
ADC DMA -> const uint16_t *raw + sample_count + configured_sample_rate_hz
DualADC -> uint16_t A[N] + uint16_t B[N] + common configured_sample_rate_hz
Capture -> forward uint32_t timestamps[] + timer_hz + counter_modulus
DAC/DDS <- const uint16_t samples[] + count + update_rate_hz + repeat
```

## 正式算法模块

以下路径均相对 `../MSPM0_Signal_Contest/`。

| 模块 | 唯一路径 | 公开 API | 输入 | 输出 | 类型/单位 | 依赖 | 验证 |
|---|---|---|---|---|---|---|---|
| ADC To Voltage | `03_measurement/adc_to_voltage/` | `SignalADCToVoltage_Process` | raw、N、conversion config | voltage | code → float V | common | PC+BUILD |
| Mean / MinMax / VPP | `03_measurement/mean/`, `minmax/`, `vpp/` | `SignalMean/MinMax/VPP_Process` | float、N | mean/min/max/Vpp | 输入同单位；本系统 V | common | PC+BUILD |
| RMS / AC RMS | `03_measurement/rms/`, `ac_rms/` | `SignalRMS/ACRMS_Process` | float、N | RMS、mean | V | Math Backend | PC+BUILD |
| Statistics | `03_measurement/statistics/` | `SignalStatistics_Process` | float、N | combined metrics | input unit | common | PC |
| Zero Cross | `03_measurement/frequency_zero_cross/` | `SignalZeroCross_Process` | float、threshold/hysteresis/direction | events | V、sample pair | common | PC+BUILD |
| Phase | `03_measurement/phase/` | `SignalPhase_FromZeroCross/FFTBin/CorrelationLag` | crossings/bins/lag | B−A phase | deg/rad | complex/math | PC+BUILD |
| Remove DC | `04_dsp/remove_dc/` | `SignalRemoveDC_Process` | float、N | centered samples、mean | V | common | PC+BUILD |
| Window | `04_dsp/window/` | `SignalWindow_Apply` | float、N、type | windowed float、gain | V、ratio | Hann/Hamming/Blackman/Rect | PC+BUILD |
| FFT | `04_dsp/fft/` | `SignalFFT_ForwardReal/ForwardComplexInPlace` | real/complex、power-of-two N | complex spectrum | unnormalized DFT | selected FFT Backend | PC+BUILD |
| FFT Magnitude | `04_dsp/fft_magnitude/` | `SignalFFTMagnitude_Process` | complex spectrum | N/2+1 magnitude | raw DFT magnitude | complex type | PC+BUILD |
| Peak Detect | `04_dsp/peak_detect/` | `SignalPeakDetect_Process` | magnitude、bin range | bin/value | bin、spectrum unit | magnitude | PC+BUILD |
| Correlation / Autocorrelation | `04_dsp/correlation/`, `autocorrelation/` | `SignalCorrelation_Process`, `SignalAutocorrelation_Process/FindPeriod` | A/B or one signal、lag、Fs | lag/coefficient/frequency | sample/ratio/Hz | workspace | PC+BUILD |
| Harmonic / THD | `04_dsp/harmonic/`, `thd/` | `SignalHarmonic_Process`, `SignalTHD_Process` | spectrum、f0、orders | H1..Hn、THD | V/% | magnitude | PC+BUILD |
| SNR / SFDR | `04_dsp/snr/`, `sfdr/` | `SignalSNR_Process`, `SignalSFDR_Process` | magnitude、ranges/masks | SNR/SFDR | dB | spectrum | PC+BUILD |
| Hampel | `04_dsp/hampel_filter/` | `SignalHampel_Process` | float、window/sigma、workspace | filtered、replace count | input unit | MAD/sort workspace | PC |
| Zero Cross Interpolation | `05_precision/zero_cross_interpolation/` | `SignalZeroCrossInterpolation_Process` | original samples、events、threshold | fractional crossings | sample | ZeroCross | PC+BUILD |
| Multi Cycle Average | `05_precision/multi_cycle_average/` | `SignalMultiCycleAverage_Process` | same-edge positions、Fs | frequency/period/cycles | Hz/s/sample | interpolation | PC+BUILD |
| FFT Parabolic Interpolation | `05_precision/fft_parabolic_interpolation/` | `SignalFFTParabolicInterpolation_Process` | peak neighbors、Fs/N | fractional bin/f | bin/Hz | Peak | PC+BUILD |
| Window Gain Correction | `05_precision/window_gain_correction/` | `SignalWindowGainCorrection_Apply` | magnitude、N、coherent gain | one-sided amplitude | Vpeak | window | PC+BUILD |
| Multi Bin Energy | `05_precision/multi_bin_energy/` | `SignalMultiBinEnergy_Process` | magnitude、center/radius | energy/RSS amplitude | spectrum²/spectrum | magnitude | PC+BUILD |
| Lock-in | `05_precision/lock_in/` | `SignalLockIn_Process` | voltage、reference f/phase、Fs | I/Q、amplitude、phase | V/deg | Math Backend | PC+BUILD |
| Robust VPP / RMS | `05_precision/robust_peak_to_peak/`, `robust_rms/` | `SignalRobustPeakToPeak_Process`, `SignalRobustRMS_Process` | V、quantiles、workspace | robust metrics | V | sort workspace | PC |

## Integration 与 Application 入口

| 模块 | 路径 | API / 输出 | 验证 |
|---|---|---|---|
| Integration glue | `08_applications/common/signal_integration.*` | `RawToVoltage/FrequencyTime/SignalMeter/Spectrum/THD/DualPhase` | Q31 PC truth 4/4 + full links |
| Sweep glue | `08_applications/sweep_analyzer/signal_sweep_analyzer.*` | f/gain linear/gain dB/phase | PC_VERIFIED + BUILD_VERIFIED |
| Replay glue | `08_applications/waveform_capture_replay/signal_waveform_capture_replay.*` | normalized/resampled DAC table | PC_VERIFIED + BUILD_VERIFIED |
| Signal Analyzer | `08_applications/signal_analyzer/` | meter/spectrum/THD/SNR/SFDR/phase by Profile | 5/5 Profile full links |
| Contest Template | `08_applications/signal_contest_template/` | Init/Acquire/Process/GetResult/Output | 4/4 Profile full links |

## Backend 入口与唯一源约束

| Backend | 配置 | 构建依赖 | 验证 |
|---|---|---|---|
| Reference FFT | `SIGNAL_FFT_BACKEND=0` | none | BUILD_VERIFIED |
| CMSIS Q15 | `=1` | CMSIS headers + M0+ library | benchmark PASS；非稳定默认 |
| CMSIS Q31 | `=2` | `ARM_MATH_CM0` + CMSIS Core/DSP + `arm_cortexM0l_math.a` | 234/0 + system truth + full links |
| CMSIS F32 | `=3` | same CMSIS library | audited，非比赛默认 |
| Reference Math | `SIGNAL_MATH_BACKEND=0` | compiler math | 当前默认 |
| IQMath RTS / MATHACL | `=1/2` | SDK IQMath/MATHACL | 可配置；cycle/runtime PENDING_BOARD |

应用不得直接调用 CMSIS/IQMath/MATHACL。唯一源清单由 `tools/round1_integration_targets.ps1` 和 `tools/final_integration_targets.ps1` 同时驱动 CLI build 与 projectspec。INT-001 用类型边界隔离；INT-002 用严格源清单阻止旧同名算法源码进入链接；两者当前均为 `MITIGATED`。
