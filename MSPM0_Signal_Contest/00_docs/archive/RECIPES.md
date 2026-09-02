# Contest recipes

RAM 是应用数组/工作区近似值，不含 SDK BSS、链接库和栈；最终必须查 CCS `.map`。下面的代码是组合伪代码，硬件实例名以应用 `.syscfg` 生成宏为准。

## 1. 普通幅值 / 频率测量

- 模块：`adc_dma -> adc_to_voltage -> signal_analyzer`
- 数据流：`uint16_t raw[N] -> float voltage[N] -> mean/min/max/Vpp/RMS/frequency`
- 初始化：SysConfig -> `SignalADC_Init` -> 准备 ADC 电压换算配置。
- 参数：Fs 至少为输入最高频率 20 倍，N 覆盖至少 5~10 周期，阈值通常取去直流后 0 V。
- RAM：N=1024 时 raw 2 KB + voltage 4 KB，约 6 KB。

```c
SignalADC_Start(raw, N); wait_until_done();
SignalADCToVoltage_Block(raw, N, &adc_v_cfg, voltage, N);
SignalAnalyzer_Analyze(voltage, N, fs_hz, threshold_v, &result);
```

## 2. 高频频率计

- 模块：`comparator_zero_cross -> timer_capture -> frequency_timer_capture/frequency_meter`
- 数据流：`模拟边沿 -> comparator 数字边沿 -> uint32_t timestamps[] -> frequency`
- 初始化：配 Comparator 阈值/迟滞 -> 配 Timer Capture 边沿与时钟 -> 开始记录时间戳。
- 参数：`timer_hz`、`counter_modulus`、边沿极性、阈值和迟滞。
- RAM：32~128 个时间戳约 0.125~0.5 KB。

```c
/* ISR 只把捕获值放入 timestamps */
SignalFrequencyMeter_FromCapture(timestamps, count, timer_hz,
    counter_modulus, &frequency_hz, &period_ticks);
```

## 3. 示波器

- 模块：`adc_pingpong_dma/adc_dma -> trigger_capture -> adc_to_voltage -> oscilloscope`
- 数据流：`raw frame -> trigger-aligned frame -> voltage -> min/max/mean/Vpp/RMS`
- 初始化：采集 -> 触发配置 -> ADC 换算 -> 示波统计。
- 参数：Fs、N、trigger level/hysteresis/edge、pre-trigger 点数、VREF。
- RAM：N=1024 时 raw 2 KB + 对齐 raw 2 KB + voltage 4 KB，约 8 KB；可复用 raw 区降至 6 KB。

```c
SignalTrigger_Find(raw, N, &trigger_cfg, search_start, &trigger_index);
SignalTrigger_Extract(raw, N, trigger_index, pretrigger, aligned, N);
SignalADCToVoltage_Block(aligned, N, &adc_v_cfg, voltage, N);
SignalOscilloscope_Analyze(voltage, N, &measurements);
```

## 4. 频谱仪

- 模块：`adc_dma -> adc_to_voltage/remove_dc -> Hann -> FFT -> magnitude -> fft_peak -> parabolic`
- 数据流：`float time[N] -> complex FFT[N] -> magnitude[N/2+1] -> peak`
- 初始化：采集完成后再调用 `SignalSpectrumAnalyzer_Analyze`，FFT 工作区由应用层静态提供。
- 参数：N 必须是 2 的幂，Fs 大于带宽 2 倍并保留抗混叠裕量，搜索 bin 排除 DC/Nyquist。
- RAM：N=1024 的调用者 FFT 链约 12.0 KB；详见 `FFT_MEMORY_BUDGET.md`。

```c
SignalSpectrumAnalyzer_Analyze(voltage, N, fs_hz, SIGNAL_SPECTRUM_HANN,
    fft_workspace, N, magnitude, N / 2U + 1U, first_bin, last_bin, &spectrum);
```

## 5. THD 分析

- 模块：`配方4 -> harmonic -> thd/harmonic_thd_analyzer`
- 数据流：`magnitude[] + fundamental_bin -> harmonic_amplitudes[] -> THD`
- 初始化：先完成频谱，再以主峰 bin 为基波。
- 参数：`bin_radius=1~2`、谐波个数不得越过 Nyquist；基波幅度为 0 时不计算。
- RAM：频谱链约 12 KB + 谐波数组数十 bytes。

