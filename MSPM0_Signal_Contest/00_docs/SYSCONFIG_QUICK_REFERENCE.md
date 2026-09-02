# SysConfig 比赛现场速查

适用：MSPM0G3507、LQFP-64、LP_MSPM0G3507、SDK 2.11.00.07。详细步骤见 [SYSCONFIG_BEGINNER_GUIDE.md](SYSCONFIG_BEGINNER_GUIDE.md)。

> 只改 `.syscfg`，不要手改生成的 `ti_msp_dl_config.c/h`。修改后必须 Generate → compile → final link → 上板验证。

## 先选 Profile

| 功能 | Profile | 当前资源 |
|---|---|---|
| 单 ADC DMA | P01 `PROFILE_01_ADC_CAPTURE` | ADC0.2 PA25 / DMA0 / TIMG0 / Event1 |
| 双 ADC | P02 `PROFILE_02_DUAL_ADC` | ADC0 PA25 + ADC1 PA17 / DMA0+1 / Event1+2 |
| DAC DMA | P03 `PROFILE_03_DAC_GENERATOR` | DAC0 PA15 / DMA1 / TIMG6 / Event3 |
| ADC + DAC | P04 `PROFILE_04_ADC_DAC` | P01 + P03 |
| Comparator Capture | P05 `PROFILE_05_FREQUENCY` | COMP0 PA27 / Event4 / TIMG6 Capture |
| 全功能资源基线 | P06 `PROFILE_06_FULL_SIGNAL` | DMA0/1/2、TIMG0/6/7、Event1..4 |

P01–P06 只有 SysConfig/compile/link PASS，不等于板测通过。

## 常用动作

| 我要改 | 进入哪里 | 改什么 | 同步检查 | 详细教程 |
|---|---|---|---|---|
| ADC Pin | ADC12 → ADCMEM0 + PinMux | Input Channel 与 Pin 一起改 | VREF、接线、ADC_ToVoltage | [ADC](SYSCONFIG_BEGINNER_GUIDE.md#adc) |
| Fs | TIMER → Clock/Divider/Prescaler/Period | `T=1/Fs` | 软件 `timer_clock_hz`、下游 Fs | [Timer](SYSCONFIG_BEGINNER_GUIDE.md#timer) |
| ADC DMA | Timer Event → ADC Event trigger → ADC DMA | Publisher/Subscriber 同 Channel | DMA width/direction/N buffer | [完整链](SYSCONFIG_BEGINNER_GUIDE.md#adc-timer-dma) |
| 双 ADC | 再加 ADC1 + 独立 DMA + 第二 Event | P02：PA17/DMA1/Event2 | 两块 buffer，不假设 interleaved | [完整链](SYSCONFIG_BEGINNER_GUIDE.md#adc-timer-dma) |
| DAC 固定电压 | DAC12 → Reference + Output Pin | 不需要 Timer/DMA | 码值、PA15、负载 | [DAC](SYSCONFIG_BEGINNER_GUIDE.md#dac) |
| DAC 波形 | TIMER → Event → DAC HWTRIG0/FIFO → DMA | P03：TIMG6/Event3/DMA1/PA15 | update rate、DDS rate、buffer | [DAC](SYSCONFIG_BEGINNER_GUIDE.md#dac) |
| Capture 测频 | COMP OUTPUT_EDGE → Event → CAPTURE Trigger | P05：Event4/TIMG6/PA27 | threshold、极性、tick Hz | [Comparator](SYSCONFIG_BEGINNER_GUIDE.md#comparator) |
| UART | UART → Baud + TX/RX Pin | UART0 PA10/PA11 | **关闭 Internal Loopback** | [UART](SYSCONFIG_BEGINNER_GUIDE.md#uart) |
| OPA/GPAMP | 输入 MUX + Gain/Feedback + Output/Internal route | 只选当前设备可用项 | 共模、摆幅、带宽、ADC 换算 | [OPA/GPAMP](SYSCONFIG_BEGINNER_GUIDE.md#opa-gpamp) |

## 100 kHz Timer

```text
T = 1 / 100000 = 10 us
TimerClock = Source / Divider / Prescaler
Load = TimerClock × 10 us - 1
```

当前 P01：BUSCLK/1/1=32 MHz，Desired Period=10 us，生成 Load=319。时钟树变化后重新看 Calculated Timer Clock/Actual Period。

## ADC DMA 必查

```text
TIMG0 ZERO_EVENT --Event1--> ADC0 Event Trigger
ADC MEM0 loaded ------------> DMA_CH0
ADC result -----------------> uint16_t raw[N]
DMA done -------------------> frame DONE
```

- ADC 实例名：`SIGNAL_ADC`
- DMA 名：`SIGNAL_ADC_DMA`
- Timer 名：`SIGNAL_SAMPLE_TIMER`
- ADC：Event trigger、repeat、MEM0 trigger next、MEM0 DMA trigger
- DMA：peripheral→block、half-word、single
- N：在 `SignalADC_Start(raw,N)`，不在 SysConfig `DMA Samples Count`

## DAC DMA 必查

```text
wave[N] --DMA_CH1--> DAC FIFO --> PA15
TIMG6 ZERO_EVENT --Event3--> DAC HWTRIG0
DAC FIFO threshold ---------> DMA refill request
```

- FIFO enable、HWTRIG0、DMA trigger enable
- DMA：block→peripheral、half-word、repeat single
- `Ftable=Fupdate/N` 只适用于一张表恰好含一周期
- DDS：`Fout=tuning_word×Fupdate/2^32`

## Event 一句话

```text
Publisher Channel ID == Subscriber Channel ID
```

还必须选对发布条件：Timer 用 ZERO_EVENT；Comparator Capture 用 OUTPUT_EDGE。ADC 还必须把 Trigger Source 设为 Event。

## 当前 Pin

| 功能 | Pin |
|---|---|
| ADC0 Channel 2 | PA25 |
| ADC1 Channel 2 | PA17 |
| DAC0 OUT | PA15 |
| COMP0 外部负输入 | PA27 |
| UART0 TX/RX | PA10/PA11 |

这些是当前 Profile 的 Pin，不是所有候选 Pin。换 Pin 时只相信当前 Device SysConfig 下拉框 + LP 原理图。

## 冲突解决顺序

1. 删除不用的硬件链。
2. 固定模拟 Pin/内部路由。
3. 安排 ADC/Timer/Capture 实例。
4. 分配 DMA；P06 的三条 Full Channel 已全占。
5. 统一 Event Channel。
6. 最后移动 UART/GPIO。

详细见 [资源冲突](SYSCONFIG_BEGINNER_GUIDE.md#conflicts)。

## 最后 8 项

- Device/Package/Board 正确
- PinMux 无冲突
- DMA/Timer/Event owner 唯一
- Actual Period/Calculated Baud 正确
- 生成宏名仍匹配正式模块
- SysConfig Generate PASS
- 全量 compile + final link PASS
- 上板用仪器验证；Build PASS 不等于 BOARD_VERIFIED
