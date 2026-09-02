# DAC DMA Platform Adapter 使用说明

> 正式源码：`../signal_dac_dma_platform.c`、`../signal_dac_dma_platform.h`。本目录只保存说明。

代码分类：本 README 中未标为 `【COMPILE-VERIFIED EXAMPLE】` 的代码块均为 `【ILLUSTRATIVE SNIPPET】`；第一次使用以 `09_examples/platform_closure/dac_dma_minimum/main.c` 为准。

## 第一次使用 DAC DMA Platform Adapter？从这里开始

目标：“用当前 P03/P04 的 DriverLib 宏把 `uint16_t samples[]` 送进 DAC0 FIFO，并由 Timer+Event+DMA 定时更新”。它通常作为 `SignalDACDMA_Init` 的 start/stop 回调。

### STEP 1：加入工程

- 唯一正式源码/头文件：`MSPM0_Signal_Contest/08_applications/common/signal_dac_dma_platform.c/.h`
- 公共依赖：`MSPM0_Signal_Contest/01_bsp/common`
- 生成依赖：目标工程 `ti_msp_dl_config.h`
- Include Path：`08_applications/common`、`01_bsp/common`、SysConfig 生成目录

不要从说明目录复制源码；projectspec 链接上一级正式 `.c`。

### STEP 2：include

```c
#include "ti_msp_dl_config.h"
#include "signal_dac_dma_platform.h"
```

### STEP 3：变量

```c
static uint16_t samples[N];
```

平台不复制 block；输出期间数组必须保持有效且不被改写。

### STEP 4：参数

`update_rate_hz`/`timer_clock_hz` 决定整数 Timer count；`count` 范围 1..65535；`repeat` 决定 DMA 循环或 one-shot。Update rate 写错或与 DDS 不一致会使输出频率按比例错误。

### STEP 5：SysConfig

**【需要 SysConfig】**。`PROFILE_03_DAC_GENERATOR` 只用于对照字段；在 CCS 中双击目标工程 `.syscfg`，通过 SysConfig 图形界面配置 DAC0 FIFO+DMA+HWTRIG0+PA15、DMA_CH1 buffer→FIFO half-word、TIMG6 periodic、Event3 ZERO_EVENT→DAC trigger 和 DAC DMA done IRQ。保存后等待 CCS 自动重新生成，确认 `SIGNAL_DAC_TIMER_*`、`SIGNAL_DAC_DMA_*` 和 DAC0 宏存在且没有 DMA/Timer/Event/PinMux 冲突；不要直接编辑 `.syscfg` 文本或生成文件。

### STEP 6：初始化

```c
SYSCFG_DL_init();
(void)SignalDACPlatform_Init(100000U, 32000000U);
```

### STEP 7：真正调用

直接调用平台：

```c
(void)SignalDACPlatform_Start(NULL, samples, N, true);
```

更推荐通过通用 wrapper：

```c
(void)SignalDACDMA_Init(&dac_dma, NULL,
    SignalDACPlatform_Start, SignalDACPlatform_Stop);
(void)SignalDACDMA_Start(&dac_dma, samples, N, true);
```

### STEP 8：结果

物理输出在 PA15/DAC_OUT；软件可读 configured rate；one-shot 用 `SignalDACPlatform_IsOneShotFinished()`。

### STEP 9：连接

```text
DDS Fill / Replay Table -> DAC DMA wrapper -> Platform Adapter -> DAC0/PA15
```

Sweep 中 P04 同时还会占 ADC0/DMA0/TIMG0；保持 DAC 资源为独立 DMA_CH1/TIMG6/Event3。

### STEP 10：Build

生成宏/头文件缺失=SysConfig/Profile；undefined symbol=漏平台 `.c`；重复 `DAC12_IRQHandler`=别处也定义 ISR；无波形=检查 Timer/Event/FIFO/DMA 路由和 PA15；one-shot 永不完成=DMA-done IRQ 配置或 handler 未生效。

### STEP 11：验证

