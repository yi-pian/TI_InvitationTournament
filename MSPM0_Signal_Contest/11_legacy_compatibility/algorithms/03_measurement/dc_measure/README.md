# DC Measure：直流分量测量

这是源码丢失后的 clean reimplementation。它复用正式 Mean 思想，不复制第二套复杂框架。

## 两种入口

1. 已有 `float voltage_v[N]`：`SignalDCMeasure_FromVoltage`，内部调用正式 `SignalMean_Process`。
2. 只有 `uint16_t raw[N]` 且换算为线性增益+偏置：`SignalDCMeasure_FromRawLinear`，无需另开 `float[N]` 缓冲。

```text
ADC raw → FromRawLinear → mean_code + dc_voltage_v
ADC raw → ADC_ToVoltage → FromVoltage → dc_voltage_v
```

## 加入工程

链接本模块，并同时链接 `03_measurement/mean/signal_mean.c`。Include Path：本目录、`mean`、`adc_to_voltage`、`03_measurement/common`。纯算法，不需要 SysConfig。

```c
signal_dc_measure_result_t r;
signal_adc_to_voltage_config_t c = {4095U, 3.3f, 1.0f, 0.0f};
SignalDCMeasure_FromRawLinear(raw, N, &c, &r);
```

比赛时修改 `adc_max_code`、真实 VREF、前端 `input_scale` 和 `offset_voltage_v`。如果传感器换算非线性，必须先逐点转换再调用 FromVoltage。DC 对慢漂移敏感；需要稳定结果可多帧平均，但不能用 AC RMS 代替 DC。错误和精度约定见 [REIMPLEMENTATION_SPEC](REIMPLEMENTATION_SPEC.md)。
