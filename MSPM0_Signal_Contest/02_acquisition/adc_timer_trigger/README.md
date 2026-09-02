# adc_timer_trigger

## 你真的需要这个模块吗？

**新工程通常不需要。** 它是旧组合层，只描述定时触发而不交付采样 buffer。要按固定 `Fs` 得到 `raw[N]`，默认直接使用 ADC DMA；只有维护旧工程时才按本 README 接入。

## 30 秒拼装路线

1. 本模块是旧组合层；新工程要固定 Fs 的 `raw[N]` 默认直接使用 ADC DMA。本目录源文件主要用于维护旧工程；比赛冻结副本只有在明确选择本 Legacy 模块时才复制。
2. 维护时还需 `[LINK]` BSP ADC/Timer 与 `08_applications/common/mspm0g3507/signal_mspm0g3507_platform.c`；`[GENERATED]` 为目标 SysConfig。
3. SysConfig 需要 ADC、Timer、合法 ADC Pin；旧例对照 P01，真正 N 点 DMA 仍按 ADC DMA 教程。
4. `SignalMSPM0G3507_ADC_Bind/Timer_Bind → SignalADCTimerTrigger_Init → Start/Stop`。
5. 结果只有 `configured_trigger_rate_hz` 和状态；没有 sample buffer。
6. Clean → Build；最小例能启停但不等于完成采集功能。

## CCS SysConfig GUI Configuration

### Required resources

需要 `ADC12` + `TIMER`。本模块是旧组合层，不包含 DMA；`ADC12`/`TIMER` 是 SysConfig module，具体 `ADC0`/`TIMG0` 等是硬件 instance，`DL_ADC12_*`/`DL_TimerG_*` 是 DriverLib C 名称。

### Step 1 - ADC12 and Event

GUI Path: `ADC12` -> `ADC Conversion Memory Configurations` -> `ADC Conversion Memory 0 Configuration`。选择合法 input channel/pin、12-bit、manual power-down 和已验证 sample time；触发选择 Event，并将 `Event Subscriber Channel ID` 与 Timer publisher 设为同一 channel。没有 DMA buffer，不要无条件打开 DMA trigger。

### Step 2 - Timer Clock Configuration

GUI Path: `TIMER` -> `Basic Configuration` -> `Clock Configuration`。设置 `Timer Clock Source`、`Timer Clock Divider`、`Timer Clock Prescaler` 和 `Desired Timer Period`；P01 对照值为 `BUSCLK / 1 / 1 / 10 us`，不是所有工程的默认值。共享[时钟教材](../../00_docs/MSPM0_SYSCONFIG_CLOCK_TIMER_ADC_DAC_BEGINNER_GUIDE.md)说明如何由 GUI 实际 clock/period 回算 `configured_trigger_rate_hz`。

### Step 3 - Verification

GUI Path: `TIMER` -> `Event Configuration` -> `Event 1 Publisher Channel ID`；`ADC12` -> `Event Configuration` -> `Event Subscriber Channel ID`。保存 Generate 后核对目标 instance 的 generated symbols；本模块没有 DMA symbol，也不产生 `raw[N]`。

### Final checklist / Common mistakes / Do not change

- Publisher/subscriber channel 相同，ADC conversion budget 覆盖事件间隔。
- 应用中的 `configured_trigger_rate_hz` 等于 GUI 实际 Timer 计数时钟推导值。
- 不把本模块误当 ADC DMA；不直接编辑 `.syscfg`/生成文件；保存后必须重新 Generate 并核对 ADC、Timer、Event 的生成宏。

## 第一次把本模块加入母版工程

### STEP 1～4：文件、CCS、SysConfig 与参数

