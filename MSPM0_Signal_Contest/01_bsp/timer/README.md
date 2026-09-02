# timer

## CCS SysConfig GUI Configuration

### Required resources

本模块的 SysConfig module 名称是 `TIMER`；`TIMG0`、`TIMG6` 等是硬件 Timer instance；`DL_TimerG_*` 是 DriverLib C API 名称，三者不要混用。基础定时只需要 `TIMER`，向 ADC/DAC/Capture 发布硬件事件时还需要对应的 `EVENT`、ADC/DAC/CAPTURE module。

### Step 1 - Add the TIMER module

GUI Path: `Add` -> `TIMER` -> 新建实例。

Action: 在 `Basic Configuration` 中选择 `Timer Peripheral`。P01 的已验证硬件 instance 为 `TIMG0`，P03 为 `TIMG6`；实例名由当前工程决定，不能因为 DriverLib 名称而强制改名。

### Step 2 - Timer clock configuration

GUI Path: `TIMER` instance -> `Basic Configuration` -> `Clock Configuration`。

Set（P01/P03 已验证基线）：`Timer Clock Source = BUSCLK`、`Timer Clock Divider = 1`、`Timer Clock Prescaler = 1`、`Timer Mode = Periodic Down Counting`、`Desired Timer Period = 10 us`。保存后读取 GUI 显示的实际/计算 Timer clock 与 `Actual Timer Period`；Timer event rate 是周期的倒数，不是 Timer clock 本身。

共享教材：[MSPM0G3507 SysConfig 时钟、Timer、ADC 与 DAC 保姆教程](../../00_docs/MSPM0_SYSCONFIG_CLOCK_TIMER_ADC_DAC_BEGINNER_GUIDE.md)。其中的 SYSCTL Clock Tree 只作为系统时钟证据，不能用 `CPUCLK_FREQ` 代替 GUI 显示的最终 Timer counter clock，除非当前 `BUSCLK/divider/prescaler` 已核对一致。

### Step 3 - Event and interrupt (when needed)

GUI Path: `TIMER` instance -> `Event Configuration` / `Interrupts Configuration`。

Action: 需要硬件触发时，在 `Event 1 Publisher Channel ID` 写入与接收方相同的 channel；P01 使用 channel `1`，P03 使用 channel `3`。需要周期中断时才启用 `Event 1 Enable Controller Interrupts` 中的 `ZERO_EVENT`；不用事件或 IRQ 时不要照搬 profile。

### Expected generated symbols

Generate 后在 `ti_msp_dl_config.h` 核对目标实例宏（例如 `SIGNAL_SAMPLE_TIMER_INST` 或 `SIGNAL_DAC_TIMER_INST`）、`*_LOAD_VALUE`、`*_IRQN` 以及 Event 相关宏。PROJECT_AUDIT 必须记录 `GUI field -> .syscfg property -> generated symbol`，而不是只看 `DL_TimerG_*`。

### Final checklist

- Timer module、实际 `TIMGx` instance 和 DriverLib 名称已分开记录。
- Clock Source -> Divider -> Prescaler -> 实际计数 clock -> Period 的链路闭合。
- 需要硬件触发时 publisher channel 与 subscriber channel 相同。
- `Actual Timer Period` 与应用使用的 rate/tick 换算一致。
- 保存 `.syscfg` 后 CCS 已重新生成，资源冲突视图无冲突。

### Common mistakes

- 把 `TimerG`/`DL_TimerG_*` 当成 SysConfig module 名称。
- 把 BUSCLK 或 CPUCLK 当成事件频率。
- 只改 Timer 周期，却忘了同步应用中的真实 `timer_clock_hz`。
- 无证据地改用 LFCLK/LFXT 或创建新的 Event channel。

### Do not change

不要直接编辑 `.syscfg` 文本或 `ti_msp_dl_config.c/.h`；不要在当前工程已有 Timer、Event、Pin 资源时照搬 profile 的 instance、channel 或引脚。

## 什么时候用它（先做 Resource Check）

- **优先使用**：固定采样/更新节拍、输入捕获、PWM，以及 Timer Event → ADC/DAC 的低抖动硬件链。
- **慎用**：极低频/极高频测量；先算 Timer clock、divider、tick 分辨率、位宽和溢出。
- **不要强行使用**：所需精度已超过时钟准确度，或实例/Event/PinMux 与现有采集、DAC、Capture 明确冲突；先重做资源分配或外加时基。

