# System Recipes

这是“题目 → 现有积木 → 可构建应用”的最终入口。路径中的算法模块均来自唯一正式仓库 `../MSPM0_Signal_Contest/`；外设与 Adapter 来自本仓库。示例只展示应用层调用形状，不替代错误检查和工作区容量检查。

通用规则：`Fs ≥ 2 × 最高输入频率` 只是理论下限；测量通常取 `5~10 ×`。`N/Fs` 决定单帧时长，`Fs/N` 决定 FFT bin 间隔。FFT 默认 CMSIS Q31；单通道优先 N=1024，双通道相位优先 N=512。所有未实板项目均为 `PENDING_BOARD`。

## RECIPE 01 — DC 电压

1. **关键词**：DC、平均值、偏置、静态电压。
2. **外设**：ADC DMA（P01）。
3. **算法**：ADC To Voltage、Mean。
4. **数据流**：`ADC_DMA raw(uint16_t) → voltage(float,V) → mean(float,V)`。
5. **推荐 Fs**：只测 DC 时 1~10 kS/s；有工频干扰时让记录覆盖整数个 20/16.67 ms 周期。
6. **推荐 N**：256~1024。
7. **关键参数**：ADC bits、VREF、输入衰减/增益、offset、校准系数。
8. **RAM**：约 `2N + 4N = 6N` B，加驱动和栈。
9. **CPU 风险**：低。
10. **精度风险**：VREF、前端增益、ADC offset、工频未平均完整周期。
11. **延迟**：约 `N/Fs`。
12. **常见错误**：直接把 ADC code 当 V；把软件配置 VREF 当作实测 VREF。
13. **备用方案**：多帧 Mean 后再平均；先执行 ADC gain/offset calibration。
14. **main.c 调用**：`SignalIntegration_RawToVoltage(...); SignalMean_Process(voltage, N, &mean_v);`
15. **对应应用**：`08_applications/signal_meter/`。

## RECIPE 02 — Vpp

1. **关键词**：峰峰值、Vpp、最大最小、毛刺抑制。
2. **外设**：ADC DMA（P01）。
3. **算法**：普通链用 ADC To Voltage、MinMax/VPP；抗毛刺链用 Hampel、Robust Peak To Peak。
4. **数据流**：普通 `raw → V → MinMax/VPP`；鲁棒 `raw → V → Hampel → RobustVPP`。
5. **推荐 Fs**：至少最高频率的 10 倍，窄脉冲需按脉宽而非基波选 Fs。
6. **推荐 N**：覆盖至少 5~10 个周期；典型 512/1024。
7. **关键参数**：Hampel 奇数窗口、sigma、RobustVPP 上下分位数。
8. **RAM**：普通约 6N B；Hampel/分位数还需输入、输出、`N` 工作区，约 14~18N B。
9. **CPU 风险**：普通低；Hampel/分位数排序中高。
10. **精度风险**：采样未命中尖峰；鲁棒算法会主动忽略真实窄脉冲。
11. **延迟**：普通 `N/Fs`；鲁棒链增加排序时间。
12. **常见错误**：在题目要求真实峰值时使用 RobustVPP；Hampel 输入输出重叠。
13. **备用方案**：模拟峰值保持、提高 Fs，或只对已确认的孤立毛刺启用 Hampel。
14. **main.c 调用**：`SignalVPP_Process(voltage, N, &vpp_v);`；鲁棒链调用 `SignalHampel_Process(...)` 后 `SignalRobustPeakToPeak_Process(...)`。
15. **对应应用**：普通链 `signal_meter`；鲁棒链由 `signal_contest_template` 按题装配。

## RECIPE 03 — RMS / AC RMS

