# Contest Requirement → Module Map

| 题目关键词 | Hardware | Algorithm / Adapter | Recipe | 推荐 Application |
|---|---|---|---|---|
| DC / offset | ADC DMA | RawToVoltage + Mean | 01 | Signal Meter |
| Vpp / peak-to-peak | ADC DMA | RawToVoltage + MinMax/VPP | 02 | Signal Meter |
| 毛刺下的 Vpp | ADC DMA | Hampel + RobustPeakToPeak | 02/14 | Contest Template |
| RMS / True RMS | ADC DMA | RMS；非正弦要覆盖谐波带宽 | 03 | Signal Meter |
| AC RMS | ADC DMA | RemoveDC + RMS 或 ACRMS | 03 | Signal Meter |
| frequency（干净边沿） | Comparator + Timer Capture | down-count Adapter + MeanPeriod | 05 | Frequency Meter A |
| frequency（高 SNR 正弦） | ADC DMA | RemoveDC + ZeroCross + Interpolation + MultiCycleAverage | 04 | Frequency Meter B |
| frequency（噪声/失真） | ADC DMA | Window + FFT + Peak + Parabolic | 06 | Frequency Meter C |
| phase / delay | Dual ADC | 双 RawToVoltage + FFT Phase / Correlation Phase | 09 | Phase Meter |
| THD / H2~H5 | ADC DMA | FFT + Magnitude + Harmonic/MultiBinEnergy + THD | 08 | THD Analyzer |
| harmonic | ADC DMA | Window + FFT + Harmonic | 08 | THD Analyzer |
| spectrum / spur / peaks | ADC DMA | RemoveDC + Window + FFT + Magnitude + GainCorrection | 07 | Spectrum Analyzer |
| SNR / SFDR | ADC DMA | Spectrum + SNR/SFDR exclusion config | 07/15 | Signal Analyzer Spectrum |
| sweep / filter response | DAC/DMA + DUT + ADC DMA | DDS + LockIn + SweepPoint | 11 | Sweep Analyzer |
| waveform generation | DAC + DMA + Timer | Sine table + DDS | 10 | DDS Generator |
| arbitrary waveform | DAC + DMA | ArbitraryWave + DACDMA | 10/13 | Wave Replay / Template |
| waveform replay | ADC/DMA + Trigger + DAC/DMA | Ring + Segment + Resample/Normalize | 13 | Wave Capture Replay |
| burst / trigger | ADC DMA / Comparator Event | RingBuffer + TriggerCapture | 12 | Wave Capture Replay |
| weak signal / narrowband | ADC DMA；可加 DDS reference | FIR/IIR + LockIn | 15 | Sweep Analyzer / Template |
| unknown periodic weak signal | ADC DMA | RemoveDC + FFT/Autocorrelation | 15 | Signal Analyzer / Template |
| AM | ADC DMA；必要时 DDS | Envelope/FFT（先核对现有 API） | 07/15 | Contest Template |
| gain | ADC DMA；最好双通道 | RMS/LockIn amplitude ratio | 11 | Sweep Analyzer |
| filter response | DDS/DAC + DUT + ADC | Sweep + LockIn amplitude/phase | 11 | Sweep Analyzer |

选择原则：先满足数据获取的物理条件，再选算法。边沿足够干净时硬件 Capture 最省资源；未知/噪声信号用 FFT；已知单频弱信号用 Lock-in；双通道量相位必须先预算同步误差。模块名字存在不等于整机精度已实板验证。
