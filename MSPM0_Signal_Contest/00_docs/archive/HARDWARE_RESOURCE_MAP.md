# MSPM0G3507 外设资源地图

本表以本机 MSPM0 SDK 2.11.00.07、LP-MSPM0G3507 SysConfig 1.28.0 的实际生成结果为准。未进入 profile 的 OPA/GPAMP/VREF 不分配“想象中的默认资源”。

## 芯片资源预算

| 类别 | 可见资源 | 工程规则 |
|---|---|---|
| ADC | ADC0、ADC1 | 单 ADC 默认 ADC0；双 ADC 用 ADC0+ADC1 |
| DAC | DAC0 | 外部 DAC_OUT 使用 PA15 |
| Comparator | COMP0 | 当前频率 profile 用 PA27 负输入 |
| OPA | OPA0/OPA1（官方 signal-chain 示例可用） | 尚未纳入六套冻结 profile，使用前必须重新跑 SysConfig 冲突检查 |
| GPAMP | GPAMP | 尚未纳入六套冻结 profile |
| DMA | 7 channels，其中 CH0..CH2 是 Full Channel | 复杂 ADC/DAC 流优先保留 CH0..2；不得假设 CH3..6 支持 Full Channel 模式 |
| General Timer | TIMG0、TIMG6、TIMG7、TIMG8、TIMG12 | 默认：ADC=TIMG0，DAC=TIMG6，Capture=TIMG7 |
| Advanced Timer | TIMA0、TIMA1 | 当前 profile 不占用，留给需要高级 PWM/捕获的赛题配置 |
| Event channel | 0..15 | 冻结分配：1/2=ADC，3=DAC，4=Comparator→Capture |
| UART | UART0 等 | profile 统一 UART0 PA10/PA11，115200 |

DMA 数量/Full Channel 数来自 SDK device header `source/ti/devices/msp/m0p/mspm0g350x.h`。具体 pinmux 和 Event route 均由 `.syscfg` 生成，不手写猜测。

## 六套集成 profile 的固定分配

| Profile | Pin | Peripheral | Timer | DMA | Event | UART |
|---|---|---|---|---|---|---|
| P01 ADC_CAPTURE | PA25 | ADC0.2 | TIMG0, 100 kHz | CH0 | 1 | UART0 PA10/11 |
| P02 DUAL_ADC | PA25 / PA17 | ADC0.2 / ADC1.2 | 共用 TIMG0, 100 kHz | CH0 / CH1 | 1 / 2 | UART0 PA10/11 |
| P03 DAC_GENERATOR | PA15 | DAC0 FIFO HWTRIG0 | TIMG6, 100 kHz | CH1 | 3 | UART0 PA10/11 |
| P04 ADC_DAC | PA25 / PA15 | ADC0.2 / DAC0 | TIMG0 / TIMG6 | CH0 / CH1 | 1 / 3 | UART0 PA10/11 |
| P05 FREQUENCY | PA27 | COMP0 → TIMG6 Capture | TIMG6, BUSCLK 32 MHz | 无 | 4 | UART0 PA10/11 |
| P06 FULL_SIGNAL | PA25 / PA17 / PA15 / PA27 | ADC0.2 / ADC1.2 / DAC0 / COMP0 | TIMG0 / TIMG6 / TIMG7 | CH0 / CH2 / CH1 | 1 / 2 / 3 / 4 | UART0 PA10/11 |

P01 的“CAPTURE”指 ADC block capture，不是 Timer edge capture。Timer edge capture 对应 P05。

### Clock 与 interrupt 预算

| Profile | Clock | 已生成 interrupt 配置 |
|---|---|---|
| P01 | CPU/BUSCLK 32 MHz；ADC ULPCLK；TIMG0 BUSCLK；UART BUSCLK | ADC0 `DMA_DONE`；UART polling |
| P02 | 同 P01；两个 ADC 均用 ULPCLK | ADC0/ADC1 `DMA_DONE`；UART polling |
| P03 | CPU/BUSCLK 32 MHz；TIMG6 BUSCLK；MFPCLK gate enabled | DAC `DMA_DONE`；UART polling |
| P04 | P01 + P03 clocks | ADC0 `DMA_DONE` + DAC `DMA_DONE` |
| P05 | Capture 与 UART 使用 BUSCLK 32 MHz；Capture period 2 ms | TIMG6 `CC0_DN` 与 `ZERO` |
| P06 | ADC/DAC/UART/Capture 使用 32 MHz clock tree；Capture period 2 ms | ADC0/ADC1 `DMA_DONE`、DAC `DMA_DONE`、TIMG7 `CC0_DN/ZERO` |

是否实际使能 NVIC、ISR 如何清 flag 和两个 DMA 如何汇合，属于 application adapter；SysConfig 生成 interrupt mask 不等于正式 ISR 已实现。

## 40 个正式模块的硬件所有权

“无固定”表示 public module 只做校验/状态/数据变换或调用注入 callback，不能从模块名推导出 pin/channel。

