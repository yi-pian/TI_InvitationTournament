# 函数依赖与单帧唯一索引

“单帧唯一”表示同一个 ADC/DMA frame、相同数据格式和参数下，该节点原则上只应执行一次。教学函数本身不带缓存；合并工程时需由应用层人工保证。

| 函数 / 数据节点 | 输入 | 输出 | 类型 | 直接前置 | 单帧唯一 | 合并提示 |
|---|---|---|---|---|---|---|
| `ReadADCOnce()` | 模拟输入 | `adc_samples[0]` code | 单点采集 | SysConfig | 不适用 | 不与 DMA 帧混用。 |
| `AcquireADCFrame()`（02） | ADC/DMA | `adc_samples[]`、Fs | 数据产生 | `InitADC()` | YES | 作为单通道总入口。 |
| `AcquireDualADCFrame()`（04） | 双 ADC/DMA | 两路 code、Fs | 数据产生 | `InitDualADC()` | YES | Phase/双通道总入口。 |
| `AcquireADCFrame()`（11/20/21/30/50/60/61） | 双 ADC 驱动 | 第一通道 code、Fs | 数据产生 | Dual ADC init | YES | 合并时只保留一个实际采集函数。 |
| `ConvertADCToVoltage()` | `adc_samples[]` code | `voltage_samples[]` V | 格式转换 | 有效 ADC 帧 | YES | Basic/Robust/Fit/Lock-In 共用。 |
| `PrepareSignal()`（11） | `adc_samples[]` code | `voltage_samples[]`、`mean_v`、`centered_samples[]` | 转换+去 DC | 有效 ADC 帧 | YES | 本函数已含转换；不要再先复制同帧转换函数。 |
| `PrepareSignal()`（20） | `adc_samples[]` code | `voltage_samples[]`、`mean_v`、`centered_samples[]` | 转换+去 DC | 有效 ADC 帧 | YES | FFT 数据链入口；可供 Basic 复用 `voltage_samples[]`。 |
| `PrepareDisplaySamples()` | `adc_samples[]` code | `voltage_samples[]` V | 格式转换 | 有效 ADC 帧 | YES（若需 V） | 时域图本身仍按 ADC code 映射。 |
| `MeasureBasicParameters()` | `voltage_samples[]` V | DC/min/max/Vpp/RMS/AC RMS/std/clipping | 帧统计 | 电压数组 | 建议 YES | 与 FFT 可共用电压，但函数会自行再次算 mean。 |
| `MeasureFrequencyZeroCross()` | `centered_samples[]` V、Fs | `frequency_hz` | 测量 | `PrepareSignal()` | YES | 和 FFT 可共享去 DC 数据，但阈值/迟滞配置须相同。 |
| `RunFFTCommon()` | `centered_samples[]` V | `fft_magnitude[]` | FFT 公共链 | FFT `PrepareSignal()` | **YES** | 一帧只能一个相同 N/window/Fs 的 FFT。 |
| `RunQ15FFT()` | 加窗 float 帧 | `fft_magnitude[]` | FFT helper | `RunFFTCommon()` | **YES** | 不单独在 main 调用。 |
| `MeasureFFTFrequency()` | `fft_magnitude[]`、Fs | `peak_bin`、`frequency_hz` | 谱峰分析 | `RunFFTCommon()` | 建议 YES | 给插值、THD/SNR/SFDR、Fit 共用。 |
| `RefineFFTFrequency()` | magnitude、`peak_bin` | `interpolated_bin`、精修频率 | 谱峰分析 | `MeasureFFTFrequency()` | 建议 YES | 不重跑 FFT。 |
| `AnalyzeHarmonicsAndTHD()` | magnitude、精修频率、Fs | harmonics、THD | 频谱分析 | FFT + peak | 建议 YES | 仅复用 magnitude。 |
| `AnalyzeSNRAndSFDR()` | magnitude、`peak_bin` | dB 指标 | 频谱分析 | FFT + peak | 建议 YES | 仅复用 magnitude。 |
| `DrawFFTSpectrum()` | magnitude、peak | TFT 图形 | 显示 | FFT + TFT init | 否 | 不得在显示中加 FFT。 |
| `DrawTimeDomainWaveform()` | `adc_samples[]` | TFT 图形 | 显示 | ADC 帧 + TFT init | 否 | 可与 FFT 共用 ADC 帧。 |
| `MeasurePhase()` | 同步双 ADC code、Fs | `phase_deg` | 测量 | `AcquireDualADCFrame()` | YES | 两路不可分别采集。 |
| `CalculateDelayFromPhase()` | deg、参考频率 | `delay_s` | 标量换算 | `MeasurePhase()` | 建议 YES | 参考频率不可为 0。 |
| `ApplySelectedFilter()` | `voltage_samples[]` V | `filtered_samples[]`、outlier count | 预处理 | 电压数组 | YES | RAW/Median/Hampel 三选一。 |
| `AnalyzeRobustStatistics()` | `filtered_samples[]` V | MAD/鲁棒 Vpp/RMS | 统计 | 选定滤波链 | 建议 YES | 所有鲁棒指标共用同一滤波输出。 |
| `RunSineFit3Param()` | 电压、Fs、初频 | 幅值/相位/DC | 精密拟合 | 电压转换 | 建议 YES | 频率为已知值。 |
| `RunSineFit4Param()` | 电压、Fs、初频 | 精修频率 | 精密拟合 | 电压转换 | 建议 YES | 初频优先复用 FFT 结果。 |
| `RunLockIn()` | 电压、Fs、参考频率 | 幅值/相位/IQ | 同步检测 | 电压转换 | 建议 YES | 参考频率由外部提供。 |
| `ReadKeypad()` / `Handle*()` | 键盘事件 | UI 状态 | UI | SysConfig | 不适用 | 与信号链独立。 |
| `InitTFTDemo()` / `Draw*()` | TFT、结果变量 | 屏幕 | UI | SysConfig | 不适用 | 不生成测量数据。 |
| `InitDDSOutput()` / `SetDDSFrequency()` | 波表、Fs、Hz | DAC DMA 波形 | 输出 | DAC SysConfig | 按设置 | 与输入 ADC 帧独立。 |
| `SetDACDC()` | DAC code | DC 输出 | 输出 | DAC SysConfig | 按设置 | 不能替代 DDS 连续波。 |

## 人工合并判定

同一帧组合 Basic + FFT 时，建议选择一个产生 `voltage_samples[]` 的函数：

```text
AcquireADCFrame()
        ↓
PrepareSignal()（20：ADC code → voltage_samples + mean_v + centered_samples）
        ├── MeasureBasicParameters()（直接读 voltage_samples）
        └── RunFFTCommon()（读 centered_samples）
                 ├── MeasureFFTFrequency()
                 ├── RefineFFTFrequency()
                 ├── AnalyzeHarmonicsAndTHD()
                 └── AnalyzeSNRAndSFDR()
```

不要同时运行 `ConvertADCToVoltage()` 与已包含转换的 `PrepareSignal()`；也不要在每个频谱后处理函数内再次调用 `RunFFTCommon()`。

## 何时允许再次执行 FFT

仅当以下任一项不同，才是另一条合法 FFT 数据链：不同通道、不同 frame、不同 N、不同 Fs、不同窗口或不同处理频带/配置。此时应在工程注释中写明原因，并为该链单独管理工作区与结果。