```c
SignalHarmonicTHDAnalyzer_Analyze(magnitude, N / 2U + 1U,
    spectrum.discrete_peak.bin, bin_radius, harmonic_count,
    harmonic_amplitudes, harmonic_count, &thd);
```

## 6. 双通道相位

- 模块：`adc_dual_sync -> adc_to_voltage(A/B) -> correlation -> phase/dual_channel_phase_meter`
- 数据流：`interleaved raw -> A/B float arrays -> delay_samples -> phase_degrees`
- 初始化：使用同一触发的双 ADC/MEM 链，先做通道延时标定。
- 参数：N=1024、`maximum_absolute_lag`、Fs、已知/已测信号频率。
- RAM：双 raw 4 KB + 双 float 8 KB，约 12 KB。

```c
SignalDualChannelPhaseMeter_Measure(channel_a, channel_b, N,
    max_lag, fs_hz, signal_frequency_hz, &phase_result);
```

## 7. DDS 发生器

- 模块：`sine/dac_wave_table -> dds -> dac_dma`
- 数据流：`wave_table + phase_accumulator -> DAC codes -> DMA repeat`
- 初始化：准备 DAC 更新 Timer/Event/DMA -> 生成波表 -> 初始化 DDS -> 启动 DAC DMA。
- 参数：`table_count=256`、`update_rate_hz`、输出频率、幅度和偏置；必须检查 DAC 范围。
- RAM：256 点 `uint16_t` 波表 0.5 KB，另加实际 DMA 环形/块缓冲。

```c
SignalDDSGenerator_PrepareSine(&dds, wave_table, 256U, 12U,
    offset_fraction, amplitude_fraction, output_hz, update_rate_hz);
SignalDACDMA_Start(&dac_dma, wave_table, 256U, true);
```

## 8. 扫频仪

- 模块：`frequency_sweep -> DDS/DAC -> DUT -> ADC -> sine_fit/phase -> sweep_analyzer`
- 数据流：`frequency list -> stimulus -> response frame -> amplitude/phase -> gain table`
- 初始化：生成扫频列表；每频点更新 DDS，等待稳态，采一帧，复用同一工作区。
- 参数：起/止频率、线性/对数、点数、每点稳定时间、N/Fs。
- RAM：每点复用 N=1024 频谱/拟合区约 12 KB，结果表每点约 16 bytes。

```c
SignalFrequencySweep_Generate(&sweep_cfg, frequencies_hz, point_count);
for_each_frequency_settle_and_capture();
SignalSweepAnalyzer_Point(reference_amplitude, response_amplitude,
    phase_degrees, &point_result);
```

## 9. 单次触发

- 模块：`adc_ring_buffer/adc_pingpong_dma -> trigger_capture`
- 数据流：`continuous raw -> rolling history -> trigger -> pre/post-trigger frame`
- 初始化：初始化 ring/ping-pong，设置触发电平、迟滞、边沿和 pre-trigger 点数。
- 参数：capacity 必须大于 pre+post；触发搜索不要从尚未写入的区域开始。
- RAM：N=4096 的 ring raw 8 KB + 输出 raw 8 KB；可通过减小单次帧节省。

```c
SignalTrigger_Find(history, history_count, &trigger_cfg,
    search_start, &trigger_index);
SignalTrigger_Extract(history, history_count, trigger_index,
    pretrigger_count, captured, capture_count);
```

## 10. 波形捕获重放

- 模块：`配方9 -> waveform_capture_replay/arbitrary_wave -> dac_dma`
- 数据流：`captured ADC raw -> normalize/resample -> DAC table -> DMA repeat/one-shot`
- 初始化：先完成单次捕获，计算捕获 min/max，准备 DAC 更新链，再生成回放表。
- 参数：捕获长度、DAC 表长、DAC 位数、回放更新率、单次/循环。
- RAM：capture 4096 raw=8 KB，replay 4096 code=8 KB，约 16 KB；不要再同时保留 4096 点 float FFT 区。

```c
SignalWaveformReplay_Prepare(captured, captured_count, captured_min,
    captured_max, 12U, dac_table, dac_table_count);
SignalDACDMA_Start(&dac_dma, dac_table, dac_table_count, repeat);
```

## 通用初始化顺序

```text
SYSCFG_DL_init -> platform adapter -> acquisition/generator init
-> start/wait or consume frame -> conversion/measurement/DSP -> result/export
```

不测频谱就删窗/FFT/magnitude/peak；不连续就删 ping-pong/ring；不输出波形就删 generator/DAC；单通道就删 dual/correlation/channel-delay。