1. **关键词**：RMS、有效值、True RMS、交流有效值。
2. **外设**：ADC DMA。
3. **算法**：ADC To Voltage、RMS；AC RMS 可用 AC RMS 或 RemoveDC + RMS。
4. **数据流**：Total `raw → V → RMS`；AC `raw → V → RemoveDC → RMS`。
5. **推荐 Fs**：最高有效谐波的 5~10 倍，而不只是基波的 2 倍。
6. **推荐 N**：覆盖整数个周期；未知频率时 1024 起步。
7. **关键参数**：VREF、scale、offset、是否移除 DC、目标带宽。
8. **RAM**：约 6N B；原地 RemoveDC 不再增加一帧。
9. **CPU 风险**：低；Math Backend 的 sqrt 可配置但默认 Reference。
10. **精度风险**：带宽不足会漏掉谐波能量；削顶会严重偏差。
11. **延迟**：`N/Fs`。
12. **常见错误**：把 AC RMS 当 Total RMS；只按基波选择 Fs 测非正弦 True RMS。
13. **备用方案**：Robust RMS 仅用于确认存在离群毛刺的场景；不能用于真实冲击能量。
14. **main.c 调用**：`SignalRMS_Process(voltage, N, &rms_v);` 或 `SignalRemoveDC_Process(...); SignalRMS_Process(...);`
15. **对应应用**：`signal_meter`、`signal_analyzer` Basic Profile。

## RECIPE 04 — 高精度正弦测频

1. **关键词**：正弦、频率高精度、高 SNR、多周期平均。
2. **外设**：ADC DMA（P01）。
3. **算法**：ADC To Voltage、RemoveDC、ZeroCross、ZeroCross Interpolation、MultiCycleAverage。
4. **数据流**：`raw → V → centered V → crossings → fractional sample → Hz`。
5. **推荐 Fs**：目标最高频率的 10~20 倍。
6. **推荐 N**：至少覆盖 8~20 个周期，且 crossing 数不超过工作区。
7. **关键参数**：hysteresis、边沿方向、预期频率范围。
8. **RAM**：约 raw 2N + voltage 4N + events 6N + positions 2N，约 14N B。
9. **CPU 风险**：低到中。
10. **精度风险**：噪声导致重复 crossing；波形失真使阈值相位漂移。
11. **延迟**：至少若干周期，约 `N/Fs`。
12. **常见错误**：混合上升/下降沿平均；RemoveDC 后仍使用原 DC 阈值。
13. **备用方案**：噪声大用 FFT；方波/边沿干净且低延迟用 Recipe 05。
14. **main.c 调用**：`SignalIntegration_FrequencyTime(voltage, N, Fs, hysteresis, events, ..., positions, ..., &frequency_hz);`
15. **对应应用**：`frequency_meter` Method B、`signal_analyzer` Frequency Profile。

## RECIPE 05 — 硬件高速测频

1. **关键词**：方波、脉冲、边沿、低延迟、高频、占用 CPU 少。
2. **外设**：Comparator、Event、Timer Capture（P05）。
3. **算法**：Timer Capture MeanPeriod / Frequency。
4. **数据流**：`analog edge → comparator → capture ticks → forward timestamps → Hz`。
5. **推荐 Fs**：无 ADC Fs；Timer clock 应使最短周期仍有足够 tick。
6. **推荐 N**：捕获 4~32 个同向边沿。
7. **关键参数**：比较器阈值/滞回、Timer Hz、counter modulus、边沿数。
8. **RAM**：仅时间戳，通常 <256 B。
9. **CPU 风险**：低；极高边沿率要检查 ISR/捕获溢出。
10. **精度风险**：比较器传播延迟、阈值噪声、Timer 参考时钟误差。
11. **延迟**：几个周期，可远低于块 ADC。
12. **常见错误**：P05 为向下计数却直接发布寄存器值；忘记回绕 modulus。
13. **备用方案**：无干净边沿时用 Recipe 04 或 06。
14. **main.c 调用**：ISR 先转换为正向时间戳，再调用 `SignalTimerCapture_MeanPeriod(timestamps, count, &cfg, &ticks, &frequency_hz);`
15. **对应应用**：`frequency_meter` Method A。

## RECIPE 06 — FFT 测频