- 维护现有 Application 时 `[LINK]` `signal_adc_timer_trigger.c`、`01_bsp/adc/signal_adc.c`、`01_bsp/timer/signal_timer.c`、正式 MSPM0 platform；公共头 `signal_status.h`。旧 projectspec 使用 `${MSPM0_SIGNAL_LIBRARY_ROOT}`，这不是新比赛母版的默认方式。
- 新比赛工程若明确选择本 Legacy 模块，则把上述必要源码/头文件冻结复制到 `modules/` 并记录来源；`[GENERATED]` 为目标 `ti_msp_dl_config.*`；`[REFERENCE ONLY]` 为 `09_examples/platform_closure/adc_timer_trigger_minimum`。
- 在 CCS 中双击 `.syscfg`，通过 SysConfig 图形界面配置 ADC Pin/channel 和 periodic Timer；instance 改名后同步 platform 绑定宏。`requested_rate_hz`、Timer clock/max count 是经常改项。不要直接编辑 `.syscfg` 文本或生成的 `ti_msp_dl_config.*`。

### STEP 5～10：main、调用、结果与连接

可编译完整代码直接看上面的 minimum example；关键顺序是：

```c
SYSCFG_DL_init();
SignalMSPM0G3507_ADC_Bind(&adc, &adc_context, 2U, 12U,
    SIGNAL_ADC_ADCMEM_0_REF_VOLTAGE_V, CPUCLK_FREQ);
SignalMSPM0G3507_Timer_Bind(&timer, SIGNAL_SAMPLE_TIMER_INST,
    CPUCLK_FREQ, 65536U);
SignalADCTimerTrigger_Init(&trigger, &timer, &adc_context,
    SignalMSPM0G3507_ADC_Enable, SignalMSPM0G3507_ADC_Disable, 100000U);
SignalADCTimerTrigger_Start(&trigger);
configured_rate_hz = trigger.configured_trigger_rate_hz;
SignalADCTimerTrigger_Stop(&trigger);
```

Bind 把生成实例交给旧抽象；Init 计算/设置 Timer rate；Start 先 arm ADC 再启 Timer；Stop 反向停机。`configured_rate_hz` 是配置值，不是实测 Fs，也没有 `raw[]`。下游真正测量应改链为 ADC DMA→raw→ToVoltage。

### STEP 11～12：Build 与最小验证

保存 SysConfig → Clean → Build。header not found=旧 BSP/platform include 缺失；undefined symbol=少链接上述 `.c`；实例宏不存在=Profile/实例名不匹配。最小验证只看返回码和 configured rate；需要数据时停止使用本模块，切 ADC DMA。

## 比赛现场最常改的地方

通常不选它作为新题入口；维护旧例时只改 requested rate、ADC Pin、Timer instance/clock。不要把它误认为 ADC DMA。

## 从母版到成功调用：完整例子

`09_examples/platform_closure/adc_timer_trigger_minimum/main.c` 是当前 API 的完整 full-link 例。旧工程可沿用其 linked-source projectspec；新比赛母版只参考其源文件清单和 P01 SysConfig 契约，采用冻结复制。

## MSPM0G3507 比赛推荐方式

本模块只组合旧 BSP Timer/ADC callback，没有提供 DMA buffer 或完整 N 点采集，**新比赛工程通常不作为入口**。简单 Timer/ADC 动作直接 DriverLib；要固定 Fs 的一帧数据直接使用 ADC DMA。源码暂保留以兼容现有最小例子。

## 0. 什么时候用

仅用于没有完整波形 buffer 的单 ADC Timer/ADC 启停顺序。24_C 双 ADC、猝发 marker、连续 DMA 和完整 raw[N] 采集应选择 `adc_dual_sync`，不要把本 Legacy 模块当作 DMA 采集模块。

## 1. 模块作用

按安全顺序组合 Timer 与 ADC 的 arm/start/stop。

24_C 双 ADC、猝发 marker、连续 DMA 和完整 raw[N] 采集应选择 `adc_dual_sync`；本 Legacy 模块不拥有 DMA 波形 buffer。

## 2. 输入

输入由公开头文件中的指针、长度、配置结构或平台回调给出；所有单位写在字段名中。