输出四点表 `{mid,high,mid,low}` 循环，用示波器检查 PA15 更新和周期；configured rate 只证明软件 Timer 配置，不代替真实波形测量。

### STEP 12：常见修改

1. 100 k→200 k update：改 Platform Init 和 SysConfig Timer，DDS update 同步。
2. repeat→one-shot：传 false，等待 finished 后再复用数组。
3. block 1000→512：同步数组/count，RAM 减少 976 B；循环边界要保持波形连续。
4. DAC 量程/偏置变化：重生成 wave table；平台只搬 raw code。

### STEP 13：完整最小示例

```c
#include "ti_msp_dl_config.h"
#include "signal_dac_dma_platform.h"
static uint16_t wave[4] = {2048U,3072U,2048U,1024U};
void Output(void)
{
    SYSCFG_DL_init();
    (void)SignalDACPlatform_Init(100000U, 32000000U);
    (void)SignalDACPlatform_Start(NULL, wave, 4U, true);
}
```

下面是平台 API、ISR、资源冲突和验证证据。

## 1. What It Does

把通用 DAC DMA wrapper 的 start/stop 回调接到 MSPM0 的 Timer → Event → DMA → DAC 硬件链。

小白理解：DDS/DAC Wave Table 只准备数字码；这个适配器才把 `uint16_t` 数组按固定更新率连续送到 DAC 引脚。

## 2. When To Use It

用于 DDS Generator、Sweep Analyzer、Waveform Replay 等定时播放 DAC code buffer。只输出一个固定直流码时使用 DAC/DC 更简单；还没有波表时先用 DDS/Wave Table 生成。

## 3. Where It Sits In The Signal Chain

```text
Wave Table / DDS -> uint16_t code[N] -> DAC DMA wrapper
                                      -> Platform Adapter -> Timer/Event/DMA/DAC -> pin
```

## 4. Inputs / Outputs

- 输入：更新率与 Timer 计数时钟；只读 `uint16_t samples[count]`；`repeat`。
- 输出：DAC pin 的时间序列、one-shot 完成标志和配置推导更新率。
- `context` 为通用 wrapper 回调签名保留；当前实现明确忽略它。

## 5. Dependencies

Required：`signal_status.h`、DriverLib、生成的 `ti_msp_dl_config.h`、DAC/DMA/Timer/Event/IRQ。通常还链接 `06_generator/dac_dma/signal_dac_dma.c`，并把 `SignalDACPlatform_Start/Stop` 作为回调传给它。参考 P03/P04/P06 和现有 DDS/Sweep/Replay projectspec。

## 6. Public API Reference

### `SignalDACPlatform_Init(update_rate_hz, timer_clock_hz)`

两参数均为 `uint32_t` Hz，必须非 0 且更新率不超过 Timer 时钟。内部取最近整数 Timer count，范围 `1..65536`；停止 Timer、写 load/count、清 one-shot 状态并使能 DAC IRQ。调用前先 `SYSCFG_DL_init()`。

### `SignalDACPlatform_Start(context, samples, count, repeat)`

- `context`：当前实现不使用，可为 wrapper 传入的值。
- `samples`：非空、只读 DAC code 数组；播放期间必须一直有效。
- `count`：元素数，必须 `1..UINT16_MAX`，不是 byte 数。
- `repeat=false`：播一遍后 ISR 停止并置完成；`true`：DMA done 后重新允许触发，持续循环，需显式 Stop。
- 未 Init 或参数错误返回对应 `signal_result_t`。

### `SignalDACPlatform_Stop(context)`

Init 后可调用；停止 Timer、DAC DMA trigger 和 DMA channel。`context` 仍被忽略。未 Init 返回 `NOT_INITIALIZED`。

### `SignalDACPlatform_IsOneShotFinished()`

仅 one-shot 播放自然结束后为 true；repeat 模式不会靠此函数结束。

### `SignalDACPlatform_GetConfiguredRate()`

返回整数 Timer 分频得到的实际配置更新率 Hz。Init 前为 0；不是示波器实测值。

