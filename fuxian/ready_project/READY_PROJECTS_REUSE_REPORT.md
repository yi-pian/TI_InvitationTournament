# READY_PROJECTS REUSE REPORT

审计日期：2026-08-23  
状态：`IMPLEMENTED / BUILD_PASS / BOARD_NOT_RUN`

## 复用原则

实现前已核对 `fuxian/fuyong/INDEX.md`、`COPY_BLOCK_INDEX.md`、`offline_index/*`、相关主题 README 与实际 `main.c`。八个工程均沿用以下数据流：

```text
一次采集
  -> 每通道一次 ADC code 转电压
  -> 每通道一次去 DC
  -> 每个需要 FFT 的信号每帧一次 FFT
  -> 频率 / 谐波 / THD / SNR / SFDR / 绘图复用既有结果
```

函数头使用三类来源标记：

- `[FUYONG_COPY]`：教学流程或调用原样复制；
- `[FUYONG_ADAPTED]`：公式与模块调用不变，只做参数化、双通道适配或统一变量名；
- `[READY_PROJECT_LOCAL]`：仅补页面、状态机、显示映射、参数限幅等应用层逻辑。

## 工程级复用清单

| 工程 | 主要 fuyong / example 来源 | `FUYONG_COPY` | `FUYONG_ADAPTED` | `READY_PROJECT_LOCAL` |
|---|---|---|---|---|
| 01_dual_waveform_scope | 04_dual_adc_dma、11_zero_cross_frequency、21_time_domain_waveform、21_waveform_display、24_auto_range、70、80；example03 | `AcquireDualADCFrame` | `ConvertADCToVoltage`、`PrepareSignal`、`MeasureFrequencyZeroCross`、`DrawTimeDomainWaveform` | `VisibleSampleCount`、CH1/CH2/DUAL/XY 页面、双路自动量程、周期数按键、XY 绘制 |
| 02_dual_spectrum_thd | 04_dual_adc_dma、20_fft_analysis、22_spectrum_display、70、80；example04 | `AcquireDualADCFrame` | `PrepareSignal`、`RunFFTCommon`、`AnalyzeChannelSpectrum`（频率/插值/谐波/THD/SNR/SFDR） | 频率轴、dB Y 轴、显示范围、峰值/谐波标记、CH1/CH2/SUMMARY 页面 |
| 03_programmable_signal_generator | 90_dds_usage、91_dac_usage、70、80；example04 | Keypad 扫描 ISR | `ApplyWaveform`、`HandleKeypad`、`UpdateDisplay`、`App_Init` | 数字输入、参数限幅、波形相关参数语义、空输入步进、单页 `current_page` |
| 04_dual_measurement_meter | 04_dual_adc_dma、11_zero_cross_frequency、30_basic_measurement、40_dual_channel_measurement、70、80；example01 | `AcquireDualADCFrame` | `ConvertADCToVoltage`、`MeasureBasicParameters`、`MeasureFrequencyZeroCross`、相位/延时测量与显示 | `CalculateGain` 逻辑、BASIC/DUAL 页面与按键切页 |
| 05_bode_sweep_analyzer | 90_dds_usage、04_dual_adc_dma、30、40、70、80；example02 | `GenerateSweepTable`、`AcquireDualADCFrame` | `MeasureVpp`、`RunSweepPoint`、`UpdateDisplay` | 扫频状态机、参数限幅/调整、START/STOP/RESTART、Gain/Phase/Current 曲线页面 |
| 06_trigger_burst_capture | 04_dual_adc_dma、23_trigger_capture、21_time_domain_waveform、30、70、80；example04 | — | `AcquireADCFrame`、`Trigger_Capture`、`PrepareCaptureResult`、捕获显示 | ARM/WAITING/TRIGGERED/HOLD、触发沿/电平/预触发设置、重触发与单页状态 |
| 07_digital_filter_lab | 04_dual_adc_dma、15_filter_processing、20_fft_analysis、21、22、50、70、80；example04 | — | `AcquireADCFrame`、`ConvertADCToVoltage`、`ApplyMovingAverage`、`ApplySelectedFilter`、`RunFFTCommon` | 模式/窗口/阈值按键、RAW/FILTERED 同尺度波形、同尺度频谱页面 |
| 08_precision_single_tone_meter | 04_dual_adc_dma、11、20、30、60_precision_measurement、70、80；example04 | `RefineFFTFrequency` 教学调用 | `AcquireADCFrame`、`PrepareSignalAndBasic`、`RunFFTCommon`、`MeasureFrequencyZeroCross`、`AnalyzeQuality`、`RunSineFit4Param` | FAST/NORMAL/PRECISION 调度、三种频率分离、最终推荐值选择、单页状态 |