## 3. 输出

输出写入调用者提供的结果/缓冲区，并返回 `signal_result_t`；不通过隐藏全局变量传递数据。

## 4. 依赖

`signal_status.h`、`signal_timer.h`。

## 5. SysConfig 设置

新手详细配置：[Timer 定时触发 ADC](../../00_docs/SYSCONFIG_BEGINNER_GUIDE.md#adc)。本模块重点检查 Timer ZERO_EVENT Publisher、ADC Event Subscriber 使用同一 Channel ID，以及 ADC Trigger Source=Event。现场速查见 [Quick Reference](../../00_docs/SYSCONFIG_QUICK_REFERENCE.md)。

通用代码不绑定 SysConfig 实例名；接到 MSPM0 时由独立平台适配器使用生成宏。当前状态不代表对应外设已实板验证。

Profile 只作为图形配置参考。保存 SysConfig 后核对 Timer 页的 Calculated Clock；只有 Timer 确为 BUSCLK/1/1 时，示例中的 `CPUCLK_FREQ` 才能作为 Timer clock。若输入信号为 10 Hz，但 ADC 仍需 1 kHz 采样，则 `requested_rate_hz` 应为 1 kHz，不是 10 Hz；低频能否分析取决于采样窗口 `N/Fs`。

## 6. 初始化方法

模块不做隐式全局初始化。包含 `signal_adc_timer_trigger.h`，由调用者准备配置、缓冲区或平台回调；如头文件提供 Init/Configure，先调用它。

## 7. 调用方法

`SignalADCTimerTrigger_Init`、`SignalADCTimerTrigger_Start`、`SignalADCTimerTrigger_Stop`、`SignalADCTimerTrigger_GetModuleStatus`。

## 8. 参数修改方法

只修改调用者配置结构、count/capacity 和采样率等函数参数；不要为某个 Demo 改底层 `.c`。

## 9. 与其他模块如何连接

通过 `signal_types.h` 的数组+长度+采样率语义或本模块公开结构连接；先检查返回码再消费输出。

## 10. 最小示例

~~~c
#include "signal_adc_timer_trigger.h"

/* 按头文件准备输入/输出，调用上述主 API，并检查 signal_result_t。 */
~~~

纳入 `10_tests/pc` 全库构建；关键数值路径还应按题目范围补充向量和误差上限。

## 11. 常见错误

空指针、零长度、capacity 小于 count、单位混用、把配置采样率当物理实测值，以及复用仍在使用的工作区。

## 12. RAM 占用

模块内动态分配 0；数组/工作区由调用者提供，具体大小由 API 的 count/capacity 决定。

## 13. Flash 占用

无固定常量：取决于编译优化、是否链入数学库和死代码删除。已纳入整库链接检查；比赛应用以 CCS 生成的 .map 为最终数据。

## 14. CPU 计算量估计

函数为同步确定性处理；硬件回调的中断上下文只做最小状态更新，重计算放在主循环。

## 15. 当前验证状态

`MODULE_STATUS_BUILD_VERIFIED`。该状态只表示现有证据等级，不等于完整比赛场景已经验证。

## 16. 以后实板验证步骤

Hardware validation: PENDING。在 SysConfig 中按目标引脚/实例完成平台适配，用已知输入验证启停、边界和连续重启，记录变量与实测条件后才可升级 BOARD_VERIFIED。

不使用时，从工程移除本目录 .c 及上层引用；若有平台外设适配，再从 SysConfig 删除对应实例。

## 17. README Usability Upgrade：完整 API

以下内容从正式 `.h` 和现有 README 整理；未公开说明的字段明确标为 `UNKNOWN / NOT EXPOSED`。

### `signal_result_t SignalADCTimerTrigger_Init(signal_adc_timer_trigger_t *module, const signal_timer_t *timer, void *adc_context, signal_trigger_control_fn arm_adc, signal_trigger_control_fn disarm_adc, uint32_t requested_rate_hz);`

- **作用：** UNKNOWN / NOT EXPOSED

| 参数 | 真实类型 | 真实说明 |
|---|---|---|
| `module` | `signal_adc_timer_trigger_t *` | UNKNOWN / NOT EXPOSED |
| `timer` | `const signal_timer_t *` | UNKNOWN / NOT EXPOSED |
| `adc_context` | `void *` | UNKNOWN / NOT EXPOSED |
| `arm_adc` | `signal_trigger_control_fn` | UNKNOWN / NOT EXPOSED |
| `disarm_adc` | `signal_trigger_control_fn` | UNKNOWN / NOT EXPOSED |
| `requested_rate_hz` | `uint32_t` | UNKNOWN / NOT EXPOSED |

- **返回：** UNKNOWN / NOT EXPOSED
- **调用前/后：** UNKNOWN / NOT EXPOSED

```c
signal_result_t result_value = SignalADCTimerTrigger_Init(module, timer, adc_context, arm_adc, disarm_adc, requested_rate_hz);
```

### `signal_result_t SignalADCTimerTrigger_Start(signal_adc_timer_trigger_t *module);`

- **作用：** UNKNOWN / NOT EXPOSED

| 参数 | 真实类型 | 真实说明 |
|---|---|---|
| `module` | `signal_adc_timer_trigger_t *` | UNKNOWN / NOT EXPOSED |

- **返回：** UNKNOWN / NOT EXPOSED
- **调用前/后：** UNKNOWN / NOT EXPOSED

```c
signal_result_t result_value = SignalADCTimerTrigger_Start(module);
```

### `signal_result_t SignalADCTimerTrigger_Stop(signal_adc_timer_trigger_t *module);`

- **作用：** UNKNOWN / NOT EXPOSED

| 参数 | 真实类型 | 真实说明 |
|---|---|---|
| `module` | `signal_adc_timer_trigger_t *` | UNKNOWN / NOT EXPOSED |

- **返回：** UNKNOWN / NOT EXPOSED
- **调用前/后：** UNKNOWN / NOT EXPOSED

```c
signal_result_t result_value = SignalADCTimerTrigger_Stop(module);
```

### `signal_module_status_t SignalADCTimerTrigger_GetModuleStatus(void);`

- **作用：** UNKNOWN / NOT EXPOSED

参数：无。

- **返回：** UNKNOWN / NOT EXPOSED
- **调用前/后：** UNKNOWN / NOT EXPOSED

```c
signal_module_status_t result_value = SignalADCTimerTrigger_GetModuleStatus();
```

## 18. Call Sequence / Connecting / Buffer Rules

```text
SignalADCTimerTrigger_Init -> SignalADCTimerTrigger_Start -> SignalADCTimerTrigger_Stop -> SignalADCTimerTrigger_GetModuleStatus
```

按模块角色选择实际所需 API：Init/Validate 先于 Start/Process/Generate，Get/Is 在执行后，Stop 在取消或退出时。所有返回码先检查；buffer 由调用者创建和持有，capacity 按元素数；运行中的 DMA/回调 buffer 不得并发改写。模块不动态分配。

## 19. Config vs SysConfig / Resources / Verification

- 原 SysConfig 说明：通用代码不绑定 SysConfig 实例名；接到 MSPM0 时由独立平台适配器使用生成宏。当前状态不代表对应外设已实板验证。
- 软件参数和数组长度为 CONFIG ONLY；真实 pin、peripheral、clock、Timer、DMA、Event、IRQ、reference 为 SYSCONFIG REQUIRED。
- RAM 由结构体与调用者 buffer 决定；Flash/Stack 看应用 `.map`。
- 用已知输入验证边界和返回码，再接真实 profile；未实板不得写 BOARD_VERIFIED。

## 20. Quick Modify Table

| 我想改什么 | 去哪里 | 会影响什么 | SysConfig? |
|---|---|---|---|
| count/capacity | Application buffer + API | RAM、处理长度、响应 | 否 |
| rate/frequency | Application config；若为真实外设率再改 profile | 时间轴、Nyquist或输出频率 | 视硬件而定 |
| threshold/gain/offset | 公开 config/API | 灵敏度、量程或偏置 | 通常否；硬件前端变化另算 |
| pin/instance/channel | `.syscfg` | 物理连线和资源冲突 | 是 |

常见错误：不检查返回码、byte/element 混用、对象生命周期不足、把请求速率当配置/实测速率、修改硬件资源后未重新生成 SysConfig。

## Integration Closure

`arm_adc/disarm_adc` callbacks 存在是为了让模块按“先 arm ADC、再 start Timer”的安全顺序控制不同平台。MSPM0G3507 正式绑定使用统一平台层：Timer 由 `SignalMSPM0G3507_Timer_Bind()` 创建；ADC context 使用 `signal_mspm0g3507_adc_context_t`；arm/disarm 直接传 `SignalMSPM0G3507_ADC_Enable/Disable`。用户不写 callback。

SysConfig 必须把 ADC trigger 设为 Event，并让 Timer publisher channel 与 ADC subscriber channel 相同；参考 `PROFILE_01_ADC_CAPTURE`。本模块只负责启停 trigger，不拥有采样 buffer；需要完整 N 点 raw buffer 时默认直接使用 ADC DMA。

## Copy Into Target Project

链接 ADC Timer Trigger、BSP Timer、System Clock、BSP ADC 和统一平台 `.c`；加入相应 Include Path。调用 `SignalADCTimerTrigger_Init(&module, &timer, &adc_context, SignalMSPM0G3507_ADC_Enable, SignalMSPM0G3507_ADC_Disable, Fs)` 后再 Start/Stop。`configured_trigger_rate_hz` 是由整数 Timer count 得到的实际配置率。

## Hardware / Platform Binding

- Platform：[MSPM0G3507 Platform Adapter](../../08_applications/common/mspm0g3507/README.md)。
- Timer 由 `SignalMSPM0G3507_Timer_Bind` 构造；arm/disarm 直接绑定 `SignalMSPM0G3507_ADC_Enable/Disable`，用户不写 callback。
- Platform 文件：`signal_mspm0g3507_platform.h/.c`；SysConfig：`PROFILE_01_ADC_CAPTURE`。
- 【COMPILE-VERIFIED EXAMPLE】：[`adc_timer_trigger_minimum/main.c`](../../09_examples/platform_closure/adc_timer_trigger_minimum/main.c)

## 21. 24_C 成功案例中的替代关系

24_C 的正式采集入口没有使用本 Legacy 组合层来承载波形 buffer，而是使用 `adc_dual_sync`：一个 Timer Event 同步触发 ADC0（模拟波形）和 ADC1（猝发标志），两个 DMA 进入三缓冲。需要完整 `raw[N]`、双路同步或连续 DMA 时，复制并配置 `02_acquisition/adc_dual_sync`，不要把本模块误当成 DMA 采集模块。

只需要一次 ADC 转换或启停顺序，且没有波形 buffer 时，才使用 `SignalADCTimerTrigger_*`。同一工程不要同时启用本模块的独立 ADC/Timer trigger 和 `adc_dual_sync` 的公共 Timer，否则会重复占用 ADC、Timer、Event 或 DMA 资源。

### 24_C 的时基核对方法

不要把请求的 Fs 或 `CPUCLK_FREQ` 直接当作真实采样率。保存 SysConfig 后读取 Timer 的实际计数时钟、分频和 `LOAD`，用整数计数得到 `configured_trigger_rate_hz`，再用 `N/Fs` 检查波形窗口是否覆盖目标周期。100 Hz 附近若窗口过短，FFT/H1 会因只含很少周期而不稳定；采样率和 FFT 长度应与上游双 ADC 配置一起核对。