1. **关键词**：噪声较大、未知波形、频率、主峰、失真正弦。
2. **外设**：ADC DMA。
3. **算法**：ADC To Voltage、RemoveDC、Window、FFT、Magnitude、PeakDetect、ParabolicInterpolation。
4. **数据流**：`raw → V → AC → window → complex spectrum → magnitude → bin → fractional bin/Hz`。
5. **推荐 Fs**：覆盖最高目标频率并留抗混叠裕量，通常 `Fs ≥ 2.5~5 fmax`。
6. **推荐 N**：1024 单通道默认；512 低延迟；当前 Simple Pipeline 禁止 2048/4096 作为默认。
7. **关键参数**：预期频率范围、窗口、FFT Backend、N。
8. **RAM**：典型 `raw 2N + voltage 4N + complex 8N + magnitude≈2N`，约 16N B。
9. **CPU 风险**：中；Q31 CMSIS 推荐，仍需检查采样周期内 deadline。
10. **精度风险**：bin 泄漏、近邻双音、峰在边界、时钟误差。
11. **延迟**：`N/Fs + FFT`。
12. **常见错误**：对 raw 直接 FFT；把 magnitude 当 V；搜索包含 DC/Nyquist 边界。
13. **备用方案**：单一高 SNR 正弦用 Recipe 04；非正弦周期信号可加 Autocorrelation。
14. **main.c 调用**：`SignalIntegration_Spectrum(voltage, N, Fs, fmin, fmax, fft, N, mag, N/2+1, 1, &result);`
15. **对应应用**：`frequency_meter` Method C、`spectrum_analyzer`。

## RECIPE 07 — 频谱

1. **关键词**：频谱、主峰、杂散、多音、带宽。
2. **外设**：ADC DMA。
3. **算法**：ADC To Voltage、RemoveDC、Hann/Window、FFT、Magnitude、WindowGainCorrection、PeakDetect。
4. **数据流**：`raw → V → AC → window → FFT → raw magnitude → one-sided corrected amplitude → peaks`。
5. **推荐 Fs**：目标带宽的 2.5~5 倍，前端必须抗混叠。
6. **推荐 N**：由 `Δf=Fs/N` 反推；默认 1024。
7. **关键参数**：窗口、峰数量、bin 搜索范围、相邻峰最小间隔。
8. **RAM**：约 16N B；N=1024 实测应用 SRAM 17,045 B。
9. **CPU 风险**：中。
10. **精度风险**：窗增益未校正、谱峰相互泄漏、ADC/DAC 前端频响。
11. **延迟**：`N/Fs + FFT + peak scan`。
12. **常见错误**：将未归一化 DFT magnitude 直接报告为 V；忽略 Nyquist 与窗函数 ENBW。
13. **备用方案**：只测已知单频幅相时用 Lock-in，RAM/CPU 更低。
14. **main.c 调用**：`SignalIntegration_Spectrum(..., requested_peak_count, &spectrum);`
15. **对应应用**：`spectrum_analyzer`、`signal_analyzer` Spectrum Profile。

## RECIPE 08 — THD

1. **关键词**：THD、H2~H5、谐波、失真度。
2. **外设**：ADC DMA。
3. **算法**：ADC To Voltage、RemoveDC、Hann、FFT、Magnitude、MultiBinEnergy/Harmonic、THD。
4. **数据流**：`raw → V → AC → Hann → FFT → magnitude → H1..H5 energy → THD%`。
5. **推荐 Fs**：至少覆盖最高计算谐波；测 H5 时 `Fs/2 > 5 f0` 并留裕量。
6. **推荐 N**：1024 默认；频率分辨率不足时降低 Fs 或采用相干采样。
7. **关键参数**：f0 搜索范围、谐波阶数、每阶 bin radius、窗口 coherent gain。
8. **RAM**：约 16N B；N=1024 实测 16,961 B。
9. **CPU 风险**：中。
10. **精度风险**：高次谐波越过 Nyquist、各谐波 bin 区间重叠、噪声底并入能量。
11. **延迟**：一帧加 FFT/能量扫描。
12. **常见错误**：直接把单 bin 高度当谐波 RMS；未校正 one-sided amplitude。
13. **备用方案**：已知 f0 且弱谐波时逐阶 Lock-in；但结果定义需与题目一致。
14. **main.c 调用**：`SignalIntegration_THD(voltage, N, Fs, fmin, fmax, radius, fft, N, mag, N/2+1, &thd);`
15. **对应应用**：`harmonic_thd_analyzer`、`signal_analyzer` THD Profile。