## 7. Call Sequence

```text
SYSCFG_DL_init -> Platform_Init
              -> SignalDACDMA_Init(wrapper, ..., Platform_Start, Platform_Stop)
              -> SignalDACDMA_Start(code, N, repeat)
              -> one-shot 等 finished / repeat 运行
              -> SignalDACDMA_Stop
```

## 8. Minimal Example

```c
signal_dac_dma_t player;
SYSCFG_DL_init();
SignalDACPlatform_Init(200000U, CPUCLK_FREQ);
SignalDACDMA_Init(&player, NULL,
    SignalDACPlatform_Start, SignalDACPlatform_Stop);
SignalDACDMA_Start(&player, dac_codes, code_count, true);
/* ... */
SignalDACDMA_Stop(&player);
```

## 9. Connecting To Other Modules

```text
Sine/DAC Wave Table -> DDS_Fill -> DAC DMA -> Platform Adapter
Captured period raw -> voltage/scale to DAC code -> DAC DMA -> Platform Adapter
Frequency Sweep -> DDS_SetFrequency/Fill -> DAC DMA -> Platform Adapter
```

真实调用参考 `dds_generator/main.c`、`sweep_analyzer/main.c`、`waveform_capture_replay/main.c`。

## 10. Parameter Guide

| 参数 | 作用 | 增大 | 减小 | RAM | SysConfig |
|---|---|---|---|---|---|
| `update_rate_hz` | DAC 每秒更新次数 | 波形时间分辨率/负载提高 | 高频输出能力下降 | 不直接改变 | 改时钟/能力边界时需核对 |
| `count` | 一次/循环表长 | 循环周期变长 | 重复更快 | code buffer=`2*count` | 否 |
| `repeat` | 连续循环 | `true` 不自然结束 | `false` 播一遍 | 无 | 否 |
| `timer_clock_hz` | Timer 真实计数率 | 分频分辨率提高 | 最高更新率受限 | 无 | 时钟树变化时是 |

输出基波关系由上游决定：直接循环 N 点波表通常 `f_out=update_rate_hz/N`；DDS Fill 的结果还由 tuning word 决定，不能只改 Timer 猜频率。

## 11. Common Modification Tasks

- 改 DAC 更新率：改 Platform Init 参数，并读取 GetConfiguredRate 传回 DDS 频率计算。
- 改循环长度：同步 code 数组与 `count`；不需要 SysConfig。
- 改 DAC 输出 pin/reference：修改 P03/P04/P06 派生 `.syscfg`，并核对模拟连线。
- 从循环改成单次：`repeat=false`，等待 `SignalDACPlatform_IsOneShotFinished()`。

## 12. Config vs SysConfig

CONFIG ONLY：code 内容、count、repeat、DDS frequency/amplitude/phase。SYSCONFIG REQUIRED：DAC instance/pin/reference、DMA channel、Timer/Event/IRQ、时钟树。

## 13. SysConfig Setup

