# Dual ADC Platform Adapter 使用说明

> 正式源码：`../signal_dual_adc_platform.c`、`../signal_dual_adc_platform.h`。本目录不保存第二份源码。

代码分类：本 README 中未标为 `【COMPILE-VERIFIED APPLICATION】` 的代码块均为 `【ILLUSTRATIVE SNIPPET】`；完整真实调用以 `08_applications/dual_channel_phase_meter/main.c` 为准。

## 第一次使用 Dual ADC Platform Adapter？从这里开始

目标：“让 ADC0 和 ADC1 由同一个 Timer 时刻触发，各自 DMA 到独立 `uint16_t channel_a[N]` / `channel_b[N]`”。这是当前 P02 的硬件平台实现。

### STEP 1：加入工程

- 唯一正式源码/头文件：`MSPM0_Signal_Contest/08_applications/common/signal_dual_adc_platform.c/.h`
- 公共依赖：`MSPM0_Signal_Contest/01_bsp/common`
- 生成依赖：目标工程 `ti_msp_dl_config.h`
- Include Path：`08_applications/common`、`01_bsp/common`、SysConfig 生成目录

本说明目录不存第二份源码；projectspec 链接上一级正式 `.c`。

### STEP 2：include

```c
#include "ti_msp_dl_config.h"
#include "signal_dual_adc_platform.h"
```

### STEP 3：变量

```c
static uint16_t raw_a[N];
static uint16_t raw_b[N];
```

平台直接写两块独立数组，不是 A/B 交织格式，因此不要再调用 Deinterleave。

### STEP 4：参数

`sample_rate_hz` 与 `timer_clock_hz` 共同得到整数 Timer count；`sample_count` 范围 1..65535。N 增大时两块 raw 合计每点对 4 B。timer clock 写错会导致两路 Fs 同比例错误。

### STEP 5：SysConfig

**【需要 SysConfig】**。`PROFILE_02_DUAL_ADC` 只用于对照字段；在 CCS 中双击目标工程 `.syscfg`，使用 SysConfig 图形界面配置，不直接编辑 `.syscfg` 文本或生成的 `ti_msp_dl_config.*`：

1. `SIGNAL_ADC_A`=ADC0/PA25/channel2，DMA_CH0。
2. `SIGNAL_ADC_B`=ADC1/PA17/channel2，DMA_CH1。
3. 两路都使用 Event trigger、MEM0 DMA、DMA-done IRQ。
4. `SIGNAL_DUAL_ADC_TIMER`=TIMG0/periodic；ZERO_EVENT 分别发布到 channel 1/2。
5. 检查两路 sample time、Fs、N 对称，PinMux/DMA/Event 无冲突。
6. 保存后等待 CCS 自动重新生成，确认头文件仍有 `SIGNAL_ADC_A_*`、`SIGNAL_ADC_B_*`、`SIGNAL_DUAL_ADC_TIMER_*` 宏；不要手改生成文件。

测 10 Hz 相位时，默认 `100 kHz × 512/1024点` 的窗口不足一个周期。应按 `frame_time=N/Fs` 让双路窗口覆盖多个最低频率周期，例如从 `Fs=1 kHz、N=1024` 开始；ADC Timer 设为采样率 1 kHz，而不是输入频率 10 Hz。改变 Timer Clock Source/Divider 后，平台配置中的 `timer_clock_hz` 必须使用图形页显示的真实计数时钟。

### STEP 6：初始化

```c
SYSCFG_DL_init();
(void)SignalDualADCPlatform_Init(100000U, 32000000U);
```

### STEP 7：真正调用

```c
if (SignalDualADCPlatform_Start(raw_a, raw_b, N) == SIGNAL_RESULT_OK) {
    while (!SignalDualADCPlatform_IsFinished()) { __WFE(); }
}
```

两路都 DMA done 才 finished；完成前不消费任一路整帧。

### STEP 8：结果

结果在 `raw_a/raw_b`，均为 ADC code；配置触发率由 `SignalDualADCPlatform_GetConfiguredRate()` 返回。两路电压比例可分别转换。

### STEP 9：连接

```text
Dual ADC Platform -> raw_a/raw_b -> 两次 ADC To Voltage -> Dual Phase
```

