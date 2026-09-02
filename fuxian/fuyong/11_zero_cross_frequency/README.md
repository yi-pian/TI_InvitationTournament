# 11_zero_cross_frequency

## 推荐复制函数

`PrepareSignal() + MeasureFrequencyZeroCross()`（COPY 区 `ZERO_CROSS_PREPARE`、`ZERO_CROSS_MEASURE`）。若新工程尚无 ADC DMA，再同时复制 `AcquireADCFrame()`；最终读取 `frequency_hz`（Hz）。

## 1. 这个工程干什么

从 ADC DMA 原始码直接完成去 DC、上升沿过零、线性插值和多周期频率计算。

## 2. 这个工程包含哪些子功能

| 我要做什么 | COPY 区域 |
|---|---|
| 准备 ADC 信号 | `PrepareSignal()` / `ZERO_CROSS_PREPARE` |
| 插值过零测频 | `PrepareSignal() + MeasureFrequencyZeroCross()` / `ZERO_CROSS_PREPARE + ZERO_CROSS_MEASURE` |

## 3. 输入

`adc_samples` 是 `uint16_t` ADC code；`SAMPLE_COUNT` 为帧点数；`sample_rate_hz` 是实际采样率。

## 4. 输出

`voltage_samples` 为 V，`centered_samples` 为去 DC V，`frequency_hz` 为 Hz。

## 5. 公共数据链

`AcquireADCFrame() → PrepareSignal() → MeasureFrequencyZeroCross()`。

其中 `PrepareSignal()` 负责 `ADC DMA → voltage_samples → mean_v/DC → centered_samples`；
`MeasureFrequencyZeroCross()` 负责 `centered_samples → zero_events → interpolation → frequency_hz`。

## 6. 功能与 COPY 区对应表

需要测频时完整复制 `PrepareSignal()` 和 `MeasureFrequencyZeroCross()`；后者依赖前者输出的 `centered_samples[]` 与 `mean_v` 语义，不再要求从 while 循环中挑取零散语句。

## 7. 使用的模块

`signal_dual_adc_mspm0g3507`、`signal_zero_cross`、`signal_zero_cross_interpolation`、CMSIS-DSP。依据：已恢复 example04 的 `App_TimeFrequency` 和各真实头文件。

## 8. SysConfig / 引脚

复制 restored example04 的完整验证配置；ADC0/ADC1 与同步 DMA 保持原样，本工程只分析 `adc_samples`。

## 9. main.c 流程

采一帧、转换电压、去 DC、找上升沿、插值并用首尾 crossing 跨度做多周期平均。

## 10. 每个 COPY 区说明

`ZERO_CROSS_COMMON` 输出两条后续共用数组；第二段只生成事件；第三段才输出频率。

## 11. 如何复制到新工程

复制 `AcquireADCFrame()`（或接入已有 ADC DMA）、`PrepareSignal()`、`MeasureFrequencyZeroCross()`，再复制四个模块及其依赖、相同 ADC SysConfig；若已有单 ADC DMA，可把 `adc_samples` 直接接入 `PrepareSignal()`。

## 12. 可调参数

`SIGNAL_SAMPLE_COUNT`、`SIGNAL_SAMPLE_RATE_HZ`、5 mV hysteresis；阈值应与去 DC 后数据一致。

## 13. 常见错误

没有两个以上同方向 crossing、信号削顶或采样率不正确都会使频率无效。

## 14. 本工程没有做什么

不做 FFT、THD、TFT 或硬件 Timer Capture。

## 15. Build 状态

待统一 SysConfig 生成和 CCS Compile/Link 审计；实板为 `NOT_RUN`。