## RECIPE 09 — 双通道相位

1. **关键词**：相位差、延迟、双通道、B 相对 A。
2. **外设**：Dual ADC + 双 DMA（P02）。
3. **算法**：两次 ADC To Voltage、RemoveDC；FFT Phase 与 Cross Correlation Phase。
4. **数据流**：`raw A/B → V A/B → centered → FFT bins / correlation lag → B−A degrees`。
5. **推荐 Fs**：目标最高频率的 10~20 倍。
6. **推荐 N**：512 默认；1024 可链接但 Phase 仅余 3,040 B SRAM。
7. **关键参数**：已知频率、最大 lag、通道 delay calibration、符号约定 B−A。
8. **RAM**：双 raw、双 voltage、双 complex FFT、correlation；N=512 实测 15,392 B。
9. **CPU 风险**：相关法随 `N×lag` 增大，应限制 lag。
10. **精度风险**：DualADC skew、前端群延迟、两通道增益/offset 差异。
11. **延迟**：一帧；相关法另加扫描时间。
12. **常见错误**：两通道使用不同 Fs/N/window；把 A−B 与 B−A 混淆。
13. **备用方案**：稳定正弦可用 ZeroCross Phase；已知参考可用 Lock-in Phase。
14. **main.c 调用**：`SignalIntegration_DualPhase(a_v, b_v, N, Fs, f0, max_lag, fft_a, fft_b, N, corr, cap, &phase);`
15. **对应应用**：`dual_channel_phase_meter`、`signal_analyzer` Phase Profile。

## RECIPE 10 — DDS 信号源

1. **关键词**：正弦输出、可调频率/幅度/偏置/相位、信号源。
2. **外设**：DAC、DMA、Timer/Event（P03）。
3. **算法/生成**：Sine WaveTable、DDS、DAC DMA wrapper。
4. **数据流**：`parameters → lookup table → DDS phase accumulator → DAC codes → DAC DMA`。
5. **推荐 Fs**：DAC update rate 至少输出频率的 20~50 倍，并满足 Timer 分频。
6. **推荐 N**：table 256；DMA block 1000 为当前基线，可按闭合周期调整。
7. **关键参数**：update rate、frequency、amplitude、offset、phase、DAC VREF/bits。
8. **RAM**：当前实测 3,244 B；大 Buffer 为 2,000 B DMA block 与 512 B table。
9. **CPU 风险**：DMA repeat 时低；更新频率时需安全停止/重填。
10. **精度风险**：DAC settling、更新时钟、查表量化、重复块边界杂散。
11. **延迟**：一次填充后连续 DMA。
12. **常见错误**：幅度+offset 越过 DAC 范围；repeat block 非整数周期闭合。
13. **备用方案**：低频静态 DC 用 DAC DC；任意波用 ArbitraryWave + DAC DMA。
14. **main.c 调用**：`SignalDDS_SetFrequency(&dds, f, update_rate); SignalDDS_Fill(&dds, block, count); SignalDACDMA_Start(&dma, block, count, true);`
15. **对应应用**：`dds_generator`。

## RECIPE 11 — 扫频响应

