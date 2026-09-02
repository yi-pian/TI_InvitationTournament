# Module Interface Matrix — Frozen

冻结日期：2026-08-08。`直接`表示类型、形状、单位均相容；`Adapter` 表示只能经过列出的唯一转换；`禁止`表示语义错误。公开 API 无 breaking change。

## Hardware → Adapter → Algorithm

| A 输出 | B 输入 | 连接 | Adapter / 单位约束 |
|---|---|---|---|
| ADC DMA `uint16_t raw[N] + count + configured Fs` | ADC To Voltage raw | 直接 | 另给 bits/VREF/input scale/offset |
| ADC raw code | Mean/MinMax/VPP/RMS/ZeroCross/FFT float | 禁止 | 先用 `SignalIntegration_RawToVoltage`，code → V |
| ADC To Voltage `float voltage[N]` | Mean/MinMax/VPP/RMS/ACRMS/RemoveDC | 直接 | 输入输出单位 V |
| RemoveDC centered float | ZeroCross / Window / FFT | 直接 | threshold/hysteresis 同为 V |
| ZeroCross integer events + original samples | ZeroCrossInterpolation | 直接 | 同一 threshold 与边沿方向 |
| fractional crossing positions | MultiCycleAverage | 直接 | 只传同方向 crossing，同时给 Fs |
| Windowed float + coherent gain | FFT Real | 直接 | N=2^k；保留 gain 给幅值校正 |
| FFT `signal_complex_f32_t[N]` | FFTMagnitude | 直接 | output capacity ≥ N/2+1 |
| raw magnitude + N + coherent gain | GainCorrection | 直接 | 输出 one-sided peak amplitude |
| corrected magnitude | Peak/SNR/SFDR | 直接 | ranges/masks 使用同一 bin 定义 |
| Peak + adjacent bins | ParabolicInterpolation | 直接 | peak 不得为 0/last bin；给 Fs/N |
| magnitude + f0 | Harmonic/MultiBinEnergy | 直接 | radius 不重叠且不越 Nyquist |
| Harmonic result H1..Hn | THD | 直接 | 必须有有效 H1 与 H2+ |
| DualADC A/B 独立 raw + common Fs | Phase float A/B | Adapter | 各调用一次 RawToVoltage；禁止交织再拆 |
| dual centered float | Correlation | 直接 | N/Fs 相同 |
| correlation lag | CorrelationPhase | Adapter 参数 | 另给 period_samples；sample → deg |
| dual FFT bins | FFTPhase | 直接 | 同 N/Fs/window/bin；约定 B−A |
| P05 down-count capture register | forward timestamps | Adapter | ISR 执行 `modulus-1-capture` |
| forward timestamps + Timer config | MeanPeriod | 直接 | tick → Hz |
| Ring ordered raw segment | Trigger/Replay | 直接 | Ring 保存 N 点需 capacity=N+1 |
| Replay table | DAC DMA | 直接 | `uint16_t` DAC code + update rate |
| DDS Fill table | DAC DMA | 直接 | repeat block 要周期闭合 |
| LockIn amplitude/phase | SweepPoint | 直接 | Vpeak、deg；reference amplitude >0 |

## Backend 边界

| 上层 | 下层 | 连接 | 规则 |
|---|---|---|---|
| Application | `SignalFFT_*` 等正式算法 API | 直接 | 公共 float-compatible API 不变 |
| Application | `arm_cfft_*`, `_IQ*`, `DL_MATHACL_*` | 禁止 | Backend 不得泄漏到应用层 |
| `SIGNAL_FFT_BACKEND=2` | CMSIS Q31 implementation | 构建 Adapter | projectspec 提供 define、Core/DSP include、M0+ static library |
| `SIGNAL_MATH_BACKEND=0/1/2` | Reference/IQMath/MATHACL | 算法内部 | 应用只选择配置，不改调用 API |
| 外设 `signal_complex_f32_t` | 算法同名 type | 禁止混合 include | INT-001 继续用 raw/N/Fs 边界隔离 |
| Contest 旧算法副本 | final application link | 禁止 | INT-002 校验器拒绝该源集 |

## 完整 Pipeline

| Application | 数据流 | Profile | Full Link |
|---|---|---|---|
| Signal Meter | ADC DMA → RawToVoltage → Mean/MinMax/VPP/RMS/ACRMS/Time Frequency | P01 | PASS |
| Frequency A | Comparator → Timer Capture Adapter → MeanPeriod | P05 | PASS |
| Frequency B | ADC DMA → Voltage → RemoveDC → ZeroCross → Interpolation → Average | P01 | PASS |
| Frequency C | ADC DMA → Voltage → Window → FFT → Magnitude → Peak → Parabolic | P01/Q31 | PASS |
| Spectrum | ADC DMA → Voltage → RemoveDC → Window → FFT → Magnitude → Correction → Peaks | P01/Q31 | PASS |
| THD | ADC DMA → Voltage → RemoveDC → Window → FFT → Magnitude → Harmonics → THD | P01/Q31 | PASS |
| Phase | DualADC → two Voltage → RemoveDC → FFT Phase + Correlation Phase | P02/Q31 | PASS |
| DDS | Sine table → DDS Fill → DAC DMA → DAC platform | P03 | PASS |
| Sweep | DDS/DAC → external DUT → ADC → Voltage → LockIn → SweepPoint | P04 | PASS |
| Wave Replay | ADC → Ring/Trigger → Period → Resample/Normalize → DAC DMA | P04 | PASS |
| Signal Analyzer | P02 → selected Basic/Frequency/Spectrum/THD/Phase Profile | P02/Q31 | 5/5 PASS |
| Contest Template | P06 → selected Basic/Spectrum/THD/Phase → result hook | P06/Q31 | 4/4 PASS |

## Adapter 唯一实现位置

| 重复转换 | 唯一位置 |
|---|---|
| raw → voltage | `08_applications/common/signal_integration.*` 调正式 ADCToVoltage |
| DualADC hardware → two raw buffers | `signal_dual_adc_platform.*` |
| FFT complex → magnitude | 正式 `SignalFFTMagnitude_Process`，由 glue 调用 |
| DAC DMA callbacks → TI Timer/Event/DMA/DAC | `signal_dac_dma_platform.*` |
| capture down-count → forward timestamp | Frequency A ISR |

后续赛题发现不兼容时先登记 `INTEGRATION_ISSUE`，再修改唯一正式模块；不得把修正版模块复制进应用。