新手逐项配置：[Timer/Event/DMA/DAC 连续波形教程](../../../00_docs/SYSCONFIG_BEGINNER_GUIDE.md#dac)。本 Adapter 的正式资源契约是 P03：`SIGNAL_DAC_DMA`/DMA_CH1、`SIGNAL_DAC_TIMER`/TIMG6、Event 3、DAC0/PA15；现场速查见 [Quick Reference](../../../00_docs/SYSCONFIG_QUICK_REFERENCE.md)。

参考 `PROFILE_03_DAC_GENERATOR`、`PROFILE_04_ADC_DAC` 或 `PROFILE_06_FULL_SIGNAL`。需要 1 DAC、1 DMA channel、1 Timer 发布更新事件、DAC DMA trigger/IRQ 和 DAC output pin。源码依赖生成宏 `SIGNAL_DAC_TIMER_INST`、`SIGNAL_DAC_DMA_CHAN_ID`、`DAC0`、`DAC12_INT_IRQN`。

## 14. Resources / Memory

硬件：1 Timer、1 DMA、1 DAC、Event、DAC IRQ、1 analog output pin。调用者 code buffer 为 `2*count` bytes；适配器只保存常数大小状态。

## 15. Buffer Rules

数组元素必须是 `uint16_t` DAC code；容量至少 count。DMA 播放期间不得释放、改写或放在会退出的栈帧中。repeat 模式下 buffer 必须一直有效到 Stop。源码只校验非空/长度，不自动限制 code 是否超过 DAC 位宽。

## 16. Result Meaning

`GetConfiguredRate` 是每个 DAC code 的更新率，不一定是输出波形频率。one-shot finished 表示最后一次 DMA done ISR 已停止链路；repeat 模式的正常状态不是 finished。

## 17. Common Mistakes

- 把 count 当字节数。
- 传入局部栈数组后立即返回。
- repeat=true 却等待 one-shot finished。
- DDS 使用请求更新率，而 Timer 实际配置率略有量化误差。
- DAC/DMA channel 与 ADC 或其他应用冲突。
- code 超 DAC 满量程或外部负载超驱动能力。
- 手改生成的 `ti_msp_dl_config.*`。

## 18. Verification

先播放两点或短斜坡表，示波器检查 DAC pin 有更新；再播放已知 N 点正弦，验证 `f_out`、Vpp、offset 和 one-shot/repeat 行为。软件 Build 成功不等于模拟幅值或带宽已实板验证。

## 19. Realistic Example

```c
SignalDDS_Init(&dds, sine_table, TABLE_N, 1000.0f,
    (float)SignalDACPlatform_GetConfiguredRate(), 0U);
SignalDDS_Fill(&dds, dac_codes, BLOCK_N);
SignalDACDMA_Start(&player, dac_codes, BLOCK_N, true);
```

## 20. Quick Modify Table

| 我想改什么 | 去哪里 | 改什么 | 影响 | SysConfig? |
|---|---|---|---|---|
| 更新率 | Platform Init/Application config | `update_rate_hz` | 输出频率刻度、Timer 负载 | 通常否，越界/改时钟时是 |
| 表长 | code buffer + Start | `count` | RAM、循环周期 | 否 |
| 连续/单次 | Start | `repeat` | 完成行为 | 否 |
| 波形频率/幅值/相位 | DDS/Wave Table config | 对应参数 | code 内容 | 否 |
| DAC pin/reference | `.syscfg` | DAC route/reference | 物理输出 | 是 |
| DMA channel | `.syscfg` | channel/event | 资源冲突 | 是 |

## Integration Closure

- `SignalDACDMA_Init()` 需要的 start/stop callback 由唯一正式源码 `../signal_dac_dma_platform.c/.h` 提供，用户不需要在 `main.c` 重写 DMA/Timer/Event glue。
- 真实闭环为 `DDS/Wave Table -> DAC DMA wrapper -> DAC DMA Platform Adapter -> DriverLib/SysConfig`。
- `dac_dma_minimum` 已完成 SysConfig、compile 和 final link；当前是 `BUILD_VERIFIED`，未做开发板验证。

## Copy Into Target Project

链接 `06_generator/dac_dma/signal_dac_dma.c`、`08_applications/common/signal_dac_dma_platform.c` 及上游波形模块；使用 `PROFILE_03_DAC_GENERATOR` 作为 SysConfig 起点。Include Path 加入上述正式源码目录、`01_bsp/common` 和 SysConfig 生成目录，不复制源码。

## Hardware / Platform Binding

- 本目录只保存说明；唯一实现是父目录 `signal_dac_dma_platform.h/.c`。
- This platform is used by：[DAC DMA README](../../../06_generator/dac_dma/README.md)。
- SysConfig：`PROFILE_03_DAC_GENERATOR`。
- 【COMPILE-VERIFIED EXAMPLE】：[`dac_dma_minimum/main.c`](../../../09_examples/platform_closure/dac_dma_minimum/main.c)
