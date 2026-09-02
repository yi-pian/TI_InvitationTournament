# 单功能示例统一输入/输出接口标准

本文件只约束每个示例 `main.c` 的教学代码和 COPY 区，不修改任何已有模块 `.c/.h` 或其 API。

| 含义 | 统一名称 | 数据类型 / 单位 |
|---|---|---|
| 单通道 ADC 原始帧 | `adc_samples` | `uint16_t[]`，ADC code |
| 双通道原始帧 | `adc_ch1_samples` / `adc_ch2_samples` | `uint16_t[]`，ADC code |
| 每帧点数 | `SAMPLE_COUNT` | 宏，元素数 |
| 实际采样率 | `sample_rate_hz` | `float`，Hz |
| ADC 换算电压 | `voltage_samples` | `float[]`，V |
| 去 DC 样本 | `centered_samples` | `float[]`，V |
| FFT 单边幅度 | `fft_magnitude` | `float[]`，幅度单位必须在注释说明 |
| DMA 帧有效标志 | `adc_frame_ready` | `bool` |
| 频率 | `frequency_hz` | `float`，Hz |
| 统计结果 | `mean_v`、`minimum_v`、`maximum_v`、`vpp_v`、`rms_v`、`ac_rms_v` | `float`，V |
| 相位/延迟 | `phase_deg`、`delay_s` | `float`，deg / s |
| 占空比 | `duty_cycle_percent` | `float`，% |
| FFT 峰值 | `peak_bin`、`interpolated_bin` | `uint32_t` / `float` |
| UI 页 | `current_page` | `uint8_t` |

每个 COPY START 前必须写 `[INPUT]`、`[OUTPUT]`，并指出数组是 ADC code、物理电压、去 DC 电压还是 FFT 幅度。若模块参数名不同，main 仍使用上述名称传入模块；禁止为了统一命名改动模块源码。

## 统一教学函数命名

同一含义优先使用同一名称，但不强制每个主题拥有相同函数集合：

- 采集：`AcquireADCFrame()`、`AcquireDualADCFrame()`；初始化：`InitADC()`、`InitDualADC()`。
- 电压/预处理：`ConvertADCToVoltage()`、`PrepareSignal()`、`PrepareDisplaySamples()`。
- 测量：`MeasureBasicParameters()`、`MeasureFrequencyZeroCross()`、`MeasureFFTFrequency()`、`MeasurePhase()`、`MeasureTimerFrequency()`。
- 分析：`RunFFTCommon()`、`RefineFFTFrequency()`、`AnalyzeHarmonicsAndTHD()`、`AnalyzeSNRAndSFDR()`、`RunSineFit3Param()`、`RunSineFit4Param()`、`RunLockIn()`。
- 显示/UI：`DrawTimeDomainWaveform()`、`UpdateLiveValue()`、`ReadKeypad()`、`HandlePageSwitch()`、`SetDDSFrequency()`。

COPY 的单位是完整 `static` 函数及它明确列出的前置函数、全局变量与模块，不再是从 `while` 循环中抽取零散语句。