1. **关键词**：扫频、频率响应、滤波器、放大器增益/相位、Bode 数据。
2. **外设**：DDS + DAC/DMA + ADC DMA（P04）；DUT 是外部模拟网络。
3. **算法**：FrequencySweep、ADC To Voltage、LockIn、Sweep point result。
4. **数据流**：`DDS → DAC → DUT → ADC → lock-in amplitude/phase → gain/result → next f`。
5. **推荐 Fs**：ADC/DAC rate 均至少最高 sweep 频率的 10~20 倍；当前示例 100 kS/s。
6. **推荐 N**：每点覆盖至少 10 周期；当前 1000。
7. **关键参数**：START/STOP/POINTS 或 STEP、settling time、激励幅度、参考相位。
8. **RAM**：实测 9,687 B；主要为 4,096 B voltage、2,048 B raw、2,000 B DDS block。
9. **CPU 风险**：每点低到中；总测试时间随点数和 settling 线性增加。
10. **精度风险**：参考幅度不是 DUT 输入实测值、通道/线缆延迟、DUT 未稳定。
11. **延迟**：每点约 `settling + N/Fs`。
12. **常见错误**：把内部 DDS 设定幅度当作 DUT 端真实幅度而不校准；扫到前端带宽之外。
13. **备用方案**：若同时采参考/响应两路，改 P02/P06 并取幅值比与相位差。
14. **main.c 调用**：`SignalLockIn_Process(...); SignalSweepAnalyzer_PointAtFrequency(f, ref_amp, response_amp, phase, &point);`
15. **对应应用**：`sweep_analyzer`。

## RECIPE 12 — 触发采集

1. **关键词**：触发、突发、单次事件、前触发/后触发、burst。
2. **外设**：ADC DMA、Ring Buffer；必要时 Comparator/Event 硬件触发。
3. **算法/采集**：TriggerCapture、RingBuffer。
4. **数据流**：`continuous raw → ring → edge/level trigger → ordered capture segment`。
5. **推荐 Fs**：按脉冲最窄细节选取，至少每个最短特征 10 点。
6. **推荐 N**：由前触发+后触发时间乘 Fs 得到。
7. **关键参数**：trigger level/hysteresis/edge、pre/post samples、ring capacity。
8. **RAM**：至少 `2N` B；若同时保留 DMA 与有序帧则约 `4N` B。
9. **CPU 风险**：块后软件查找低；逐点软件触发在高 Fs 下风险高。
10. **精度风险**：阈值抖动、触发死区、RingBuffer 覆盖策略。
11. **延迟**：事件后等待 post-trigger 样本。
12. **常见错误**：RingBuffer 用 N 槽却期望保存 N 个元素；当前实现需 N+1。
13. **备用方案**：低延迟用 Comparator + Event 触发 ADC/Timer。
14. **main.c 调用**：`SignalADCRing_Push(...); SignalTrigger_Find(...); SignalTrigger_Extract(...);`
15. **对应应用**：`waveform_capture_replay` 的采集前半链。

## RECIPE 13 — 波形捕获重放

1. **关键词**：捕获、复制、周期提取、波形回放、任意波。
2. **外设**：ADC DMA、RingBuffer、Trigger、DAC DMA（P04）。
3. **算法/生成**：Period/segment selection、ArbitraryWave linear resample、auto-range normalize。
4. **数据流**：`ADC → trigger → two same-edge crossings → one period → resample/normalize → DAC DMA`。
5. **推荐 Fs**：输入最高细节 10~20 倍；DAC update rate按 `capture_Fs × replay_N / captured_period_N` 计算。
6. **推荐 N**：capture 2048、replay table 512 为当前默认。
7. **关键参数**：trigger level/hysteresis/edge、minimum period、DAC bits、repeat。
8. **RAM**：实测 18,173 B；五个主要 Buffer 共约 17 KiB。
9. **CPU 风险**：一次性线性重采样低；连续重捕获需检查周转时间。
10. **精度风险**：仅按两次 crossing 估计周期；归一化会改变原绝对幅度/offset。
11. **延迟**：一帧采集 + trigger scan + resample。
12. **常见错误**：输入非周期/多周期变化仍强制抽一个周期；把 normalize 后输出当绝对电压重放。
13. **备用方案**：保留绝对幅度时禁用 auto-range，按 VREF/前端比例映射 DAC code。
14. **main.c 调用**：`SignalWaveformReplay_PrepareAutoRange(segment, period_N, dac_bits, table, replay_N, &min, &max);`
15. **对应应用**：`waveform_capture_replay`。

