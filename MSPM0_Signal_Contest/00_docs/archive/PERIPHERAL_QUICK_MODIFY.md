# 外设现场快速修改表

| 想改什么 | 改哪里 | 不要改哪里 | 修改后必须做 |
|---|---|---|---|
| ADC 点数 N | `signal_hw_config.h` 或 `SignalADC_Start(buffer,N)` 调用参数 | `signal_adc_dma.c` | 检查 buffer 字节数、重新 build、哨兵测试 |
| ADC Fs | 集中配置 + `SignalADC_SetSampleRate()` | 算法内部常量 | 比较 configured rate；重跑动态实板验证 |
| ADC pin/channel | application 的 `.syscfg` ADC MEM/pin | DriverLib 生成 `.c/.h`、跨目录 include | Generate/Clean/Rebuild；更新资源表和 Vref 元数据 |
| DAC update rate | `.syscfg` DAC Timer period 或 adapter 集中参数 | 波表生成函数内的魔数 | 重算输出频率、检查 FIFO/DMA 吞吐 |
| DAC table length | 静态 table 与 count 配置 | DMA ISR 临时分配 | 检查 RAM、DMA count 和 repeat 边界 |
| UART baud/pin | `.syscfg` UART instance | ADC_DMA 正式模块 | PC 端同步串口设置、Generate/Clean/Rebuild |
| DMA channel | `.syscfg` 的 `DMA_CHANNEL.peripheral.$assign` | 只改文档编号 | 查看 Full Channel 要求、重新生成并链接 |
| Timer instance | `.syscfg` peripheral assign | 手改 `*_INST` 宏 | 检查 STOP/STANDBY retention 和 Event route |
| Event channel | publisher 和 subscriber 两端同时在 `.syscfg` 改 | 只改一端数字 | 查看 Event.dot、重新生成 |
| Comparator input/threshold | `.syscfg` 合法 pin + frontend config | 猜 pinmux | 实板阈值/迟滞验证 |
| OPA/GPAMP | 从精简 profile 新增官方实例和薄 adapter | 直接塞进 P06 后跳过冲突检查 | SysConfig、编译、range check、实板 |

## SysConfig 与连带影响

| 我要改什么 | 主要修改位置 | 要改 SysConfig | 可能影响其他模块 |
|---|---|---:|---|
| 采样率 | `signal_hw_config.h` + `SignalADC_SetSampleRate` | 否；若改 clock/divider 则是 | Timer 带宽、ADC sample window、算法频率轴 |
| 采样点数 | buffer 定义 + Start 的 N | 否 | SRAM、DMA count、算法 workspace |
| ADC 通道 | `.syscfg` ADC MEM/pin | **是** | GPIO pinmux、模拟前端、Vref 元数据 |
| DualADC 两通道 | P02/P06 的 ADC_A/ADC_B pin/MEM | **是** | 两 DMA、Event1/2、同步 adapter |
| DMA Channel | `.syscfg` DMA assignment | **是** | DAC/第二 ADC；Full Channel 能力 |
| Timer | `.syscfg` instance/period | **是** | DAC/Capture 分配、retention、Event route |
| DAC 输出率 | TIMG6 period 或 DAC adapter 参数 | 通常是 | DDS 配置频率、DMA/FIFO 吞吐 |
| Comparator threshold | frontend config；必要时 COMP DAC 设置 | pin/reference 改动时是 | 过零点、迟滞、Capture event rate |
| UART baudrate | `.syscfg` UART baud | **是** | PC 终端设置、debug 输出时间 |

## 常用数值


### ADC buffer RAM

```text
single block bytes = N * 2
single ping-pong bytes = N * 2 * 2
dual independent blocks bytes = N * 2 * 2
dual ping-pong bytes = N * 2 * 2 * 2
```

N=4096 时：单 block 8192 B，单通道 ping-pong 16384 B，双通道 ping-pong 32768 B，已经占满 MSPM0G3507 的 32 KiB SRAM，且没有给 stack/其他状态留空间，因此双通道 ping-pong 4096 不可用。

### Timer 整数计数

```text
timer_count = round(timer_clock_hz / requested_rate_hz)
configured_rate = timer_clock_hz / timer_count
```

32 MHz 下：100 kHz→320 counts，200 kHz→160，500 kHz→64。当前生成宏 load 通常为 `count-1`。

### samples per cycle

```text
samples_per_cycle = configured_sample_rate / input_frequency
```

- 100 kSPS / 1 kHz = 100
- 200 kSPS / 1 kHz = 200
- 500 kSPS / 10 kHz = 50

这些是理论配置值，不是动态模拟性能证明。

## CCS 当场核对

- Generated header 中的 `CPUCLK_FREQ`、Timer `*_LOAD_VALUE`。
- ADC `*_INST`、`*_ADCMEM_0`、DMA `*_CHAN_ID`。
- Event publisher/subscriber channel。
- Console 真实 `-I` 物理路径。
- Linker `.map` 的 FLASH/SRAM used；不要只看源码数组大小。

## 30 秒回退策略

修改失败时，不改正式模块源码。回到最近通过的 profile，删除新增 peripheral，再逐个重新添加。只有 `profile.syscfg` 是配置 source of truth，`Debug/ti_msp_dl_config.*` 不是可维护文件。
