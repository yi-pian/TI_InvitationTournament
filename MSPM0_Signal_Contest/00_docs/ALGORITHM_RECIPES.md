# Algorithm Recipes：比赛 main.c 怎么拼积木

> **历史模块式拼装示例。** 本文件保留给已经使用旧 API 的 Application。新比赛工程先看 [SIGNAL_ALGORITHM_COOKBOOK.md](SIGNAL_ALGORITHM_COOKBOOK.md)：Mean、Vpp、RMS、ADC To Voltage、Remove DC 等已经改成 Direct Recipe，不再要求复制旧 `.c/.h` 和 result struct。

下面代码是“main.c 级连接示例”，硬件函数名仅表示 [HARDWARE_ALGORITHM_CONTRACT.md](HARDWARE_ALGORITHM_CONTRACT.md) 的 Expected Interface。另一个硬件任务最终若使用不同公开函数名，只改最前面的取 buffer 代码，不改算法源码。

每一步都应检查返回状态。为了突出连接关系，示例用 `CHECK_OK(expr)` 代表“若不为 `SIGNAL_ALGORITHM_OK`，立即停止本次结果并记录错误”。大型数组必须放静态区，不要放 ISR 栈。

## Recipe 1：基础电压测量

```text
ADC_DMA -> ADC_ToVoltage -> Mean / Vpp / RMS
```

```c
#define SAMPLE_COUNT 1024U
static float voltage_v[SAMPLE_COUNT];

void MeasureBasicVoltage(void)
{
    const uint16_t *raw = SignalADC_GetBuffer();       /* Expected Hardware */
    uint32_t count = SignalADC_GetSampleCount();
    signal_adc_to_voltage_config_t cvt = {4095U, 3.3f, 1.0f, 0.0f};
    signal_mean_result_t mean;
    signal_vpp_result_t vpp;
    signal_rms_result_t rms;

    if (count > SAMPLE_COUNT) return;
    CHECK_OK(SignalADCToVoltage_Process(raw,voltage_v,count,&cvt));
    CHECK_OK(SignalMean_Process(voltage_v,count,&mean));
    CHECK_OK(SignalVPP_Process(voltage_v,count,&vpp));
    CHECK_OK(SignalRMS_Process(voltage_v,count,&rms));
    /* mean.mean_value: V; vpp.amplitude_vpp: V; rms.rms_v: V */
}
```

总 RMS 包含 DC。只要交流 RMS 时改用 `SignalACRMS_Process()` 或 `RemoveDC -> RMS`。如果前端有稳定 gain/offset 误差，把 Calibration 放在转换后、测量前。

## Recipe 2：高精度正弦频率

```text
ADC_DMA -> ADC_ToVoltage -> RemoveDC -> ZeroCross(rising)
        -> LinearInterpolation -> MultiCycleAverage
```

```c
#define N 1024U
#define MAX_CROSSINGS 64U
static float voltage_v[N];
static signal_zero_cross_event_t events[MAX_CROSSINGS];
static float crossing_samples[MAX_CROSSINGS];

void MeasureFrequency(float sample_rate_hz)
{
    const uint16_t *raw = SignalADC_GetBuffer();
    uint32_t count = SignalADC_GetSampleCount();
    signal_adc_to_voltage_config_t cvt = {4095U,3.3f,1.0f,0.0f};
    signal_remove_dc_result_t dc;
    signal_zero_cross_config_t zc_cfg = {0.0f,0.002f,SIGNAL_ZERO_CROSS_RISING};
    signal_zero_cross_result_t zc;
    signal_zero_cross_interpolation_result_t interp;
    signal_multi_cycle_average_result_t frequency;

    if (count > N) return;
    CHECK_OK(SignalADCToVoltage_Process(raw,voltage_v,count,&cvt));
    CHECK_OK(SignalRemoveDC_Process(voltage_v,voltage_v,count,&dc));
    CHECK_OK(SignalZeroCross_Process(voltage_v,count,&zc_cfg,
                                     events,MAX_CROSSINGS,&zc));
    CHECK_OK(SignalZeroCrossInterpolation_Process(
        voltage_v,count,zc_cfg.threshold_v,events,zc.event_count,
        crossing_samples,MAX_CROSSINGS,&interp));
    CHECK_OK(SignalMultiCycleAverage_Process(
        crossing_samples,interp.position_count,sample_rate_hz,&frequency));
    /* frequency.frequency_hz 是最终 Hz；frequency.cycle_count 是平均周期数 */
}
```

