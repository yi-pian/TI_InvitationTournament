# 常用信号链方案

以下是“选择和复制函数”的离线方案，不是新架构或自动调度器。函数、变量与模块仍来自相应教学工程的 `main.c`。

## 方案 1：FFT 测频

目标：未知周期信号 → `frequency_hz`。

```text
ADC DMA
  ↓ AcquireADCFrame()                （02 或 20）
PrepareSignal()                      （20：code → V、mean、去 DC）
  ↓ centered_samples[]
RunFFTCommon()                       （20：唯一 FFT）
  ↓ fft_magnitude[]
MeasureFFTFrequency()
  ↓ frequency_hz
RefineFFTFrequency()                 （可选：插值精修）
```

需要工程：`02_adc_dma`、`20_fft_analysis`。复制时还需 `RunQ15FFT()` 及 20 README 中的 FFT/Window 模块。注意：不要复制一份 Basic 的 ADC 转换后又运行 20 的 `PrepareSignal()`。

## 方案 2：FFT + 幅值 + THD/SNR/SFDR

目标：同一帧得到 `mean_v`、`vpp_v`、`rms_v`、`frequency_hz`、THD、SNR、SFDR。

```text
AcquireADCFrame()
  ↓
PrepareSignal()（20，唯一 code → V / 去 DC）
  ├── MeasureBasicParameters()（30；输入 voltage_samples[]）
  └── RunFFTCommon()（20，唯一 FFT）
        ├── MeasureFFTFrequency()
        ├── RefineFFTFrequency()
        ├── AnalyzeHarmonicsAndTHD()
        └── AnalyzeSNRAndSFDR()
```

需要工程：`02_adc_dma`、`20_fft_analysis`、`30_basic_measurement`。注意：Basic 只复用 `MeasureBasicParameters()`，不再复制 `ConvertADCToVoltage()`；同帧 `mean_v` 的重复标量计算不影响频谱，但若要严格去重，应由合并工程把 Basic 的 mean 步骤改为复用 `PrepareSignal()` 已算的 `mean_v`，并明确保留原公式。

## 方案 3：过零插值测频 + TFT 显示

目标：低/中频平滑周期信号 → `frequency_hz` 并显示。

```text
AcquireADCFrame()
  ↓
PrepareSignal()（11）
  ↓ centered_samples[]
MeasureFrequencyZeroCross()
  ↓ frequency_hz
InitTFTDemo()（一次） → UpdateLiveValue()
```

需要工程：`11_zero_cross_frequency`、`80_tft_usage`。注意：过零法依赖足够的上升过零事件；削顶、噪声或信号不跨阈值时优先改用 FFT 或 Timer。

## 方案 4：双通道相位差

目标：同步两路周期信号 → `phase_deg`、`delay_s`。

```text
InitDualADC()（一次）
  ↓
AcquireDualADCFrame()
  ↓ adc_ch1_samples[] / adc_ch2_samples[]
MeasurePhase()
  ↓ phase_deg
CalculateDelayFromPhase()（已知 reference_frequency_hz）
  ↓ delay_s
```

需要工程：`04_dual_adc_dma`、`40_dual_channel_measurement`。注意：不能以两次单 ADC 采样代替同步帧。

## 方案 5：频谱 + 时域波形显示

目标：一帧 ADC 同时画时域图与频谱，输出频率/THD。

```text
AcquireADCFrame()（一次）
  ├── DrawTimeDomainWaveform()（读 adc_samples[]）
  └── PrepareSignal() → RunFFTCommon()（一次）
        ├── MeasureFFTFrequency()
        ├── AnalyzeHarmonicsAndTHD()
        └── DrawFFTSpectrum()
```

需要工程：`20_fft_analysis`、`21_time_domain_waveform`、`80_tft_usage`。注意：时域图不应创建第二个 DMA 帧；频谱显示不应执行第二次 FFT。

## 方案 6：抗干扰幅值稳定测量

目标：带尖峰/偶发离群点的电压 → 稳健 Vpp/RMS。

```text
AcquireADCFrame()
  ↓
ConvertADCToVoltage()
  ↓ voltage_samples[]
ApplySelectedFilter()（RAW / Median / Hampel 选一）
  ↓ filtered_samples[]
AnalyzeRobustStatistics()
  ↓ mad_v / robust_vpp_v / robust_rms_v
```

需要工程：`02_adc_dma`、`50_robust_measurement`。注意：Median 与 Hampel 不是默认串联关系；确认要选的一条链。

## 方案 7：FFT 粗测 + 四参数正弦精密拟合

目标：高精度频率、幅值、相位、DC。

```text
AcquireADCFrame()
  ↓
PrepareSignal()（20） → RunFFTCommon() → MeasureFFTFrequency() → RefineFFTFrequency()
  ↓ frequency_hz
initial_frequency_hz = frequency_hz
  ↓
RunSineFit3Param() / RunSineFit4Param()（60）
```

需要工程：`20_fft_analysis`、`60_precision_measurement`。注意：复用同一电压帧；不要为 Fit 再采一帧，除非题目明确允许不同时间点。

## 方案 8：已知频率弱信号 Lock-In

目标：已知激励频率下检测弱幅值/相位。

```text
DDS 或题目已知 reference_frequency_hz
  ↓
AcquireADCFrame() → ConvertADCToVoltage()
  ↓ voltage_samples[]
RunLockIn()
  ↓ amplitude_v / phase_deg
```

需要工程：`61_lock_in`；若内部激励，另需 `90_dds_usage`。注意：Lock-In 的参考频率必须和激励或被测目标一致。

## 方案 9：DDS + 键盘 + TFT

目标：按键调整 `frequency_hz`，更新 DAC DMA 正弦并显示数值。

```text
InitDDSOutput()（一次） + InitTFTDemo()（一次）
  ↓
ReadKeypad() → HandleDDSFrequencyAdjust()
  ↓ frequency_hz
SetDDSFrequency()
  ↓ DAC DMA waveform
UpdateLiveValue()
```

需要工程：`70_keypad_usage`、`80_tft_usage`、`90_dds_usage`。注意：不要把 70 的 `adjustable_value` 与 90 的 `frequency_hz` 混为一个没有单位说明的变量。
