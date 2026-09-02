# 01_adc_basic

## 推荐复制函数

`ReadADCOnce()`（COPY 区 `ADC_BASIC`）。输入为 ADC 模拟电压，输出统一的 `adc_samples[0]`（`uint16_t` ADC code）。

## 1. 这个工程干什么

按 `adc_basic` README 推荐方式，直接用 SysConfig 生成的 DriverLib 宏读取一次 ADC code。

## 2. 这个工程包含哪些子功能

| 我要做什么 | COPY 区域 |
|---|---|
| 单点 ADC polling | `ADC_BASIC` |

## 3. 输入

PA25/ADC0 模拟输入。

## 4. 输出

`adc_samples[0]`，即 `uint16_t` ADC code；`SAMPLE_COUNT=1`。

## 5. 公共数据链

`clear flag → start conversion → wait result-loaded → read MEM0`。

## 6. 功能与 COPY 区对应表

只有 `ADC_BASIC`。

## 7. 使用的模块

无旧 ADC wrapper；依据 `02_acquisition/adc_basic/README.md` 的推荐 DriverLib 代码。

## 8. SysConfig / 引脚

复制 `PROFILE_07_BASIC_IO/profile.syscfg`，实例 `SIGNAL_BASIC_ADC`，ADC0/PA25。

## 9. main.c 流程

每轮清中断标志、启动、等待、读取一个 code。

## 10. 每个 COPY 区说明

输出不是电压；接后续处理前先做 ADC code→voltage 换算。

## 11. 如何复制到新工程

复制 `ADC_BASIC` 区和相同 P07 SysConfig；无需复制旧 ADC wrapper。

## 12. 可调参数

ADC reference/分辨率/采样时间仅在 SysConfig 改。

## 13. 常见错误

不清 flag 可能读旧结果；一次读不适合固定 Fs 波形采集。

## 14. 本工程没有做什么

不做 DMA，不将 code 换算为 V。

## 15. Build 状态

待统一 SysConfig 生成和 CCS Compile/Link 审计；实板 `NOT_RUN`。