```c
(void)SignalADCToVoltage_Process(raw_a, a_v, N, &scale_a);
(void)SignalADCToVoltage_Process(raw_b, b_v, N, &scale_b);
```

### STEP 10：Build

`ti_msp_dl_config` 宏缺失=Profile/实例名不对；undefined platform symbol=漏链接正式 `.c`；重复 IRQ=别的源码也定义同名 ADC ISR；SysConfig conflict=ADC/DMA/Timer/Event 被占用；SRAM overflow=双 raw+双 float+双 FFT 太大。

### STEP 11：验证

先给两路相同稳定电压，检查 raw_a/raw_b 都更新、N 一致、finished 正常；再输入同源同相波形检查相位约 0°。同步 skew/前端延迟仍需实板校准。

### STEP 12：常见修改

1. N 512→1024：两块 raw 总增 2048 B；双 float/FFT 还会继续增加，full link 看 `.map`。
2. Fs 100 k→200 k：改 Init 参数并同步下游 Fs；确认 ADC sample time/吞吐。
3. 换任一路通道：只改 P02 `.syscfg` 和该路换算配置。
4. 不需要双通道：改用单通道 ADC DMA，释放 ADC1/DMA1/RAM。

### STEP 13：完整最小示例

```c
#include "ti_msp_dl_config.h"
#include "signal_dual_adc_platform.h"
static uint16_t a[512], b[512];
void Acquire(void)
{
    SYSCFG_DL_init();
    (void)SignalDualADCPlatform_Init(100000U, 32000000U);
    if (SignalDualADCPlatform_Start(a, b, 512U) == SIGNAL_RESULT_OK) {
        while (!SignalDualADCPlatform_IsFinished()) { __WFE(); }
    }
}
```

下面是平台 API、ISR、资源、Buffer 规则和验证证据。

## 1. What It Does

用一个 Timer 同时触发两套 ADC/DMA，把同一帧的 A、B 两路原始码分别写进两块 `uint16_t` buffer。

小白理解：它把 SysConfig 生成的具体 ADC、DMA、Event 和中断宏藏在平台层，上层只需提供两块数组并等待两路都完成。

## 2. When To Use It

用于双通道相位、增益比较或同步波形分析。只有单通道时用 ADC DMA；若输入本来是 A/B 交织数组而不是两套 DMA，使用 Dual ADC Sync Deinterleave。

## 3. Where It Sits In The Signal Chain

```text
CH-A analog -> ADC-A -> DMA-A -> uint16_t raw_a[N] --+
Timer/Event                                            +-> ToVoltage -> Phase
CH-B analog -> ADC-B -> DMA-B -> uint16_t raw_b[N] --+
```

## 4. Inputs / Outputs

- 输入配置：每通道采样率 `sample_rate_hz`、Timer 真实计数时钟 `timer_clock_hz`。
- 输入 buffer：调用者创建 `uint16_t channel_a[N]`、`channel_b[N]`。
- 输出：两块独立 raw buffer、完成布尔值、整数 Timer 配置推导的实际触发率。
- buffer 在两路完成前由 DMA 写，应用不得读写或释放。

## 5. Dependencies

Required：`signal_status.h`、DriverLib、SysConfig 生成的 `ti_msp_dl_config.h`，以及 P02/P06 所示的双 ADC、两 DMA、共享 Timer/Event/IRQ。应用只链接本适配器的唯一 `../signal_dual_adc_platform.c` 和公共 status 源；DriverLib/生成文件由工程提供。

## 6. Public API Reference

### `SignalDualADCPlatform_Init(sample_rate_hz, timer_clock_hz)`

- 两参数均为 `uint32_t` Hz，必须非 0 且采样率不大于 Timer 时钟。
- 计算 `round(timer_clock_hz/sample_rate_hz)`；计数必须为 `1..65536`。
- 停 Timer、写 load/count、清状态并使能两路 ADC IRQ。
- 成功 `SIGNAL_RESULT_OK`；非法参数或计数越界返回对应错误。`SYSCFG_DL_init()` 必须已经调用。

### `SignalDualADCPlatform_Start(channel_a, channel_b, sample_count)`

