# 10_timer_frequency

## 推荐复制函数

`InitTimerFrequencyMeasurement() + MeasureTimerFrequency()`（COPY 区 `TIMER_CAPTURE`）。输出 `frequency_hz` 和 `duty_cycle_percent`，不依赖 ADC 数组。

## 1. 这个工程干什么

用 COMP0 整形后的外部脉冲驱动 TIMG Capture，得到频率和占空比。

## 2. 这个工程包含哪些子功能

| 我要做什么 | COPY 区域 |
|---|---|
| 硬件捕获测频和占空比 | `TIMER_CAPTURE` |

## 3. 输入

比较器输入 PA27；数据格式是硬件边沿，不是 `adc_samples`。

## 4. 输出

`frequency_hz`（Hz）、`duty_cycle_percent`（%）和 `capture_result`。

## 5. 公共数据链

`PA27 → COMP0 → TIMG6 Capture → capture_result`。

## 6. 使用的模块

`signal_timer_capture_mspm0g3507.c/.h`、`signal_status.h`；依据：`02_acquisition/timer_capture/README.md` 和真实头文件。

## 7. SysConfig / 引脚

直接复制 `PROFILE_05_FREQUENCY/profile.syscfg`；不修改 `SIGNAL_CAPTURE`、`SIGNAL_COMP`、TIMG6、COMP0 或 PA27。

## 8. main.c 流程

初始化 SysConfig 和 Capture 模块，循环读取已由中断更新的结果。

## 9. 如何复制到新工程

复制 `modules` 中列出的三个文件、`TIMER_CAPTURE` 区和同一 SysConfig profile；保留生成的宏名。

## 10. 可调参数

仅可在 SysConfig 中按实际硬件调整 Capture 时钟/周期；不要在模块中改宏。

## 11. 常见错误

没有比较器边沿时 `capture_result.valid` 为 false；Capture 实例与 profile 不一致会导致无法编译。

## 12. 本工程没有做什么

不采 ADC、不做过零算法，也不显示 TFT。

## 13. Build 状态

待统一 SysConfig 生成和 CCS Compile/Link 审计；实板验证为 `NOT_RUN`。
