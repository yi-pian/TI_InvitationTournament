# 30_basic_measurement

## 推荐复制函数

`AcquireADCFrame() + ConvertADCToVoltage() + MeasureBasicParameters()`（COPY 区 `BASIC_CONVERT`、`BASIC_MEASUREMENT`）。最终读取 `mean_v`、`minimum_v`、`maximum_v`、`vpp_v`、`rms_v`、`ac_rms_v`、`population_stddev_v` 与 `clipping`。

## 1. 这个工程干什么

将一帧 `adc_samples` 换算为电压并集中计算基础波形统计量。

## 2. 这个工程包含哪些子功能

| 我要做什么 | COPY 区域 |
|---|---|
| DC、Vpp、RMS、AC RMS、标准差、削顶 | `BASIC_MEASUREMENT` |

## 3. 输入

`adc_samples` 是 `uint16_t` ADC 原始码，`SAMPLE_COUNT` 是样本数。

## 4. 输出

`mean_v`、`minimum_v`、`maximum_v`、`vpp_v`、`rms_v`、`ac_rms_v`、`population_stddev_v`、`clipping`。

## 5. 公共数据链

`ADC code → voltage_samples → CMSIS statistics / centered_samples → AC RMS`。

## 6. 功能与 COPY 区对应表

本主题只有一个完整的 `BASIC_MEASUREMENT` COPY 区。

## 7. 使用的模块

`signal_dual_adc_mspm0g3507` 和 CMSIS-DSP；统计调用依据为 restored example04 `App_BasicMeasurements`。

## 8. SysConfig / 引脚

复制 restored example04 的同步 ADC/DMA SysConfig；本主题只使用 ADC0 的 `adc_samples`。

## 9. main.c 流程

采集、ADC code 转电压、计算所有统计量。

## 10. 每个 COPY 区说明

`centered_samples` 是 AC RMS 专用工作数组；不会改写 `voltage_samples`。

## 11. 如何复制到新工程

复制本 COPY 区；如果已有 ADC DMA，直接提供 `adc_samples` 即可。

## 12. 可调参数

参考电压、ADC 满量程码、削顶边界 0.02/3.28 V。

## 13. 常见错误

不要把 ADC code 直接当 V；AC RMS 必须在去 DC 后计算。

## 14. 本工程没有做什么

不测频、不做 FFT、不滤波。

## 15. Build 状态

待统一 SysConfig 生成和 CCS Compile/Link 审计；实板 `NOT_RUN`。