`DrawText`、TFT 初始化和 4×4 键盘扫描等短适配函数也在各 `main.c` 内标明来源；上表列的是决定仪器功能和数据复用关系的关键函数。

本轮进一步统一了八个工程的交互实现：按键入口采用 `moni01/main.c` 的 8 项单生产者/单消费者环形队列；TFT 采用 `moni01` 的“静态 UI 与动态区域分离”方式。固定标题、标签和外框只在首次显示或翻页时绘制，普通刷新只更新波形/频谱/曲线内部与数值矩形。

## 接口与复用审计

| 检查项 | 结果 |
|---|---|
| 单通道采样数组使用 `adc_samples` | PASS |
| 双通道采样数组使用 `adc_ch1_samples/adc_ch2_samples` | PASS |
| 电压/去 DC/FFT/频率/幅值变量按统一物理含义命名 | PASS |
| 可共用算法采用输入指针、输出指针、点数和结果指针参数化 | PASS |
| 同一帧不重复 ADC code→voltage | PASS |
| 同一帧同一通道不重复去 DC | PASS |
| 02 的 CH1/CH2 各 FFT 一次，THD/SNR/绘图复用幅度谱 | PASS |
| 07 的 RAW/FILTERED 来自同一 ADC frame，各自 FFT 一次 | PASS |
| 08 每帧 FFT 一次，插值/THD/SNR/Sine Fit 复用前级结果 | PASS |
| 页面状态只描述用户正在看的页面，不与采集/精度/波形状态共用变量 | PASS |
| 周期刷新没有整屏 FillScreen，只局部更新动态区域 | PASS |
| 1 ms SysTick / 5 ms 键盘扫描 / 8 项事件队列与 moni01 一致 | PASS |
| 所有大数组为文件作用域 `static` | PASS |

## 各工程烧录后界面

- 01：CH1、CH2、DUAL、XY；显示采样率、时间跨度、两路 V/div 和周期数；
- 02：CH1 频谱、CH2 频谱、数值汇总；显示频率、H2/H3/H4、THD、SNR、SFDR；
- 03：SINE/TRIANGLE/SAWTOOTH/SQUARE；显示 Frequency、Vpp、Offset、Duty、Symmetry，无意义参数显示 `--`；
- 04：BASIC 页显示两路 F/Vpp/RMS/DC，DUAL 页显示 Gain/Gain(dB)/Phase/Delay；
- 05：GAIN、PHASE、CURRENT；保存每点 frequency/gain/phase，支持 START/STOP/RESTART；
- 06：显示捕获波形、ARM/WAITING/TRIGGERED/HOLD、Level、Slope、Duration、Start、End、Vpp；
- 07：WAVEFORM 显示 RAW/FILTERED，SPECTRUM 显示两者频谱，支持 NONE/MA/MEDIAN/HAMPEL；
- 08：显示推荐频率、FFT/ZeroCross/SineFit 三种频率、Amplitude/Vpp/RMS/DC/THD/SNR，并切换 FAST/NORMAL/PRECISION。

## 模块与 SysConfig 来源

每个目标工程的模块代码与下列已验证示例逐文件同哈希，`.syscfg` 也同哈希：

- 01 ← `fuxian/example03/signal_contest_template_final`；
- 02、03、06、07、08 ← `fuxian/example04/signal_contest_template_final`；
- 04 ← `fuxian/example01/signal_contest_template_final`；
- 05 ← `fuxian/example02/signal_contest_template_final`。

未修改 `fuyong`、`example` 或任何既有模块源文件。新增能力全部留在当前工程 `main.c` 的 `static` 函数、状态变量与宏中。
