# 15_filter_processing

对一帧 `voltage_samples[]`（float，V）执行 Moving Average、Median、Hampel、FIR 或 IIR，输出同长度 `filtered_samples[]`（float，V）。

## 最简单复制

复制 `FILTER_CONVERT` 和所需的 `MOVING_AVERAGE`、`MEDIAN_HAMPEL` 或 `FIR_IIR`。输入是 `voltage_samples[]`，输出是 `filtered_samples[]`；所有算法函数均为 `main.c` 的 `static` 函数。

## 变量与复用

- `adc_samples[]`：`uint16_t` ADC code；`voltage_samples[]`、`filtered_samples[]`：物理 V。
- `workspace[]`：Median/Hampel 共用；`fir_state[]`、`iir_state[]`：CMSIS-DSP 状态。
- Median/Hampel 复用已有模块；FIR/IIR 复用 CMSIS-DSP，未新增模块。
- `Filter_Process()` 单帧只运行选择的一条链；本例每帧清状态，连续流滤波请保留状态。