## RECIPE 14 — 抗毛刺参数测量

1. **关键词**：偶发毛刺、离群点、稳健/鲁棒 Vpp、鲁棒 RMS。
2. **外设**：ADC DMA。
3. **算法**：ADC To Voltage、Hampel、RobustPeakToPeak、RobustRMS。
4. **数据流**：`raw → V → outlier filtering/winsorization → robust metric`。
5. **推荐 Fs**：仍按真实信号带宽选，不能用低 Fs“滤掉”毛刺。
6. **推荐 N**：512~2048；分位数需要有足够样本。
7. **关键参数**：Hampel window/sigma/min scale、上下 quantile、remove_dc。
8. **RAM**：多个 float buffer，先按 16~20N B 预算。
9. **CPU 风险**：排序/滑窗中高，不适合无间隔高速逐帧。
10. **精度风险**：真实脉冲会被误判；输出不再是物理最大/最小。
11. **延迟**：一帧加滑窗/排序。
12. **常见错误**：为了好看在所有题中默认启用；输入/输出违反不重叠契约。
13. **备用方案**：中值滤波、模拟限幅，或同时报告 raw metric 与 robust metric。
14. **main.c 调用**：`SignalHampel_Process(in, filtered, N, &cfg, workspace, cap, &h); SignalRobustRMS_Process(filtered, N, &r_cfg, workspace, cap, &r);`
15. **对应应用**：从 `signal_contest_template` 按题组合；当前没有伪造独立“万能鲁棒应用”。

## RECIPE 15 — 低 SNR 已知/未知信号

1. **关键词**：弱信号、低 SNR、噪声底、窄带检测、相关检测。
2. **外设**：ADC DMA；有参考激励时可加 DDS/DAC（P04）。
3. **算法**：已知单频优先 FIR/IIR + LockIn；周期未知可 RemoveDC + FFT/Autocorrelation；已知模板可 Correlation。
4. **数据流**：`raw → V → appropriate filter → FFT / Correlation / LockIn → result`。
5. **推荐 Fs**：只覆盖必要带宽；过高 Fs 会把更多噪声带入且缩短同 N 观测时间。
6. **推荐 N**：使观测覆盖足够积分周期；512~1024 起步，按 RAM 与响应时间权衡。
7. **关键参数**：目标带宽/频率、filter coefficients、reference phase、correlation lag、SNR exclusion bins。
8. **RAM**：LockIn 约 6N B；FFT 约 16N B；Correlation 另加 lag workspace。
9. **CPU 风险**：长 FIR、全 lag correlation、FFT 同开时高；只选择一种主链。
10. **精度风险**：频率失配导致 LockIn 衰减；噪声非白；前端自身噪声和动态范围。
11. **延迟**：积分时间越长，检测更稳但响应越慢。
12. **常见错误**：在未知频率上直接用 LockIn；同时运行所有方法；把算法 SNR 当整机仪器 SNR。
13. **备用方案**：已知窄频区可先 FFT 找粗频，再用 LockIn/拟合精化；失真周期信号用 Autocorrelation。
14. **main.c 调用**：已知频率 `SignalLockIn_Process(voltage, N, &cfg, &result);`；未知周期 `SignalAutocorrelation_Process(...); SignalAutocorrelation_FindPeriod(...);`
15. **对应应用**：`sweep_analyzer`（Lock-in）、`signal_analyzer` Spectrum Profile；按题从 Contest Template 装配。

## 选择约束摘要

- 不要在 `main.c` 重写 raw→V、DualADC 拆分、FFT magnitude 或 DAC/DMA 搬运循环；使用正式 API/Adapter。
- FFT 应用层只能调用 `SignalFFT_*` 或 `SignalIntegration_*`，不能直接调用 CMSIS。
- 题目要求真实瞬态峰值、burst 或脉冲时，禁止默认启用 Hampel/Robust 算法。
- 所有 Fs、N、VREF、频率范围和功能开关放在应用 `signal_config.h`/`signal_features.h`；引脚、DMA channel、Event route 必须改 SysConfig。
