# 61_lock_in

## 推荐复制函数

`AcquireADCFrame() + ConvertADCToVoltage() + RunLockIn()`（COPY 区 `LOCK_IN_CONVERT`、`LOCK_IN`）。输入 `reference_frequency_hz` 和 `sample_rate_hz`，输出 `amplitude_v`、`phase_deg`。

## 1. 这个工程干什么

对已知参考频率的 ADC 电压帧做 I/Q 同步检测，输出幅度和相位。

## 2. 这个工程包含哪些子功能

| 我要做什么 | COPY 区域 |
|---|---|
| Lock-In 幅相测量 | `LOCK_IN` |

## 3. 输入

`voltage_samples`（V）、`reference_frequency_hz`、`sample_rate_hz`。

## 4. 输出

`amplitude_v`、`phase_deg` 和 `lock_in_result`。

## 5. 公共数据链

`ADC → voltage_samples → I/Q reference multiplication → result`。

## 6. 功能与 COPY 区对应表

本主题只有 `LOCK_IN`。

## 7. 使用的模块

`signal_lock_in`、`signal_dual_adc_mspm0g3507`；依据 restored example04 `App_SineFitAndLockIn` 与真实头文件。

## 8. SysConfig / 引脚

复制 restored example04 ADC/DMA 配置。

## 9. main.c 流程

采集、换算电压、填入已知参考频率并调用 Lock-In。

## 10. 每个 COPY 区说明

reference 由题目、DDS 设置或已确认的测频结果给出；模块不自动猜频。

## 11. 如何复制到新工程

复制 lock-in 模块和 `LOCK_IN` 区；保留 `voltage_samples` 单位为 V。

## 12. 可调参数

参考频率、参考初相、remove DC 标志。

## 13. 常见错误

参考频率错会严重衰减结果；不要把 ADC code 当电压。

## 14. 本工程没有做什么

不生成参考波，不重新做 FFT。

## 15. Build 状态

待统一 SysConfig 生成和 CCS Compile/Link 审计；实板 `NOT_RUN`。
