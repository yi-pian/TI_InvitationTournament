# 02_adc_dma

## 推荐复制函数

`InitADC() + AcquireADCFrame()`（COPY 区 `ADC_DMA`）。输出统一的 `adc_samples[]`、`SAMPLE_COUNT` 与 `sample_rate_hz`，可直接接 Basic、FFT 或时域图函数。

## 1. 这个工程干什么

以固定采样率采集一帧 `uint16_t adc_samples[]`。

## 2. 输入

ADC 模拟输入；`SAMPLE_COUNT`；`SAMPLE_RATE_HZ`。

## 2.1 功能与 COPY 区对应表

| 我要做什么 | COPY 区域 |
|---|---|
| 取得一帧统一命名的 ADC DMA 数据 | `ADC_DMA` |

## 3. 输出

`adc_samples` 是 ADC 原始码，`sample_rate_hz` 是实际触发率，`adc_frame_ready` 表示数组可用。

## 4. 数据链

`Timer Event -> ADC -> DMA -> adc_samples -> adc_frame_ready`。

## 5. 使用的集成库模块

`signal_adc_dma.c/.h` 与 `signal_status.h`，调用依据：`02_acquisition/adc_dma/README.md`。

## 6. SysConfig / 引脚

复制 `PROFILE_01_ADC_CAPTURE/profile.syscfg` 的已验证配置；实例名保持 `SIGNAL_ADC`、`SIGNAL_ADC_DMA`、`SIGNAL_SAMPLE_TIMER`。

## 7. main.c 流程

`SYSCFG_DL_init -> SignalADC_Init -> SignalADC_Start -> IsFinished -> 使用 adc_samples`。

## 8. 最关键代码

```c
SignalADC_Start(adc_samples, SAMPLE_COUNT);
while (!SignalADC_IsFinished()) { __WFI(); }
```

## 9. 如何复制到新工程

复制 modules 中三个文件、COPY 区和同名 SysConfig 资源。

## 10. 可以修改的参数

`SAMPLE_COUNT`、`SAMPLE_RATE_HZ`、ADC Pin/Channel（只在 SysConfig 改）。

## 11. 常见错误

完成前读 buffer；把 ADC 时钟当采样率；Timer Event 与 ADC subscriber 不一致。

## 12. 本工程没有做什么

不转换电压、不显示、不做算法。