有 DC 时先 RemoveDC，再用 threshold=0；也可不去 DC、直接传已知中心阈值，但二者只能选一种一致做法。只使用同方向过零，不能把 rising/falling 混入 MultiCycleAverage。

## Recipe 3：频谱分析

```text
ADC_DMA -> ADC_ToVoltage -> RemoveDC -> Hann -> FFT
        -> Magnitude -> WindowGainCorrection
```

```c
#define FFT_N 1024U
#define FFT_BINS (FFT_N/2U+1U)
static float samples_v[FFT_N];
static signal_complex_f32_t spectrum[FFT_N];
static float magnitude[FFT_BINS];

void AnalyzeSpectrum(void)
{
    signal_remove_dc_result_t dc;
    signal_window_result_t window;
    signal_fft_magnitude_result_t mag_result;
    /* raw/count/cvt 同 Recipe 1；必须保证 count == FFT_N */
    CHECK_OK(SignalADCToVoltage_Process(raw,samples_v,FFT_N,&cvt));
    CHECK_OK(SignalRemoveDC_Process(samples_v,samples_v,FFT_N,&dc));
    CHECK_OK(SignalHann_Apply(samples_v,samples_v,FFT_N,&window));
    CHECK_OK(SignalFFT_ForwardReal(samples_v,spectrum,FFT_N,FFT_N));
    CHECK_OK(SignalFFTMagnitude_Process(
        spectrum,FFT_N,magnitude,FFT_BINS,&mag_result));
    CHECK_OK(SignalWindowGainCorrection_Apply(
        magnitude,magnitude,FFT_BINS,FFT_N,window.coherent_gain));
    /* bin k 的频率 = k*sample_rate_hz/FFT_N；magnitude[k] 为单边峰值幅度 V */
}
```

这里 1024 点 Simple 数组约占 14,340 bytes（不含 RAW；若同时保留 RAW 则约 16,388 bytes），还需栈和硬件余量，详见 FFT_MEMORY_BUDGET。

## Recipe 4：THD（BASIC 与 COMPETITION）

```text
ADC_DMA -> Voltage -> RemoveDC -> Hann -> FFT -> Magnitude
        -> Harmonic/MultiBinEnergy -> THD
```

```c
void MeasureTHD(float sample_rate_hz, float fundamental_frequency_hz)
{
    signal_harmonic_config_t hc;
    signal_harmonic_result_t harmonics;
    signal_thd_result_t thd;

    /* 先执行 Recipe 3 到 raw magnitude；THD 比值中公共 FFT 标度会相消。 */
    hc.fundamental_frequency_hz = fundamental_frequency_hz;
    hc.first_order = 1U;
    hc.last_order = 5U;
    hc.radius_bins = 0U; /* BASIC：近相干采样，单 bin */
    CHECK_OK(SignalHarmonic_Process(magnitude,FFT_BINS,
              sample_rate_hz,FFT_N,&hc,&harmonics));
    CHECK_OK(SignalTHD_Process(&harmonics,&thd));

    hc.radius_bins = 2U; /* COMPETITION：Hann 主瓣多 bin 能量；先扫频验证 */
    CHECK_OK(SignalHarmonic_Process(magnitude,FFT_BINS,
              sample_rate_hz,FFT_N,&hc,&harmonics));
    CHECK_OK(SignalTHD_Process(&harmonics,&thd));
    /* thd.thd_percent: % */
}
```

BASIC 只读中心 bin，快但非整 bin 会漏能量。COMPETITION 用 true f0 映射每阶中心并积多个 bin，更抗泄漏；代价是收入更多噪声，且相邻谐波窗口不得重叠。绝不能在 THD 前加会衰减 H2~H5 的随意低通。

## Recipe 5：抗毛刺 Vpp

```text
ADC_DMA -> Voltage -> Hampel -> RobustVPP
```