芯片 Timer 数量和系统能力见 [MSPM0G3507 资源能力指南](../../00_docs/MSPM0G3507_RESOURCE_CAPABILITY_GUIDE.md)；内部/外置判断见 [内部/外置选择指南](../../00_docs/INTERNAL_VS_EXTERNAL_SELECTION_GUIDE.md)。

## 你真的需要这个模块吗？

**Timer 启停、读 counter 等基础动作不需要旧 wrapper。** 新工程直接用 SysConfig + Timer DriverLib；只有 ADC 定时 DMA、完整 Capture 等多资源流程才选对应复杂模块。见 [TI DriverLib 初学者指南](../../00_docs/TI_DRIVERLIB_BEGINNER_GUIDE.md)。

## 30 秒拼装路线

1. 新工程不链接旧 signal_timer.c/.h；它是 [REFERENCE ONLY]。简单 Timer 使用 SysConfig + DriverLib。
2. [GENERATED] ti_msp_dl_config.*；在 CCS 中双击 `.syscfg`，通过 SysConfig 图形界面添加 TimerG 并选择 clock/divider/prescaler/period/mode/event/IRQ；不直接编辑 `.syscfg` 文本或生成文件。
3. SYSCFG_DL_init() 后 DL_TimerG_startCounter()；需要时读 DL_TimerG_getTimerCount()、再 stop。
4. P01 的 SIGNAL_SAMPLE_TIMER_INST 是 TIMG0/periodic，并发布 Event1；不需要 event 时不要照搬。
5. 结果 uint32_t count 单位是 timer tick，不是秒；换算需真实 timer clock。
6. Clean → Build；先看计数器变化，再接 ADC DMA/Capture。

## 第一次把本模块加入母版工程

### STEP 1～4：加入方式、SysConfig 与参数

- [LINK]/[COPY] 无；[GENERATED] SysConfig；[REFERENCE ONLY] P01/P05。
- projectspec 不加 BSP Timer 源。在 CCS SysConfig 图形界面给母版添加 Timer，确定 periodic/one-shot/capture、clock/divider/prescaler/period、Event/IRQ；保存后由 CCS 自动生成。
- 经常改目标 period/rate；换 clock/divider 时同步 tick↔s 和 config 的 timer_clock_hz。Timer/Event 改动必须检查 ADC/DAC/Capture 资源冲突。

### STEP 5～10：main、调用、结果与连接

~~~c
#include <stdint.h>
#include "ti_msp_dl_config.h"
volatile uint32_t g_timer_count;
int main(void)
{
    SYSCFG_DL_init();
    DL_TimerG_startCounter(SIGNAL_SAMPLE_TIMER_INST);
    g_timer_count = DL_TimerG_getTimerCount(SIGNAL_SAMPLE_TIMER_INST);
    DL_TimerG_stopCounter(SIGNAL_SAMPLE_TIMER_INST);
    while (1) { __WFI(); }
}
~~~

结果 g_timer_count 是 tick。连接：Timer Event→ADC DMA；Timer Event→DAC DMA；Comparator Event→Timer Capture。复杂链优先用相应功能模块，不在 main 重拼。

### STEP 11～12：Build 与最小验证

实例宏不存在=SysConfig 名字不同；rate 固定比例错误=clock/divider 误算；Event 无效=publisher/subscriber channel 不一致。保存 SysConfig → Clean → Build；debug 看 count 变化。

## 比赛现场最常改的地方

经常改 rate/period；偶尔改 clock/divider/instance/Event；通常不要手改 generated init，也不要绕过复杂功能模块。

## 从母版到成功调用：完整例子

上面的 main.c 是计数器最小闭环；采样/输出系统继续按 ADC DMA、DAC DMA 或 Timer Capture README 拼接。

## MSPM0G3507 比赛推荐方式

Timer start/stop/read counter 直接使用 `DL_TimerG_startCounter()`、`DL_TimerG_stopCounter()`、`DL_TimerG_getTimerCount()`。本目录 callback/descriptor 对这些基础动作增加了层级，**新工程通常不推荐**。Timer Capture、ADC DMA、DAC DMA 等需要回绕、ISR、Event、DMA 或状态机的完整流程仍使用相应复杂模块。

## 1. 模块作用

统一 Timer 设置周期、启动、停止和读计数接口。

## 2. 输入

输入由公开头文件中的指针、长度、配置结构或平台回调给出；所有单位写在字段名中。

## 3. 输出

输出写入调用者提供的结果/缓冲区，并返回 `signal_result_t`；不通过隐藏全局变量传递数据。

## 4. 依赖

`signal_status.h`。

## 5. SysConfig 设置

