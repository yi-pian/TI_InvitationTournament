# 信号处理 Pipeline 指南

## A. 测 DC

```text
ADC_DMA -> ADC_ToVoltage -> Mean
```

不要接 RemoveDC。增加点数可降低随机噪声，但会降低对快速变化的跟踪速度。

## B. 测 Vpp

正常信号：

```text
ADC_DMA -> ADC_ToVoltage -> MinMax/Vpp
```

有偶发毛刺：

```text
ADC_DMA -> ADC_ToVoltage -> Hampel -> RobustVPP
```

若尖峰就是目标，使用第一条链并改进采样带宽，不能先去毛刺。

## C. 测 RMS

总 RMS：

```text
ADC_DMA -> ADC_ToVoltage -> RMS
```

交流 RMS：

```text
ADC_DMA -> ADC_ToVoltage -> RemoveDC -> RMS
```

也可直接用 `AC_RMS` 同时得到 mean 与交流 RMS。记录尽量覆盖整数个完整周期。

## D. 精确测正弦频率

```text
ADC_DMA -> ADC_ToVoltage -> RemoveDC
        -> ZeroCross -> LinearInterpolation -> MultiCycleAverage
```

线性插值用过零前后两个样本估计“零点在两点之间的哪一小部分”；多周期平均用更长时间间隔除以周期数，减少单个过零的时间误差。

## E. 噪声较大的频率测量

```text
ADC_DMA -> ADC_ToVoltage -> RemoveDC -> Hann
        -> FFT -> Magnitude -> Peak -> ParabolicInterpolation
```

Hann 降低泄漏但加宽主瓣；三点插值估计峰值在 bin 之间的位置。结果分辨能力仍受记录时长和 SNR 限制。

## F. FFT 频谱

```text
ADC_DMA -> ADC_ToVoltage -> RemoveDC -> Window
        -> FFT -> Magnitude -> WindowGainCorrection
```

如果要保留 DC 频谱，不接 RemoveDC。若严格相干采样可考虑 Rectangular；未知频率一般先选 Hann。

## G. THD

```text
ADC_DMA -> ADC_ToVoltage -> RemoveDC -> Hann -> FFT
        -> Magnitude/MultiBinEnergy -> Harmonic -> THD
```

不要在前面随意低通，因为低通会降低真实谐波，得到虚假的低 THD。

## H. 相位

```text
DualADC -> 两路 ADC_ToVoltage -> 两路 RemoveDC -> Phase
```

三种方法的适用范围：

| 方法 | 适合 | 不适合/注意 |
|---|---|---|
| ZeroCross Phase | 纯正弦、边沿附近 SNR 高、RAM 紧 | 噪声会制造假过零，谐波会移动过零点 |
| FFT Phase | 已有 FFT、锁定单一频率、有噪声 | 频率 bin/窗和通道延时必须一致处理 |
| Cross Correlation Phase | 两路波形形状相似、可非正弦 | 计算量较大，周期信号可能有多个等价峰 |

## 一条实际基础链

```c
const uint16_t *raw = SignalADC_GetBuffer();
uint32_t count = (uint32_t)SignalADC_GetSampleCount();

if (SignalADCToVoltage_Process(raw, voltage_v, count, &convert_cfg)
        != SIGNAL_ALGORITHM_OK) {
    /* 参数、码值或数据错误 */
}

if (SignalACRMS_Process(voltage_v, count, &ac_rms_result)
        != SIGNAL_ALGORITHM_OK) {
    /* 不使用 ac_rms_result */
}
```

Pipeline 的每一条箭头都应核对：数据类型、单位、count、采样率、是否覆盖了输入 buffer。
