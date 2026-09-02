# 60_precision_measurement

## 推荐复制函数

`AcquireADCFrame() + ConvertADCToVoltage() + RunSineFit3Param()`；需要精修频率时额外复制 `RunSineFit4Param()`。将 `20_fft_analysis` 的 `frequency_hz` 赋给 `initial_frequency_hz` 可作为 4P 初值。

## 1. 这个工程干什么

用现有 3 参数、4 参数正弦拟合从电压样本求幅度、相位、DC 和精修频率。

## 2. 这个工程包含哪些子功能

| 我要做什么 | COPY 区域 |
|---|---|
| 已知频率下拟合幅相/DC | `SINE_FIT_3PARAM` |
| 初值附近精修频率 | `SINE_FIT_4PARAM` |

## 3. 输入

`adc_samples` 为 ADC code；`voltage_samples` 为 V；`initial_frequency_hz` 必须已知或由 `20_fft_analysis` 获得。

## 4. 输出

`frequency_hz`、`amplitude_v`、`phase_deg`、`mean_v`、`fit3_result`、`fit4_result`。

## 5. 公共数据链

`ADC → voltage_samples → 3P/4P sine fit`。

## 6. 功能与 COPY 区对应表

4P 独立运行也需要合理 `initial_frequency_hz`，可复用 FFT 主题的 `frequency_hz`。

## 7. 使用的模块

`signal_sine_fit_3param`、`signal_sine_fit_4param` 和同步 ADC；依据为 restored example04 `App_SineFitAndLockIn` 与真实头文件。

## 8. SysConfig / 引脚

复制 restored example04 ADC/DMA 配置。

## 9. main.c 流程

采样转电压，先 3P，再以相同初值运行 4P。

## 10. 每个 COPY 区说明

3P 不搜索频率；4P 仅在 `search_half_width_hz` 范围内迭代。

## 11. 如何复制到新工程

复制对应拟合模块、`voltage_samples` 换算和需要的 COPY 区。

## 12. 可调参数

`initial_frequency_hz`、搜索半宽、迭代数、采样率。

## 13. 常见错误

初值差太大、输入不是电压、采样率错误、非正弦或削顶都会降低可信度。

## 14. 本工程没有做什么

不在本工程重新实现 FFT 初频估计。

## 15. Build 状态

待统一 SysConfig 生成和 CCS Compile/Link 审计；实板 `NOT_RUN`。