| 模块 | Pin/ADC/DAC/Analog | Timer/DMA/Event | Interrupt/Clock ownership |
|---|---|---|---|
| bsp/adc | 无固定；adapter 可选 ADCx | 无固定 | adapter 负责 |
| bsp/comparator | 无固定；adapter 可选 COMP0 | 无固定 | adapter 负责 |
| bsp/dac | 无固定；adapter 可选 DAC0 | 无固定 | adapter 负责 |
| bsp/dma | source/destination 描述，不选 CHx | 无固定 | adapter 负责 |
| bsp/gpamp | 配置结构，不选 pin/instance | 无固定 | adapter 负责 |
| bsp/gpio | port/pin 由调用者对象决定 | 无固定 | adapter 负责 |
| bsp/opa | 配置结构，不选 OPA/pin | 无固定 | adapter 负责 |
| bsp/system_clock | 只校验频率/算 ticks | 无固定 | 调用者提供 clock Hz |
| bsp/timer | adapter 决定 TIMx | 无固定 | adapter 负责 |
| bsp/uart | adapter 决定 UART/pin | 无固定 | adapter 负责 |
| bsp/vref | 只计算有效 Vref | 无固定 | 无 |
| acq/adc_basic | 经 bsp/adc adapter | 无固定 | 无自有 ISR/clock |
| acq/adc_continuous | frame 状态机 | 无固定 | 回调提供采集 |
| acq/adc_dma | P01/P04：PA25 ADC0.2 | TIMG0/DMA0/Event1 | ADC0 DMA_DONE ISR；Timer clock 由 config 提供 |
| acq/adc_dual_sync | 纯数据拆分 | 无固定 | 无 |
| acq/adc_pingpong_dma | 静态 buffer ownership | DMA channel 由外部 ISR/adapter | 无自有 ISR |
| acq/adc_ring_buffer | RAM ring | 无 | 无 |
| acq/adc_timer_trigger | ADC/Timer callback coordinator | 无固定 | adapter 负责 |
| acq/timer_capture | 时间戳数学 | 无固定 | 调用者提供 timer clock |
| acq/trigger_capture | ADC 数组触发/截帧 | 无 | 无 |
| gen/am_modulation | RAM arrays | 无 | 无 |
| gen/arbitrary_wave | RAM wave table | 无 | 无 |
| gen/dac_dc | 经 bsp/dac adapter | 无固定 | adapter 负责 |
| gen/dac_dma | DMA start/stop callback wrapper | 无固定 | adapter 负责 |
| gen/dac_wave_table | RAM wave table | 无 | 无 |
| gen/dds | RAM table/phase state | 无 | 调用者提供 update rate |
| gen/frequency_sweep | RAM frequency list | 无 | 无 |
| gen/sawtooth | RAM wave table | 无 | 无 |
| gen/sine | RAM wave table | 无 | 无 |
| gen/square | RAM wave table | 无 | 无 |
| gen/triangle | RAM wave table | 无 | 无 |
| frontend/comparator_threshold | 输出 COMP 配置意图 | 无固定 | bsp adapter 负责 |
| frontend/comparator_zero_cross | 输出 COMP 配置意图 | 无固定 | bsp adapter 负责 |
| frontend/gpamp_buffer | 输出 GPAMP 配置意图 | 无固定 | bsp adapter 负责 |
| frontend/gpamp_gain | 输出 GPAMP 配置意图 | 无固定 | bsp adapter 负责 |
| frontend/opa_buffer | 输出 OPA 配置意图 | 无固定 | bsp adapter 负责 |
| frontend/opa_dac_bias | 计算 bias，不占 DAC | 无 | 无 |
| frontend/opa_inverting | 输出 OPA 配置意图 | 无固定 | bsp adapter 负责 |
| frontend/opa_noninverting_pga | 输出 OPA 配置意图 | 无固定 | bsp adapter 负责 |
| frontend/opa_to_adc | 纯范围检查 | 无 | 无 |

## 现有三个 ADC_DMA Demo

| Demo | ADC pin/channel | Timer/Event/DMA | 额外资源 |
|---|---|---|---|
| `adc_dma_demo` | PA25 / ADC0.2 | TIMG0 / Event1 / DMA_CH0 | PA12 validation GPIO（debug only） |
| `adc_dma_onboard_selftest` | PB24 / ADC0.5 | TIMG0 / Event1 / DMA_CH0 | LFXT 32.768 kHz + FCC；板载 TMP6131 |
| `adc_buffer_uart_dump` | PB24 / ADC0.5 | TIMG0 / Event1 / DMA_CH0 | UART0 PA10/PA11, 115200 |

板载 TMP6131 路径只属于 test profile，不改变正式 PA25/ADC0.2 输入约定。

## 官方 SDK 交叉依据

下列本地官方示例用于确认资源组合，而不是复制生成文件：

- 双 ADC 同步：`examples/nortos/LP_MSPM0G3507/driverlib/adc12_simultaneous_trigger_event`
- ADC0/ADC1 DMA 同采：SDK 中 `adc_simultaneous_sample` 示例
- DAC FIFO + Timer Event：`examples/nortos/LP_MSPM0G3507/driverlib/dac12_fifo_timer_event`
- DAC DMA：`examples/nortos/LP_MSPM0G3507/driverlib/dac12_dma_sampletimegen`
- Timer Capture：SDK capture edge 示例
- COMP Event → Timer：`examples/nortos/LP_MSPM0G3507/driverlib/comp_dac_to_timer_event`
- GPAMP → ADC：`examples/nortos/LP_MSPM0G3507/driverlib/gpamp_buffer_to_adc`
- OPA signal chain → ADC：SDK `opa_signal_chain_to_adc` 示例

## 低功耗约束

SysConfig 明确提示 TIMG6/TIMG7 在 STOP/STANDBY 不保留配置。本工程当前 smoke main 只进入 WFI，不宣称 STOP/STANDBY 恢复已验证。比赛应用若切换低功耗等级，必须把 Timer 保存/恢复或重新初始化纳入状态机。