新手详细配置：[Timer 时钟、Period、Capture、Interrupt 与 Event 教程](../../00_docs/SYSCONFIG_BEGINNER_GUIDE.md#timer)。本模块重点检查 Timer instance、Clock Source/Divider/Prescaler、Mode、Actual Period、Event/IRQ 和是否由应用运行时改 Load。现场速查见 [Quick Reference](../../00_docs/SYSCONFIG_QUICK_REFERENCE.md)。

实际操作使用 CCS SysConfig 图形界面；参考 Profile 只用于对照字段和资源。保存后检查 Calculated Clock、Actual Period 和生成 LOAD，不手改 `.syscfg` 文本或 `ti_msp_dl_config.*`。

通用代码不绑定 SysConfig 实例名；接到 MSPM0 时由独立平台适配器使用生成宏。当前状态不代表对应外设已实板验证。

## 6. 初始化方法

模块不做隐式全局初始化。包含 `signal_timer.h`，由调用者准备配置、缓冲区或平台回调；如头文件提供 Init/Configure，先调用它。

## 7. 调用方法

`SignalTimer_SetRate`、`SignalTimer_Start`、`SignalTimer_Stop`、`SignalTimer_ReadCount`、`SignalTimer_GetModuleStatus`。

## 8. 参数修改方法

只修改调用者配置结构、count/capacity 和采样率等函数参数；不要为某个 Demo 改底层 `.c`。

## 9. 与其他模块如何连接

通过 `signal_types.h` 的数组+长度+采样率语义或本模块公开结构连接；先检查返回码再消费输出。

## 10. 最小示例

~~~c
#include "signal_timer.h"

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

**与上层模块的关系：** 这是低层计时 building block；普通采集用 ADC DMA，硬件测频用 Timer Capture 上层链。

以下声明来自真实公开头文件；源码没有说明的项保留 `UNKNOWN / NOT EXPOSED`。

### `signal_result_t SignalTimer_SetRate(const signal_timer_t *timer, uint32_t requested_rate_hz, uint32_t *configured_rate_hz);`

- **作用：** UNKNOWN / NOT EXPOSED

| 参数 | 真实类型 | 真实说明 |
|---|---|---|
| `timer` | `const signal_timer_t *` | UNKNOWN / NOT EXPOSED |
| `requested_rate_hz` | `uint32_t` | UNKNOWN / NOT EXPOSED |
| `configured_rate_hz` | `uint32_t *` | UNKNOWN / NOT EXPOSED |

- **返回：** UNKNOWN / NOT EXPOSED
- **调用前/后：** UNKNOWN / NOT EXPOSED

```c
signal_result_t result_value = SignalTimer_SetRate(timer, requested_rate_hz, configured_rate_hz);
```

### `signal_result_t SignalTimer_Start(const signal_timer_t *timer);`

- **作用：** UNKNOWN / NOT EXPOSED

| 参数 | 真实类型 | 真实说明 |
|---|---|---|
| `timer` | `const signal_timer_t *` | UNKNOWN / NOT EXPOSED |

- **返回：** UNKNOWN / NOT EXPOSED
- **调用前/后：** UNKNOWN / NOT EXPOSED

```c
signal_result_t result_value = SignalTimer_Start(timer);
```

### `signal_result_t SignalTimer_Stop(const signal_timer_t *timer);`

- **作用：** UNKNOWN / NOT EXPOSED

| 参数 | 真实类型 | 真实说明 |
|---|---|---|
| `timer` | `const signal_timer_t *` | UNKNOWN / NOT EXPOSED |

- **返回：** UNKNOWN / NOT EXPOSED
- **调用前/后：** UNKNOWN / NOT EXPOSED

```c
signal_result_t result_value = SignalTimer_Stop(timer);
```

### `signal_result_t SignalTimer_ReadCount(const signal_timer_t *timer, uint32_t *count);`

- **作用：** UNKNOWN / NOT EXPOSED

| 参数 | 真实类型 | 真实说明 |
|---|---|---|
| `timer` | `const signal_timer_t *` | UNKNOWN / NOT EXPOSED |
| `count` | `uint32_t *` | UNKNOWN / NOT EXPOSED |

- **返回：** UNKNOWN / NOT EXPOSED
- **调用前/后：** UNKNOWN / NOT EXPOSED

```c
signal_result_t result_value = SignalTimer_ReadCount(timer, count);
```

### `signal_module_status_t SignalTimer_GetModuleStatus(void);`

- **作用：** UNKNOWN / NOT EXPOSED

参数：无。

- **返回：** UNKNOWN / NOT EXPOSED
- **调用前/后：** UNKNOWN / NOT EXPOSED

```c
signal_module_status_t result_value = SignalTimer_GetModuleStatus();
```

## 18. Call Sequence / Connecting / Buffer Rules

```text
SignalTimer_SetRate -> SignalTimer_Start -> SignalTimer_Stop -> SignalTimer_ReadCount -> SignalTimer_GetModuleStatus
```

按具体功能只调用需要的 API；Init/Validate 在执行前，Get/Is 在执行后，Stop 在退出/取消时。每步检查返回值。指针、count、capacity 按元素数和真实声明准备；对象/数组由调用者拥有，模块不动态分配；DMA 或外设仍使用 buffer 时不能改写。

## 19. Config vs SysConfig / Resources / Verification

- 原 SysConfig 说明：通用代码不绑定 SysConfig 实例名；接到 MSPM0 时由独立平台适配器使用生成宏。当前状态不代表对应外设已实板验证。
- 软件参数和 buffer 长度为 CONFIG ONLY；pin、instance、clock、Timer、DMA、Event、IRQ、reference 为 SYSCONFIG REQUIRED。
- RAM 看实例和调用者 buffer；Flash/Stack 看最终 `.map`。
- 用已知输入与边界返回码做最小验证；未实板不得写 BOARD_VERIFIED。

## 20. Quick Modify Table

| 我想改什么 | 去哪里 | 影响 | SysConfig? |
|---|---|---|---|
| 软件参数/长度 | 上述真实 API/结构 | 按参数说明；UNKNOWN 项不猜测 | 否 |
| pin/外设/时钟 | `.syscfg` 与平台层 | 改变物理资源，需核对生成宏 | 是 |
| buffer 容量 | Application 声明与 count/capacity | RAM/可处理数据量 | 否 |

常见错误：不检查返回码、byte/element 混用、生命周期不足、跳过 Init/Validate、把配置值当实测值、改硬件后未重新生成 SysConfig。

## Integration Closure

Timer callbacks 用于把通用“目标 Hz/周期 tick”映射到具体 TimerG。正式实现 `SignalMSPM0G3507_Timer*` 位于统一平台层，调用 `DL_TimerG_setLoadValue/setTimerCount/startCounter/stopCounter/getTimerCount`。API 的 `count` 是完整周期 tick 数，硬件 LOAD 写 `count-1`；`clock_hz` 是分频/预分频后的真实计数时钟。

## 低频与溢出检查

Timer 周期为 `Ttimer=(LOAD+1)/clock_hz`。周期定时中断可以反复运行，但把 Timer 用作“两个事件之间的时间戳”时，必须确认上层是否真的把 overflow 数扩展进时间戳。当前 `timer_capture` 只用 ZERO 计数做超时，不能恢复相邻边沿之间的多次回绕；默认 2 ms Capture 因而不能测 10 Hz。不要把“打开 ZERO interrupt”误认为自动获得了扩展计数器。

10 Hz Capture 的 CCS 图形配置起点见 [Timer Capture README](../../02_acquisition/timer_capture/README.md)：LFXT 32.768 kHz → LFCLK/2、Period 2 s。普通 ADC/DAC 定时触发则仍按采样/更新率设置 Timer，不要因为输入信号是 10 Hz 就把采样 Timer 设成 10 Hz。

## Copy Into Target Project

链接 `01_bsp/timer/signal_timer.c`、`01_bsp/system_clock/signal_system_clock.c` 和平台 `.c`。SysConfig 先创建 TIMER/CAPTURE instance；再用生成的 instance 宏调用 `SignalMSPM0G3507_Timer_Bind()`。Timer trigger ADC 可参考 `PROFILE_01_ADC_CAPTURE`，capture 可参考 `PROFILE_05_FREQUENCY`。

## Hardware / Platform Binding

- Platform：[MSPM0G3507 Platform Adapter](../../08_applications/common/mspm0g3507/README.md)，正式文件 `signal_mspm0g3507_platform.h/.c`。
- `SignalMSPM0G3507_Timer_Bind` 填入 set/start/stop/read callbacks；count 单位为 tick，硬件 LOAD 写 `count-1`。
- SysConfig：trigger 参考 `PROFILE_01_ADC_CAPTURE`，capture 参考 `PROFILE_05_FREQUENCY`。
- 【COMPILE-VERIFIED EXAMPLE】：[`adc_timer_trigger_minimum/main.c`](../../09_examples/platform_closure/adc_timer_trigger_minimum/main.c)
