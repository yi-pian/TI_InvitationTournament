# 40_dual_channel_measurement

## 推荐复制函数

`AcquireDualADCFrame() + MeasurePhase() + CalculateDelayFromPhase()`（COPY 区 `PHASE_MEASURE`、`PHASE_DELAY`）。输出 `phase_deg`（deg）和由已知 `reference_frequency_hz` 推导的 `delay_s`（s）。

## 1. 这个工程干什么

用同步双 ADC 原始码计算两路信号的相位与时间延迟。

## 2. 这个工程包含哪些子功能

| 我要做什么 | COPY 区域 |
|---|---|
| 双通道相位/延迟 | `PHASE` |

## 3. 输入

`adc_ch1_samples`、`adc_ch2_samples` 为 `uint16_t` ADC code；`SAMPLE_COUNT`、`sample_rate_hz` 为公共参数。

## 4. 输出

`phase_deg`（°）、`delay_s`（s）和 `phase_result`。

## 5. 公共数据链

`同步 ADC DMA → dual_adc_phase → phase/delay`。

## 6. 功能与 COPY 区对应表

相位与延迟使用同一个 `PHASE` 区。

## 7. 使用的模块

`signal_dual_adc_mspm0g3507`、`signal_dual_adc_phase`；依据是真实头文件与 restored example04 模块清单。

## 8. SysConfig / 引脚

复制 restored example04 双 ADC 同步触发配置，不得将两路改为独立软件触发。

## 9. main.c 流程

采集同一帧两路 ADC code，然后调用相位模块。

## 10. 每个 COPY 区说明

模块配置中的 hysteresis、最小幅度和 frequency ratio 都来自真实结构体。

## 11. 如何复制到新工程

复制模块、`PHASE` 和完整双 ADC SysConfig；保留统一数组命名。

## 12. 可调参数

`hysteresis_code`、`min_amplitude_code`、`frequency_ratio`。

## 13. 常见错误

不同步采样、信号幅度太小或频率比不等于配置会使结果无效。

## 14. 本工程没有做什么

不做 Lissajous、FFT 或增益标定。

## 15. Build 状态

待统一 SysConfig 生成和 CCS Compile/Link 审计；实板 `NOT_RUN`。