```c
#define N 1024U
static float voltage_v[N];
static float filtered_v[N];
static float robust_workspace[N];

void MeasureRobustVPP(void)
{
    float hampel_workspace[7];
    signal_hampel_config_t hf = {7U,3.0f,0.001f};
    signal_hampel_result_t hf_result;
    signal_robust_peak_to_peak_config_t rc = {0.01f,0.99f};
    signal_robust_peak_to_peak_result_t vpp;

    CHECK_OK(SignalADCToVoltage_Process(raw,voltage_v,N,&cvt));
    CHECK_OK(SignalHampel_Process(voltage_v,filtered_v,N,&hf,
                                  hampel_workspace,7U,&hf_result));
    CHECK_OK(SignalRobustPeakToPeak_Process(filtered_v,N,&rc,
              robust_workspace,N,&vpp));
    /* 同时检查 hf_result.replaced_count；数量异常大时结果不可直接采用 */
}
```

这是主动修改尾部的强鲁棒链。只有确认尖峰是错误数据时使用；若题目测脉冲、过冲或峰值，禁止接 Hampel/RobustVPP。

## Recipe 6：双通道相位

### FFT Phase

```text
DualADC -> 两路 Voltage -> 两路 RemoveDC/Hann/FFT -> 同一 bin -> FFT Phase
```

```c
signal_phase_result_t phase;
/* ch_a/ch_b 必须同步、同 N、同窗；分别生成 spectrum_a/spectrum_b */
CHECK_OK(SignalPhase_FromFFTBin(
    spectrum_a,spectrum_b,FFT_N,fundamental_bin,&phase));
/* phase.phase_difference_deg = phase_B - phase_A */
```

### Cross-Correlation Phase

```c
#define MAX_LAG 32U
static float corr[2U*MAX_LAG+1U];
signal_correlation_result_t cr;
signal_phase_result_t phase;

CHECK_OK(SignalRemoveDC_Process(ch_a_v,ch_a_v,N,&dc_a));
CHECK_OK(SignalRemoveDC_Process(ch_b_v,ch_b_v,N,&dc_b));
CHECK_OK(SignalCorrelation_Process(ch_a_v,ch_b_v,N,MAX_LAG,
                                    corr,2U*MAX_LAG+1U,&cr));
CHECK_OK(SignalPhase_FromCorrelationLag(
    (float)cr.best_lag_samples,period_samples,&phase));
```

FFT Phase 适合锁定单一频率；Correlation 适合形状相似的非正弦，但计算量为 O(N·lag)。若通道存在固定 delay，最后接 ChannelDelayCalibration；约定和选择表见 PHASE_METHOD_SELECTION。

## Recipe 7：低 SNR 周期信号频率

```text
ADC_DMA -> Voltage -> RemoveDC -> Bandpass(FIR external coefficients)
        -> Autocorrelation / FFT -> Frequency
```

本库没有“写死截止频率”的 Bandpass 黑盒。用已经离线设计、验证过的外部 FIR 系数：

```c
static const float bandpass_taps[TAP_COUNT] = { /* 离线设计并验证 */ };
static float fir_state[TAP_COUNT];
static float filtered_v[N];
static float ac[MAX_LAG+1U];

signal_fir_t fir;
signal_autocorrelation_result_t ac_result;
signal_autocorrelation_period_result_t period;

CHECK_OK(SignalFIR_Init(&fir,bandpass_taps,TAP_COUNT,
                        fir_state,TAP_COUNT));
CHECK_OK(SignalFIR_Process(&fir,centered_v,filtered_v,N));
CHECK_OK(SignalAutocorrelation_Process(
    filtered_v,N,MAX_LAG,ac,MAX_LAG+1U,&ac_result));
CHECK_OK(SignalAutocorrelation_FindPeriod(
    ac,ac_result.lag_count,min_lag,max_lag,sample_rate_hz,&period));
/* period.frequency_hz */
```

`min_lag≈floor(Fs/f_max)`、`max_lag≈ceil(Fs/f_min)`，避免把 lag=0 附近宽峰当周期。FIR 会改变幅相但周期频率通常保留；仍须验证通带、瞬态和群延迟。也可把滤波输出接 Hann→FFT→Peak→Interpolation。

## Recipe 使用纪律

1. 每条链先保存无处理结果作对照。
2. 只修改一个参数并用已知真值扫频/扫幅。
3. 检查每个状态码、buffer 容量、单位和 in-place 规则。
4. PC_VERIFIED 不等于硬件正确；Vref、采样时钟、模拟带宽、同步误差必须实板标定。
5. 比赛现场优先组合已在赛前用你的板、函数发生器和示波器验证过的短链。
