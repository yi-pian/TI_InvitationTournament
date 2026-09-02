# 21_time_domain_waveform

## 推荐复制函数

`AcquireADCFrame() + PrepareDisplaySamples() + DrawTimeDomainWaveform()`（COPY 区 `TIME_DOMAIN_PREPARE`、`TIME_DOMAIN_PLOT`）。输入为 `adc_samples[]`（ADC code）；`voltage_samples[]` 为 V，当前 Y 映射保持 ADC code 满量程显示。

## 1. 这个工程干什么

把 `adc_samples` 映射成 TFT 时域折线，重点演示采样点与屏幕像素的对应关系。

## 2. 这个工程包含哪些子功能

| 我要做什么 | COPY 区域 |
|---|---|
| 绘制 ADC 时域波形 | `TIME_DOMAIN_PLOT` |

## 3. 输入

`adc_samples` 为 `uint16_t` ADC code，`SAMPLE_COUNT` 为帧点数，`sample_rate_hz` 为实际 Hz。

## 4. 输出

`voltage_samples`（V）和 TFT 折线。

## 5. 公共数据链

`ADC DMA → adc_samples → X/Y mapping → TFT_ST7789_DrawLine`。

## 6. 功能与 COPY 区对应表

只有 `TIME_DOMAIN_PLOT`；可直接接在 `02_adc_dma` 的 `adc_samples` 后。

## 7. 使用的模块

同步 ADC、ST7789 平台驱动与底层绘图模块；依据 restored example04 的 TFT 初始化和真实头文件。

## 8. SysConfig / 引脚

复制 restored example04 ADC+TFT 配置，不重分配 SPI 或 GPIO。

## 9. main.c 流程

采集一帧、更新电压数组、清绘图区、按列画相邻采样点连线。

## 10. 每个 COPY 区说明

当点数多于图宽时，每个 X 像素选择按比例对应的样本；Y 使用 ADC 满量程做固定缩放。

## 11. 如何复制到新工程

复制 TFT/ADC 模块、`TIME_DOMAIN_PLOT`、同一 SysConfig；已有 ADC 时可删除采集部分。

## 12. 可调参数

`GRAPH_X/Y/W/H`、Y 量程、刷新周期。

## 13. 常见错误

不清局部区域会残留旧曲线；ADC code 映射需与实际 ADC 满量程一致。

## 14. 本工程没有做什么

不做 FFT、触发对齐或自动量程。

## 15. Build 状态

待统一 SysConfig 生成和 CCS Compile/Link 审计；实板 `NOT_RUN`。
