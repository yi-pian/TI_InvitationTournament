# 50_robust_measurement

## 推荐复制函数

`AcquireADCFrame() + ConvertADCToVoltage() + ApplySelectedFilter() + AnalyzeRobustStatistics()`。可按需要复制 `ApplyMedianFilter()` 或 `ApplyHampelFilter()`；最终输出 `mad_v`、`robust_vpp_v`、`robust_rms_v`、`outlier_count`。

## 1. 这个工程干什么

对 ADC 电压帧演示中值滤波、Hampel 离群点替换、MAD、鲁棒 Vpp 和鲁棒 RMS。

## 2. 这个工程包含哪些子功能

| 我要做什么 | COPY 区域 |
|---|---|
| 中值滤波 | `MEDIAN_FILTER` |
| Hampel 去异常点 | `HAMPEL_FILTER` |
| MAD | `MAD` |
| 鲁棒 Vpp | `ROBUST_VPP` |
| 鲁棒 RMS | `ROBUST_RMS` |

## 3. 输入

`adc_samples` 是 ADC code；`voltage_samples` 和 `filtered_samples` 是 V。

## 4. 输出

`filtered_samples`、`outlier_count`、`mad_v`、`robust_vpp_v`、`robust_rms_v`。

## 5. 公共数据链

`ADC → voltage_samples → (median/Hampel) → robust statistics`。

## 6. 功能与 COPY 区对应表

各区可独立选用；`MAD`/Vpp/RMS 的输入可选原始或滤波后数组，示例采用 `filtered_samples`。

## 7. 使用的模块

`signal_median_filter`、`signal_hampel`、`signal_mad`、`signal_robust_peak_to_peak`、`signal_robust_rms`；调用依据是 restored example04 `App_RobustMeasurement` 和真实头文件。

## 8. SysConfig / 引脚

复制 restored example04 ADC/DMA 配置。

## 9. main.c 流程

换算电压，先演示滤波，再测 MAD、鲁棒 Vpp/RMS。

## 10. 每个 COPY 区说明

所有模块共用 `workspace`，按顺序执行，避免额外 N 点永久数组。

## 11. 如何复制到新工程

复制需要的模块及其依赖、相应 COPY 区；若仅求鲁棒 Vpp/RMS，不必复制两种滤波区。

## 12. 可调参数

中值窗宽、Hampel sigma、MAD 最小尺度、Vpp/RMS 分位数。

## 13. 常见错误

尖峰本身是被测目标时不要滤波；窗口必须为奇数；workspace 容量必须符合模块要求。

## 14. 本工程没有做什么

不做 FFT 和实时显示。

## 15. Build 状态

待统一 SysConfig 生成和 CCS Compile/Link 审计；实板 `NOT_RUN`。