- 两个指针都必须非空；每块容量至少 `sample_count` 个 `uint16_t`。
- `sample_count` 是 `uint16_t` 且必须非 0。
- 重装两路 DMA 目的地址/长度，清标志，使能 ADC/DMA 并启动共享 Timer。
- 未 Init 返回 `NOT_INITIALIZED`；硬件仍忙返回 `BUSY`；成功后两路 DMA 拥有 buffer 写权限。

### `SignalDualADCPlatform_IsFinished()`

两路 DMA done ISR 都到达才返回 `true`。只有此时才可把两块 buffer 交给算法。

### `SignalDualADCPlatform_Stop()`

立即停止 Timer、两路转换与两 DMA。无返回值；可用于完成后收尾或异常退出。

### `SignalDualADCPlatform_GetConfiguredRate()`

返回 `timer_clock_hz / rounded_timer_count`，单位 Hz。它是配置推导值，不是仪器实测采样率；Init 前为 0。

## 7. Call Sequence

```text
SYSCFG_DL_init -> Init -> Start -> 等 IsFinished -> 读 A/B -> Stop
                                      `-> 下一帧 Start
```

## 8. Minimal Example

```c
static uint16_t raw_a[1024];
static uint16_t raw_b[1024];
SYSCFG_DL_init();
if (SignalDualADCPlatform_Init(100000U, CPUCLK_FREQ) == SIGNAL_RESULT_OK &&
    SignalDualADCPlatform_Start(raw_a, raw_b, 1024U) == SIGNAL_RESULT_OK) {
    while (!SignalDualADCPlatform_IsFinished()) { __WFI(); }
    uint32_t actual_fs_hz = SignalDualADCPlatform_GetConfiguredRate();
}
```

## 9. Connecting To Other Modules

```text
raw_a -> ADC To Voltage -> voltage_a --+
raw_b -> ADC To Voltage -> voltage_b --+-> FFT Phase / Correlation Phase
```

真实调用参考 `dual_channel_phase_meter/main.c`、`signal_analyzer/main.c`、`signal_contest_template/signal_pipeline.c`。

## 10. Parameter Guide

| 参数 | 默认值 | 增大 | 减小 | RAM | SysConfig |
|---|---|---|---|---|---|
| `sample_rate_hz` | Application 定义 | Timer 周期变短、每秒数据更多 | 观察同样 N 更久 | 不直接改变 | 改率通常不需重画资源，但必须满足现有 Timer/ADC 能力 |
| `sample_count` | Application 定义 | 每帧更长，两块 raw RAM 增加 | 更快完成 | 每增加 1 点总 raw 增 4 bytes | 否 |
| `timer_clock_hz` | 真实 Timer 计数时钟 | 可得到更细的整数分频 | 可能限制最高 Fs | 否 | 时钟树变化时是 |

## 11. Common Modification Tasks

- N 从 512 改 1024：两块数组和 Start 参数都改，raw 总 RAM 从 2048 增到 4096 bytes；不改 SysConfig。
- 改两路 ADC 引脚/通道：修改 P02/P06 派生 `.syscfg` 的两个 ADC MEM/pin，并核对生成宏；需要 SysConfig。
- 改 Fs：先确认 Timer 实际计数时钟，再改 Init 参数；使用 GetConfiguredRate 传给算法。

## 12. Config vs SysConfig

CONFIG ONLY：N、请求 Fs、等待策略、算法开关。SYSCONFIG REQUIRED：ADC A/B 实例/通道/引脚、两 DMA 通道、共享 Timer/Event、IRQ 和时钟树。

## 13. SysConfig Setup

新手逐项配置：[ADC + Timer + Event + DMA 教程](../../../00_docs/SYSCONFIG_BEGINNER_GUIDE.md#adc-timer-dma)。在单链基础上对照 P02 添加 ADC1、独立 DMA 和 Event 2；本 Adapter 输出两块独立 buffer，不是 interleaved。现场速查见 [Quick Reference](../../../00_docs/SYSCONFIG_QUICK_REFERENCE.md)。

参考 `09_examples/integration_profiles/PROFILE_02_DUAL_ADC/profile.syscfg` 或 `PROFILE_06_FULL_SIGNAL/profile.syscfg`。需要双 ADC、两 DMA channel、同一 Timer 发布事件、两 ADC 订阅及两个 DMA-done IRQ。源码依赖生成宏 `SIGNAL_DUAL_ADC_TIMER_INST`、`SIGNAL_ADC_A_*`、`SIGNAL_ADC_B_*`；生成名必须与 profile 一致。

## 14. Resources / Memory

独占/共享资源：1 Timer、2 ADC、2 DMA、Event 路由、2 ADC IRQ、2 analog pins。调用者 raw RAM 为 `4N` bytes；适配器只保存少量状态。

## 15. Buffer Rules

两块 buffer 必须不同、可写、各至少 N 项；源码没有显式检测重叠，调用者必须避免别名。DMA 完成前不能读；下一次 Start 会覆盖旧数据。单位是 raw code，不是 V。

## 16. Result Meaning

第 i 个 `channel_a[i]` 与 `channel_b[i]` 由同一共享触发节拍采得，但模拟采样孔径、ADC 实例固定延迟和前端延迟仍可能形成相位偏差，需要校准。

## 17. Common Mistakes

- 只等一路完成就处理。
- 两路 buffer 指向同一数组。
- 两 DMA 通道或 Event 路由冲突。
- 把 `CPUCLK_FREQ` 当 Timer 计数时钟而实际有分频。
- ADC A/B 通道或 pin 配反。
- 把配置率当作外部实测率或忽略通道固定延迟。

## 18. Verification

先让两路接同一稳定信号：检查两块 buffer 都变化、完成标志稳定、均值/幅值接近；再输入已知相位差验证 B-A 符号。未实板执行前只记录 build/source 状态。

## 19. Realistic Example

```c
while (!SignalDualADCPlatform_IsFinished()) { __WFI(); }
SignalIntegration_RawToVoltage(raw_a, N, 12U, 3.3f, 1.0f, 0.0f, va, N);
SignalIntegration_RawToVoltage(raw_b, N, 12U, 3.3f, 1.0f, 0.0f, vb, N);
SignalIntegration_DualPhase(va, vb, N,
    (float)SignalDualADCPlatform_GetConfiguredRate(), signal_hz,
    max_lag, fft_a, fft_b, N, corr, 2U * max_lag + 1U, &phase);
