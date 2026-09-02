# 比赛现场资源重分配指南

先选最接近题目的 profile，再删资源，最后才新增或改 pin。不要把六个 `.syscfg` 文本机械合并。

## 推荐全功能预算

P06 是当前最大无冲突基线：

| 职责 | 默认资源 | 允许的优先调整 |
|---|---|---|
| ADC channel A | ADC0.2 / PA25 / DMA_CH0 / Event1 | 换通道时只在 SysConfig 改 ADC pin/channel，并同步应用元数据 |
| ADC channel B | ADC1.2 / PA17 / DMA_CH2 / Event2 | 单通道题直接删除，不占着 ADC1/CH2 |
| DAC generator | DAC0 / PA15 / DMA_CH1 / TIMG6 / Event3 | DAC0/PA15 基本固定；优先改 Timer/Event，不抢 ADC DMA |
| Comparator | COMP0 / PA27 / Event4 | 输入 pin 必须由 SysConfig 选择合法复用 |
| Edge capture | TIMG7 / Event4 | 若无 DAC，可改回 TIMG6；否则保留 TIMG7 |
| ADC sample timer | TIMG0 / Event1+2 | 两个 ADC 共用周期源，不复制第二个 Timer |
| Debug UART | UART0 / PA10+PA11 / 115200 | 决赛发布版可删除；采样链不能依赖它 |

## DMA 分配顺序

1. CH0：主 ADC block。
2. CH1：DAC FIFO repeat stream。
3. CH2：第二 ADC block。
4. CH3..6：仅在 SysConfig 明确接受所需 transfer mode 后使用；它们不是 Full Channel，不承诺可替换 P06 中任一复杂流。

如果只需要 ADC+UART，选 P01，不要带 DAC/COMP。只需要 DAC，选 P03。资源删除比现场换 channel 更安全。

## Timer 分配顺序

| 优先级 | Timer | 用途 |
|---|---|---|
| 1 | TIMG0 | ADC sampling trigger |
| 2 | TIMG6 | DAC update trigger |
| 3 | TIMG7 | comparator/event capture |
| reserve | TIMG8/TIMG12 | 临时周期任务、debug validation |
| reserve | TIMA0/TIMA1 | 需要高级计数/PWM 时再启用 |

改变采样率或 DAC 更新率时，改 profile 中 `timerPeriod` 或通过正式模块 API 改 load；不要同时修改 clock tree、divider、prescaler 和 load，除非重新计算全部关系。

## Event channel 分配

| Channel | 冻结职责 |
|---|---|
| 1 | TIMG0 → ADC0 |
| 2 | TIMG0 → ADC1 |
| 3 | TIMG6 → DAC0 HWTRIG0 |
| 4 | COMP0 edge → TIMG capture |

0、5..15 当前 profile 未占用，但“未占用”不是永久保留。增加 route 时先查 SysConfig Event 图，不能只看编号不看 publisher/subscriber 类型。

## 现场改 ADC 输入

1. 复制最接近的 profile 到比赛 application 目录。
2. 在 SysConfig GUI 中选 ADC instance、MEM0 channel 和合法 pin；让工具更新 pinmux。
3. 保持 `.c` 里 `#include "signal_adc_dma.h"`，不要改成跨目录相对 include。
4. 如果生成宏名仍是 `SIGNAL_ADC/SIGNAL_ADC_DMA/SIGNAL_SAMPLE_TIMER`，现有 ADC_DMA 可直接复用；若 profile 使用 A/B 名称，应写薄 adapter，不能在正式模块里堆条件宏。
5. 重新生成、Clean、Rebuild，并核对实际 `-I` 和生成的 `*_INST` 宏。
6. 更新 frame 的 `reference_voltage_v`、`adc_bits`、channel 注释和实板测试记录。

## 现场改采样点数/速率

- N：传给 `SignalADC_Start(buffer, N)`；buffer 必须至少 N 个 `uint16_t`，当前 API N 范围 1..65535。
- Fs：`SignalADC_SetSampleRate(Fs)`；模块按 `round(timer_clock/Fs)` 选整数计数。
- `GetConfiguredTriggerRate()` 只说明整数配置结果。需要相位/频率精度时，将独立测量误差作为算法校准参数。
- RAM 先算：单 ADC buffer = `2*N` bytes；双 ADC 两个独立 buffer = `4*N` bytes；ping-pong 再乘 2。

## OPA/GPAMP 接入规则

OPA/GPAMP 当前只有可组合的配置计算模块和 BSP callback 接口，没有冻结到 P01..P06。比赛需要时：

1. 从只包含必要 ADC/DAC 的 profile 分支；
2. 参考本机 SDK 对应官方 example 加真实 OPA/GPAMP instance；
3. 让 SysConfig 决定 pin 和内部 route；
4. 编写很薄的 DriverLib adapter 实现 BSP Apply callback；
5. 更新冲突矩阵和实板状态，不把 SysConfig PASS 写成 BOARD PASS。
