# 阶段五：FFT 频谱与失真分析

## 当前处理链

```text
校准电压 -> Remove DC Recipe -> Window -> CMSIS Q15 CFFT / Complex Magnitude
-> 窗相干增益修正 -> Peak Detect Recipe -> 抛物线插值 -> Harmonic / THD / SNR / SFDR
```

`main.c` 采用 `00_docs/recipes/remove_dc.md`、`cmsis_fft_spectrum.md` 和 `peak_detect.md`，不再使用 `signal_fft.c/.h`、`signal_fft_magnitude.c/.h`、`signal_peak_detect.c/.h` 等冻结封装：

```c
arm_mean_f32(g_calibrated, SIGNAL_SAMPLE_COUNT, &g_mean_v);
arm_offset_f32(g_calibrated, -g_mean_v, g_centered, SIGNAL_SAMPLE_COUNT);
SignalWindow_Apply(g_centered, g_voltage, SIGNAL_SAMPLE_COUNT,
    g_window, &window_result);
App_RecipeCMSISSpectrumQ15(g_voltage, SIGNAL_SAMPLE_COUNT, g_magnitude);
SignalWindowGainCorrection_Apply(g_magnitude, g_magnitude,
    (SIGNAL_SAMPLE_COUNT / 2U) + 1U, SIGNAL_SAMPLE_COUNT,
    window_result.coherent_gain);
g_fft_peak.bin = App_RecipePeakIndex(g_magnitude, 1U,
    SIGNAL_SAMPLE_COUNT / 2U, &g_fft_peak.peak_value);
SignalFFTParabolicInterpolation_Process(g_magnitude,
    (SIGNAL_SAMPLE_COUNT / 2U) + 1U, g_fft_peak.bin,
    (float)g_effective_sample_rate_hz, SIGNAL_SAMPLE_COUNT,
    &g_fft_interpolated);
```

Q15 输入先按本帧绝对峰值归一化，FFT 后按相同规则恢复伏特量纲；因此原有的窗增益修正、基波/H2/H3、THD、SNR、SFDR 和显示功能仍接收电压幅值，不会把 Q15 整数当作伏特。

`SignalHarmonic_Process`、`SignalTHD_Process`、`SignalSNR_Process`、`SignalSFDR_Process` 与正式的抛物线插值模块保留不变。削顶提示按 `clipping_detect.md` 在基础测量阶段、滤波前得到，页面仍显示 `CLIPPING` 或 `SAFE`。

## 硬件前提

SysConfig 必须继续生成 CMSIS-DSP 链接配置，并保留 ADC DMA。窗口按键仍为 `8/9/0/#` 对应 Rectangular/Hann/Hamming/Blackman；CFFT 长度由当前 `SIGNAL_SAMPLE_COUNT=512` 决定。
