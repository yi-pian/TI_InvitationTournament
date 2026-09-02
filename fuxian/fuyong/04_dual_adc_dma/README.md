# 04_dual_adc_dma

## 推荐复制函数

`InitDualADC() + AcquireDualADCFrame()`（COPY 区 `DUAL_ADC_DMA`）。输出 `adc_ch1_samples[]`、`adc_ch2_samples[]` 与 `sample_rate_hz`；两路同下标同步。

## 1. 这个工程干什么

使用同一 Timer 触发两路 ADC、两路 DMA，取得可以按样本下标对应的双通道 ADC 原始码帧。

## 2. 输入

ADC0/PA25 为 A，ADC1/PA17 为 B；两路输入都必须在 ADC 允许电压范围内。

## 2.1 功能与 COPY 区对应表

| 我要做什么 | COPY 区域 |
|---|---|
| 取得同步双 ADC DMA 帧 | `DUAL_ADC_DMA` |

## 3. 输出

`adc_ch1_samples[]`、`adc_ch2_samples[]`、`sample_rate_hz` 和 `adc_frame_ready`。

## 4. 数据链

```text
Timer ZERO Event -> ADC A/B -> DMA A/B -> SignalDualADC_IsFinished() -> 两路 samples
```

## 5. 使用的集成库模块

| 模块 | 作用 | 依据 |
|---|---|---|
| `signal_dual_adc_mspm0g3507.c/.h` | 同步双 ADC DMA | `02_acquisition/adc_dual_sync/README.md` |
| `signal_status.h` | 返回状态 | 同上 |

## 6. SysConfig / 引脚

复制 example04 的已验证 SysConfig：`SIGNAL_ADC_A`、`SIGNAL_ADC_B`、两路 DMA 和 `SIGNAL_DUAL_ADC_TIMER`。不要手改生成文件。

## 7. main.c 流程

初始化 -> `SignalDualADC_Start()` -> 等待 `IsFinished()` -> 使用数组。

## 8. 最关键代码

```c
SignalDualADC_Start(adc_ch1_samples, adc_ch2_samples, SAMPLE_COUNT);
while (!SignalDualADC_IsFinished()) { __WFI(); }
```

## 9. 如何复制到新工程

复制 `modules/signal_dual_adc_mspm0g3507.c/.h`、`signal_status.h`，复制 `main.c` 的 COPY START/END 区，并配置同名 SysConfig 资源。

## 10. 可以修改的参数

`SAMPLE_COUNT`、`SAMPLE_RATE_HZ`、实际 ADC 引脚/通道（仅在 SysConfig 中改）。

## 11. 常见错误

DMA 未完成就读数组；把目标 Fs 当实际 Fs；两路 ADC 没使用同一 Timer Event。

## 12. 本工程没有做什么

不做 FFT、相位、TFT 或菜单；它只提供双通道采样帧。
