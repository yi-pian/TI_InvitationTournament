# 综合 Example ↔ fuyong 教学函数映射

本表用于“从教学函数回到综合题”定位，不表示把教学代码逐字覆盖综合工程。综合题中的校准、动态采样率、扫频、捕获回放、外设协议和页面状态机必须保留。

| 综合工程 | 可对应的 fuyong 主题 | 对应教学函数边界 | 必须保留的专属逻辑 |
|---|---|---|---|
| example01 | 04、40、70、80 | `AcquireDualADCFrame()`、`MeasurePhase()`、键盘/TFT函数 | 双 trace、按键队列、相位页面。 |
| example02 | 04、40、70、80、90 | 双 ADC、相位、DDS、键盘/TFT函数 | 扫频、增益/相位曲线、点位状态。 |
| example03 | 04、21、70、80 | 双 ADC、`DrawTimeDomainWaveform()`、键盘/TFT函数 | 双通道网格、量程、旧线擦除恢复。 |
| example04 | 04、11、20、30、50、60、61、70、80、90 | `App_AcquireFrame()` ↔ 采集；`App_BasicMeasurements()` ↔ Basic；`App_Spectrum()` ↔ FFT；其余见下表 | 两点校准、动态窗口/滤波、Nyquist 保护、7 页 UI、捕获回放、四种 DDS 波形。 |
| example05 | 04、40、80 | 双 ADC、相位、TFT函数 | AD9833、复阻抗、R/L 拟合、扫频。 |
| example06 | 02、11、30、70、80 | 单 ADC、过零、Basic、键盘/TFT函数 | I/Q 重构、显示窗口、题目周期选择。 |
| example07 | 04、61、70、80、90 | 双 ADC、Lock-In、DDS、键盘/TFT函数 | 预补偿、未知网络扫频、谐波合成。 |
| example08 | 04、40、70、80 | 双 ADC、相位、键盘/TFT函数 | GPAMP/OPA/COMP 前端、量程和边沿计数。 |
| 24_A_rebuild | 02、30、50、70、80 | ADC、Basic、Robust、键盘/TFT函数 | VCA820、UGBW、Slew Rate、静态功耗四问。 |
| 24_C_rebuild | 04、20、21、80 | 双 ADC、FFT、时域图、TFT函数 | 动态采样、Burst gate、Hann 幅度修正、猝发状态机。 |
| 22_X | 04、40、70 | 双 ADC、相位、键盘函数 | PLL/YV 控制、李萨如映射、ILI9341 显示。 |

## example04 重点对应

| example04 函数 | 对应 fuyong 函数 | 说明 |
|---|---|---|
| `App_AcquireFrame()` | `AcquireDualADCFrame()` | 保留 example04 的动态采样率和 capture 缓冲。 |
| `App_BasicMeasurements()` | `ConvertADCToVoltage()` + `MeasureBasicParameters()` | 输入仍为两点校准后的 `g_calibrated`。 |
| `App_TimeFrequency()` | `PrepareSignal()` + `MeasureFrequencyZeroCross()` / Timer Capture | 保留原页面的多源频率语义。 |
| `App_Spectrum()` | `RunFFTCommon()` + Peak/Interpolation/THD/SNR/SFDR | 保留动态 Window、校准输入和 H3 Nyquist 保护；每帧 FFT 一次。 |
| `App_RobustMeasurement()` | `ApplySelectedFilter()` + `AnalyzeRobustStatistics()` | 保留 RAW/MEDIAN/HAMPEL 三选一。 |
| `App_SineFitAndLockIn()` | `RunSineFit3Param()`、`RunSineFit4Param()`、`RunLockIn()` | 初频来自本帧 FFT，Lock-In 参考来自 DDS。 |
| `App_ProcessKey()` / UI 绘制 | `ReadKeypad()` / `UpdateLiveValue()` | 不覆盖 SysTick 队列和七页布局。 |
| `App_ApplyWaveform()` | `SetDDSFrequency()` 的职责边界 | 保留四波形、幅值、偏置与回放速率恢复。 |

更完整的工程级审计请同时查阅 `fuxian/backup/BACKUP_REUSE_MAP.md` 和 `backup/example04/.../EXAMPLE04_REUSE_MAP.md`。
