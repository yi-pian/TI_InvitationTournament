# 完整函数复制顺序指南

本指南中的“复制”指复制完整 `static` 函数、该函数头注释、所需全局变量/宏、所列模块 `.c/.h` 和对应 SysConfig 资源；不是只复制 while 循环中的几行算法语句。

## FFT 测频

按顺序复制：

1. `InitADC()` + `AcquireADCFrame()`（02），或接入已有同格式 `adc_samples[]` 和真实 `sample_rate_hz`；
2. `PrepareSignal()`（20）；
3. `RunQ15FFT()` 与 FFT 工作数组；
4. `RunFFTCommon()`；
5. `MeasureFFTFrequency()`；
6. 需要精修时再复制 `RefineFFTFrequency()`。

不要复制整个 20 工程；只复制以上函数和 20 README 声明的 FFT/Window 模块。`RunFFTCommon()` 只能每帧一次。

## Basic 测量

按顺序复制：

1. 一个实际采集入口；
2. `ConvertADCToVoltage()`；
3. `CalculatePopulationStdDev()`；
4. `MeasureBasicParameters()`。

如果同一帧已由 20 的 `PrepareSignal()` 产生 `voltage_samples[]`，跳过第 2 步；Basic 直接读取该电压数组。

## 过零插值测频

按顺序复制：

1. 一个实际采集入口；
2. `PrepareSignal()`（11）；
3. `MeasureFrequencyZeroCross()`；
4. `signal_zero_cross`、`signal_zero_cross_interpolation` 与 CMSIS-DSP。

新项目已有 ADC DMA 时只接入 `adc_samples[]`、点数、真实 Fs；不要再保留第二个采样 while 循环。

## 双通道相位

按顺序复制：

1. `InitDualADC()`；
2. `AcquireDualADCFrame()`；
3. `MeasurePhase()`；
4. `CalculateDelayFromPhase()`（需要 `reference_frequency_hz`）。

必须复制双 ADC 公共触发相关 SysConfig；不能改成单 ADC 函数两次调用。

## 鲁棒 Vpp/RMS

按顺序复制：

1. 一个实际采集入口；
2. `ConvertADCToVoltage()`；
3. `ApplyMedianFilter()` 或 `ApplyHampelFilter()`，或保留 `ApplySelectedFilter()`；
4. `AnalyzeRobustStatistics()`；
5. `workspace[]`、`filtered_samples[]` 及对应 Robust 模块。

选择 RAW、Median、Hampel 其中一条最终链。不要误以为三个模式需要并行执行。

## 精密正弦拟合

按顺序复制：

1. 采集函数；
2. `ConvertADCToVoltage()`；
3. `RunSineFit3Param()`；
4. 需要自由频率搜索时复制 `RunSineFit4Param()`。

若初频来自 FFT，先完成 FFT 测频链，再把 `frequency_hz` 赋给 `initial_frequency_hz`。

## Lock-In

按顺序复制：

1. 采集函数；
2. `ConvertADCToVoltage()`；
3. 设置 `reference_frequency_hz`；
4. `RunLockIn()`。

若已由 Basic/Fit 做过电压转换，复用现有 `voltage_samples[]`，不要第二次转换。

## DDS + 键盘 + TFT

初始化阶段复制：

1. `InitDDSOutput()`；
2. `InitTFTDemo()`；
3. `DrawStaticText()`。

循环阶段复制：

1. `ReadKeypad()`；
2. 按题目调用 `HandleDDSFrequencyAdjust()` 或自己的参数校验；
3. `SetDDSFrequency()`；
4. `UpdateLiveValue()`。

## 如何人工合并多个功能

错误做法：分别复制 02、30、20 的完整主循环。这样可能同一时刻采多帧、转换多次、FFT 多次，结果还来自不同帧。

正确做法：先选唯一采集入口，再按数据节点分支：

```text
一次 AcquireADCFrame()
       ↓
一次 ADC code → voltage_samples[]
       ├── Basic / Robust / Sine Fit / Lock-In
       └── 一次去 DC → 一次 RunFFTCommon()
                         ├── FFT Frequency
                         ├── Interpolation
                         ├── THD
                         ├── SNR/SFDR
                         └── Spectrum Draw
```

同一数据链节点只保留一次。若必须因不同通道、不同 N、不同 Fs 或不同窗口重算，应在 main.c 注释中写明它是另一条独立数据链。