```

## 20. Quick Modify Table

| 我想改什么 | 去哪里 | 改什么 | 影响 | SysConfig? |
|---|---|---|---|---|
| N | Application config/buffers | 两块数组与 Start 参数 | RAM/帧长 | 否 |
| Fs | Init call/Timer config | `sample_rate_hz`、真实 `timer_clock_hz` | 时间刻度 | 仅改时钟/资源时是 |
| A/B 通道 | `.syscfg` | 两 ADC channel/pin | 输入来源 | 是 |
| DMA 通道 | `.syscfg` | A/B DMA channel | 资源冲突 | 是 |
| 相位校准 | Algorithm/Application | channel delay correction | 结果偏置 | 否 |

## Integration Closure

- 双 ADC 的 ADC/DMA/Timer/Event/ISR glue 只由唯一正式源码 `../signal_dual_adc_platform.c/.h` 提供；输出是两块独立 `uint16_t raw_a[N]`、`raw_b[N]`。
- Application 不需要交织/拆分循环；两路都完成后才能消费整帧。
- `PROFILE_02_DUAL_ADC` 已完成 SysConfig、compile 和 final link；当前是 `BUILD_VERIFIED`，未做开发板验证。

## Copy Into Target Project

链接 `02_acquisition/adc_dual_sync/signal_adc_dual_sync.c` 与 `08_applications/common/signal_dual_adc_platform.c`，使用 `PROFILE_02_DUAL_ADC` 作为 SysConfig 起点。Include Path 加入两者目录、`01_bsp/common` 和 SysConfig 生成目录，不复制源码。

## Hardware / Platform Binding

- 本目录只保存说明；唯一实现是父目录 `signal_dual_adc_platform.h/.c`。
- 上层纯软件整理模块：[ADC Dual Sync README](../../../02_acquisition/adc_dual_sync/README.md)。
- SysConfig：`PROFILE_02_DUAL_ADC`。
- 【COMPILE-VERIFIED APPLICATION】：`08_applications/dual_channel_phase_meter/main.c`，Round 1 full link 回归目标 `dual_channel_phase_meter`。
