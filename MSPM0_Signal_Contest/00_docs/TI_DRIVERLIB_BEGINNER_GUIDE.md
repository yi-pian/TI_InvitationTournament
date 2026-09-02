# TI DriverLib 初学者指南（MSPM0G3507）

> 适用基线：LP-MSPM0G3507、MSPM0 SDK `2.11.00.07`。本文中的函数名和签名已对照本机 `C:\ti\mspm0_sdk_2_11_00_07\source\ti\driverlib` 头文件及同版本 LP-MSPM0G3507 DriverLib 示例。更换 SDK 后，应重新检查对应头文件，不能把本文当成跨版本保证。

本文不是 TI 全量 API Reference。它只解释信号类电赛和片外器件驱动中最常遇到的 DriverLib 用法。比赛现场只想查函数名时，看 [TI_DRIVERLIB_QUICK_REFERENCE.md](TI_DRIVERLIB_QUICK_REFERENCE.md)。

## 0. 比赛常用简单功能直接实现

本项目已锁定 MSPM0G3507 + CCS + SysConfig + MSPM0 SDK。先用下面这条规则：

```text
简单硬件动作 → SysConfig + DriverLib
复杂硬件流程 → 正式硬件/集成模块
信号计算     → MSPM0_Signal_Contest
```

| 我要做什么 | SysConfig 先完成 | main 里的推荐运行时动作 | 是否需要 Signal Module |
|---|---|---|---|
| DAC 输出固定 code | DAC0、VREF、12-bit、PA15、output enable | `DL_DAC12_output12(DAC0, code)` | 否 |
| GPIO 拉高/拉低/翻转 | Pin、方向、上下拉、初值 | `DL_GPIO_setPins/clearPins/togglePins` | 否 |
| 读取 GPIO | GPIO input、pull-up/down | `DL_GPIO_readPins(port, pin)` | 否；要按键消抖事件才用 Button |
| 简单读一次 ADC | ADC instance/channel/MEM/reference/software trigger | `DL_ADC12_startConversion` → 等待完成 → `DL_ADC12_getMemResult` | 否；采 `raw[N]` 用 ADC DMA |
| Timer 启动/停止/读计数 | mode、clock、divider、period | `DL_TimerG_startCounter/stopCounter/getTimerCount` | 否；完整捕获测频用 Timer Capture |
| UART blocking 发字节 | UART、Pin、baud、frame | `DL_UART_Main_transmitDataBlocking` | 否 |
| SPI blocking 发字节 | SPI、Pin、mode、bit rate、CS GPIO | `DL_SPI_transmitDataBlocking8` | 否；复杂片外器件协议用器件模块 |
| DAC 连续输出正弦 | Timer/Event/DMA/DAC | DDS/Wave Table → DAC DMA | 是 |
| 固定 Fs 采 N 点 | Timer/Event/ADC/DMA | ADC DMA | 是 |

### DAC 固定 code：最短完整形态

```c
#include "ti_msp_dl_config.h"

int main(void)
{
    SYSCFG_DL_init();
    DL_DAC12_output12(DAC0, 2048U);
    while (1) __WFI();
}
```

`DAC0` 是当前工程的 DAC instance；`2048U` 是 12-bit 半量程附近的 code，合法范围 `0..4095`。当前 Profile 已在生成初始化中 enable DAC，所以运行时只写 code。真实可编译工程见 [dac_dc_minimum](../09_examples/platform_closure/dac_dc_minimum/README.md)。

### GPIO：最短形态

```c
SYSCFG_DL_init();
DL_GPIO_setPins(SIGNAL_GPIO_PORT, SIGNAL_GPIO_OUTPUT_PIN);
bool high = (DL_GPIO_readPins(INPUT_PORT, INPUT_PIN) & INPUT_PIN) != 0U;
```

`SIGNAL_GPIO_*` 是当前参考 Profile 的生成宏；`INPUT_PORT/INPUT_PIN` 表示你自己生成头里的宏，不是固定名字。

### ADC 单次结果：最短可靠形态

```c
DL_ADC12_clearInterruptStatus(
    SIGNAL_BASIC_ADC_INST, DL_ADC12_INTERRUPT_MEM0_RESULT_LOADED);
DL_ADC12_startConversion(SIGNAL_BASIC_ADC_INST);
while (DL_ADC12_getRawInterruptStatus(
           SIGNAL_BASIC_ADC_INST,
           DL_ADC12_INTERRUPT_MEM0_RESULT_LOADED) == 0U) {}
uint16_t raw = DL_ADC12_getMemResult(
    SIGNAL_BASIC_ADC_INST, SIGNAL_BASIC_ADC_ADCMEM_0);
```

这是 bring-up/单样本路径。需要采样率 `Fs` 与样本数 `N` 时，直接转到 ADC DMA README，不把这个轮询复制 N 次。

### Timer 与 UART：最短形态

```c
DL_TimerG_startCounter(YOUR_TIMER_INST);
uint32_t count = DL_TimerG_getTimerCount(YOUR_TIMER_INST);
DL_TimerG_stopCounter(YOUR_TIMER_INST);

DL_UART_Main_transmitDataBlocking(SIGNAL_UART_INST, (uint8_t)'A');
```

`YOUR_TIMER_INST` 明确表示“替换成当前生成头中的 Timer instance 宏”。PinMux、Timer load 与 UART baud 不由这些调用重新配置。

更完整的选择边界见 [WHEN_TO_USE_DRIVERLIB_OR_MODULE.md](WHEN_TO_USE_DRIVERLIB_OR_MODULE.md)。

## 1. DriverLib 在工程中的位置

```text
Application
  ├─ 简单动作 ───────────────→ TI DriverLib → MSPM0G3507
  ├─ 复杂硬件流程 → Signal Module → TI DriverLib → MSPM0G3507
  └─ 信号计算 ───────────────→ Algorithm Module
```

- 要采一帧 `raw[N]`：优先使用现有 `ADC_DMA`，不要在 `main.c` 里重新拼 Timer + Event + ADC + DMA。
- 要连续输出波形：优先使用现有 `DAC_DMA` / DDS 模块。
- 要驱动陌生 SPI、I2C、UART 器件，阅读 TI 官方示例，或定位底层故障：需要理解 DriverLib。
- DriverLib 解决“怎样操作片上外设”，不会替你处理片外芯片的数据手册协议、模拟电气条件或信号算法。

## 2. 第一条规则：先看 SysConfig 已经做了什么

大多数本仓库 CCS 工程包含：

```c
#include "ti_msp_dl_config.h"

int main(void)
{
    SYSCFG_DL_init();
    /* 从这里开始做运行时操作 */
}
```

`SYSCFG_DL_init()` 是 SysConfig 生成的工程级初始化入口。当前 SDK 生成代码通常已完成系统时钟、PinMux、外设电源和复位、外设静态参数，以及 SysConfig 中声明的 Event 路由。生成的 `ti_msp_dl_config.h` 还提供实例、端口、引脚、IRQ、ADC MEM 等宏，例如官方示例中的：

```c
#define CPUCLK_FREQ                 32000000
#define ADC12_0_INST                ADC0
#define ADC12_0_ADCMEM_0            DL_ADC12_MEM_IDX_0
#define TIMER_0_INST                TIMG0
#define GPIO_LEDS_PORT              GPIOA
#define GPIO_LEDS_USER_LED_1_PIN    DL_GPIO_PIN_0
```

宏名由你在 SysConfig 中的实例名/引脚组名决定。**先打开自己工程生成的 `ti_msp_dl_config.h`，不要照抄其他示例的宏名。**

| 通常交给 SysConfig | 通常由应用在运行时调用 |
|---|---|
| 系统时钟、外设源时钟和分频 | 启动/停止 Timer、ADC 转换、DMA |
| PinMux、输入/输出属性、上下拉 | GPIO set/clear/toggle/read |
| SPI mode、bit rate、data size、FIFO 阈值 | SPI transmit/receive、片选时序 |
| I2C bit rate、controller/target 模式 | 发起一次 transaction、收发数据 |
| UART baud、数据位、校验、停止位 | 收发、查询 FIFO、处理中断 |
| ADC 通道、参考、采样时间、触发源 | start、get result；块采集调用正式模块 |
| Timer 模式、时钟、预分频、Capture 路由 | start/stop/read count/read capture |
| DAC 参考、放大器、FIFO/触发结构 | 写单个 code；连续波形调用正式模块 |
| OPA/GPAMP/COMP/VREF 静态模拟路由 | 少量阈值调整、enable、读状态 |

除非你明确要动态重配置，否则不要在 `main()` 再调用 `DL_xxx_enablePower()`、`DL_xxx_reset()` 和整套 `DL_xxx_init...()` 覆盖生成配置。若怀疑 SysConfig 没生效，先看 `.syscfg`、生成的 `ti_msp_dl_config.c/.h` 和 [SYSCONFIG_BEGINNER_GUIDE.md](SYSCONFIG_BEGINNER_GUIDE.md)。

## 3. 怎样读函数名和参数

函数名通常是：

```text
DL_外设_动作或对象
```

| 前缀 | 属于 | 常见用途 |
|---|---|---|
| `DL_Common_` | 通用辅助 | 短延时 |
| `DL_GPIO_` | GPIO/IOMUX | 控制 CS、RESET、LED，读 BUSY/按键 |
| `DL_SYSCTL_` | 系统控制/时钟 | 时钟状态、低功耗、复位原因 |
| `DL_TimerG_` / `DL_Timer_` | GPTimer | 定时、采样触发、PWM、Capture |
| `DL_ADC12_` | 片上 ADC12 | 转换、结果、FIFO、DMA、事件 |
| `DL_DAC12_` | 片上 DAC12 | 写 code、FIFO、触发、DMA |
| `DL_DMA_` | DMA | 地址、长度、通道启停 |
| `DL_SPI_` | SPI | 同步串行收发 |
| `DL_I2C_` | I2C | controller/target transaction |
| `DL_UART_` / `DL_UART_Main_` | UART | 串口收发 |
| `DL_COMP_` | Comparator | 比较、内部 DAC 阈值、事件 |
| `DL_VREF_` | 内部参考 | 1.4 V/2.5 V 参考配置 |
| `DL_OPA_` / `DL_GPAMP_` | 片上模拟放大器 | 缓冲、PGA、模拟路由 |
| `DL_MathACL_` | 数学加速器 | 定点乘除、三角等低层运算 |

常见动词：`enable/disable` 打开或关闭功能；`start/stop` 开始或停止一次运行；`set/get` 写/读配置或状态；`read/write` 操作 GPIO/寄存器数据；`transmit/receive` 收发；`clear...Status` 清标志；`getPendingInterrupt` 读取最高优先级待处理中断索引。

常见参数不是“随便填的整数”：

- `GPIO_Regs *gpio`、`SPI_Regs *spi` 等：外设寄存器实例指针。优先传生成宏，如 `SPI_0_INST`，宏最终可能展开成 `SPI0`。
- `pins`、`interruptMask`、`eventMask`：位掩码，可用 `|` 同时选择多个位。
- `DL_xxx_...` 类型：受限的枚举；到对应头文件找枚举定义，不猜数字。
- `buffer`：数组首地址；必须保证长度、元素宽度、生命周期和 DMA 可访问性正确。
- `count/length/size`：先看单位是 byte、halfword、样本还是 DMA transfer，不能只看变量名。

## 4. A — Common：什么时候用

用于外部芯片复位后的短等待、简单 GPIO 时序和 bring-up。不要拿它产生 ADC 采样周期或长期定时。

### `DL_Common_delayCycles(uint32_t cycles)`

**一句话：**让 CPU 至少消耗指定数量的 CPU cycle。

**什么时候用：**RESET 脉冲、上电后几十微秒级等待、低速 bit-bang 调试。

**不要什么时候用：**精确采样、精确波形更新、协议超时、长时间等待；这些用 GPTimer/事件/状态机。

**参数：**`cycles` 是 `uint32_t` 的 CPU cycle 数，不是微秒。当前 SDK 明确说明传 `0` 会产生最大可能延时，不代表“零延时”。

**估算：**

```text
理想时间（秒）≈ cycles / CPUCLK_FREQ
cycles ≈ 目标秒数 × CPUCLK_FREQ
```

若 `CPUCLK_FREQ = 32,000,000`，`DL_Common_delayCycles(3200)` 的理想量级约为 `100 us`。SDK 只保证“至少”这些 cycle，不保证精确相等；其文档说明典型偏差约 10 cycle 或更小（代码在 Flash 且 cache 开启时），函数进出栈、4-cycle 循环对齐、代码地址对齐和 Flash wait-state 都会产生偏差。

**最小示例：**

```c
#include "ti_msp_dl_config.h"

int main(void)
{
    SYSCFG_DL_init();
    DL_GPIO_clearPins(GPIO_LEDS_PORT, GPIO_LEDS_USER_LED_1_PIN);
    DL_Common_delayCycles(CPUCLK_FREQ / 1000U); /* 约 1 ms，不是精密 1 ms */
    DL_GPIO_setPins(GPIO_LEDS_PORT, GPIO_LEDS_USER_LED_1_PIN);
    for (;;) {}
}
```

**常见错误：**把 `1000` 当 `1000 us`；CPU 时钟改变后不重算；在主循环中阻塞导致丢采样；传 `0` 期望立即返回。

**替代：**精确定时用 GPTimer；等待器件 ready 最好轮询 BUSY/状态并加 Timer 超时。

## 5. B — GPIO：什么时候用

控制片外芯片 `CS/RESET/ENABLE`、LED、继电器和模拟开关，或读取 `BUSY/DRDY/INT`、按键。PinMux、方向和上下拉通常先在 SysConfig 配好。

### GPIO 输出家族

```c
void DL_GPIO_setPins(GPIO_Regs *gpio, uint32_t pins);
void DL_GPIO_clearPins(GPIO_Regs *gpio, uint32_t pins);
void DL_GPIO_togglePins(GPIO_Regs *gpio, uint32_t pins);
void DL_GPIO_writePinsVal(GPIO_Regs *gpio, uint32_t pinsMask, uint32_t pinsVal);
void DL_GPIO_enableOutput(GPIO_Regs *gpio, uint32_t pins);
void DL_GPIO_disableOutput(GPIO_Regs *gpio, uint32_t pins);
```

**一句话：**分别把选中引脚置高、置低、翻转、按掩码更新，以及打开/关闭输出驱动。

**参数：**`gpio` 是端口实例，如生成宏最终展开的 `GPIOA/GPIOB`；`pins`/`pinsMask` 是 `DL_GPIO_PIN_x` 位掩码，可写 `PIN_A | PIN_B`；`pinsVal` 中只有 `pinsMask` 选中的位会生效。

**选择：**单独置高用 `setPins`；单独置低用 `clearPins`；周期翻转用 `togglePins`；同时更新同一端口几根脚且保留其他脚用 `writePinsVal`。`writePins` 会给所有已启用 GPIO 输出写入 `pins` 的对应位，不适合作为“只改一根脚”的默认 API。

**最小 CS/RESET 示例：**假设 SysConfig 已将两脚配置为初始高电平输出，并生成下列宏。

```c
#include "ti_msp_dl_config.h"

static void device_reset(void)
{
    DL_GPIO_clearPins(GPIO_DEVICE_PORT, GPIO_DEVICE_RESET_PIN);
    DL_Common_delayCycles(CPUCLK_FREQ / 10000U); /* 约 100 us */
    DL_GPIO_setPins(GPIO_DEVICE_PORT, GPIO_DEVICE_RESET_PIN);
}

static void device_write_begin(void)
{
    DL_GPIO_clearPins(GPIO_DEVICE_PORT, GPIO_DEVICE_CS_PIN); /* active-low */
}

static void device_write_end(void)
{
    DL_GPIO_setPins(GPIO_DEVICE_PORT, GPIO_DEVICE_CS_PIN);
}
```

**常见错误：**把芯片的 active-low 信号理解反；先 enable 输出再设置安全初值导致毛刺；把 PINCM/IOMUX 宏当作 `pins`；忘记不同引脚可能不在同一 GPIO port。

### GPIO 输入

```c
uint32_t DL_GPIO_readPins(GPIO_Regs *gpio, uint32_t pins);
```

返回所选引脚当前为高的位。判断单脚要用掩码：

```c
bool busy = (DL_GPIO_readPins(GPIO_DEVICE_BUSY_PORT,
                             GPIO_DEVICE_BUSY_PIN) != 0U);
```

`DL_GPIO_initDigitalInput(uint32_t pincmIndex)` 和 `DL_GPIO_initDigitalOutput(uint32_t pincmIndex)` 的参数是 **PINCM 寄存器索引**，不是 `DL_GPIO_PIN_x`。它们属于 PinMux/静态初始化，通常由 SysConfig 生成；`enableOutput` 只控制端口输出驱动，也不能代替 PINCM 配置。

### GPIO 中断

```c
void DL_GPIO_enableInterrupt(GPIO_Regs *gpio, uint32_t pins);
uint32_t DL_GPIO_getEnabledInterruptStatus(GPIO_Regs *gpio, uint32_t pins);
uint32_t DL_GPIO_getRawInterruptStatus(GPIO_Regs *gpio, uint32_t pins);
DL_GPIO_IIDX DL_GPIO_getPendingInterrupt(GPIO_Regs *gpio);
void DL_GPIO_clearInterruptStatus(GPIO_Regs *gpio, uint32_t pins);
```

- `enableInterrupt` 只打开 GPIO 外设内部掩码；还要 `NVIC_EnableIRQ(生成的_IRQN)`。
- `getEnabledInterruptStatus` 只看已使能且待处理的位；`getRawInterruptStatus` 不要求先使能。
- `getPendingInterrupt` 返回最高优先级 IIDX，适合在 ISR 中 `switch`。
- `clearInterruptStatus` 的第二参数仍是 pin bit mask，不是 IIDX。

边沿、极性和滤波优先在 SysConfig 配。按键还需要软件消抖；清标志的时机按该工程 ISR 结构和 SDK 示例处理。

## 6. C — SYSCTL / Clock：什么时候用

用于确认时钟/复位原因和低功耗行为。系统时钟树是全局资源，普通应用不要运行时随意改。

常见真实 API：

| API | 用途 | 默认归属 |
|---|---|---|
| `DL_SYSCTL_getClockStatus()` | 读时钟源/PLL/FCC 等状态位 | 调试/状态检查 |
| `DL_SYSCTL_getResetCause()` | 读取复位原因枚举 | 启动诊断 |
| `DL_SYSCTL_enableSleepOnExit()` | ISR 返回后自动进入 sleep | 低功耗事件应用 |
| `DL_SYSCTL_disableSleepOnExit()` | 关闭上述行为 | 运行模式切换 |
| `DL_SYSCTL_setSYSOSCFreq()`、`DL_SYSCTL_setMCLKDivider()` | 改系统振荡器声明/主时钟分频 | 通常由 SysConfig 生成 |
| `DL_SYSCTL_configSYSPLL()`、`DL_SYSCTL_enableSYSPLL()` | 配置/打开 PLL | 通常由 SysConfig 生成 |

`CPUCLK_FREQ` 是当前 SysConfig 生成头里的频率宏，适合计算本工程的 cycle 延时；它不是 DriverLib 自动测得的实时频率。改 Clock Tree 后要重新 Generate，并重新核查 Timer、UART、SPI、I2C 和采样率。

## 7. D — Timer / GPTimer：什么时候用

用于周期中断、ADC/DAC 硬件触发、PWM、边沿计数和 Capture 测频。Timer 的模式、输入路由、时钟源、divider、prescaler、load 和事件连接通常由 SysConfig 配；应用负责启停与读取。

`dl_timerg.h` 把 `DL_TimerG_startCounter` 等名字重定向到通用 `DL_Timer_*` 实现，因此在代码中看到两种前缀并不代表两套 Timer。

### 启停和计数

```c
void DL_TimerG_startCounter(GPTIMER_Regs *gptimer);
void DL_TimerG_stopCounter(GPTIMER_Regs *gptimer);
bool DL_TimerG_isRunning(const GPTIMER_Regs *gptimer);
void DL_TimerG_setLoadValue(GPTIMER_Regs *gptimer, uint32_t value);
uint32_t DL_TimerG_getLoadValue(const GPTIMER_Regs *gptimer);
uint32_t DL_TimerG_getTimerCount(const GPTIMER_Regs *gptimer);
void DL_TimerG_setTimerCount(GPTIMER_Regs *gptimer, uint32_t value);
```

所有 `gptimer` 参数传生成的 Timer 实例宏，如 `TIMER_0_INST`。`value` 的有效位宽取决于具体 Timer 实例。运行中直接 `setTimerCount` 可能产生不可预测行为；SDK 明确建议停表后使用，运行中改变周期应使用 `setLoadValue`（并理解 shadow/load 行为）。

简单周期模式中常见估算为：

```text
timer_tick = source_clock / divider / prescaler
period ≈ (LOAD + 1) / timer_tick
```

中心对齐 PWM、上下计数和特殊模式不应直接套这个式子；以 SysConfig 显示的实际周期和 TRM 为准。

**最小示例：**

```c
#include "ti_msp_dl_config.h"

int main(void)
{
    SYSCFG_DL_init();
    NVIC_EnableIRQ(TIMER_0_INST_INT_IRQN);
    DL_TimerG_startCounter(TIMER_0_INST);
    for (;;) { __WFI(); }
}

void TIMER_0_INST_IRQHandler(void)
{
    switch (DL_TimerG_getPendingInterrupt(TIMER_0_INST)) {
        case DL_TIMER_IIDX_ZERO:
            DL_GPIO_togglePins(GPIO_LEDS_PORT, GPIO_LEDS_USER_LED_1_PIN);
            break;
        default:
            break;
    }
}
```

宏和值必须以自己生成头为准；不同 TimerG 示例也可能使用 `DL_TIMERG_IIDX_...` 别名。

### Capture

```c
uint32_t DL_Timer_getCaptureCompareValue(
    GPTIMER_Regs *gptimer, DL_TIMER_CC_INDEX ccIndex);
```

`gptimer` 是 Capture Timer 实例，`ccIndex` 是 `DL_TIMER_CC_0_INDEX` 等捕获比较通道索引；返回捕获瞬间的 CC 寄存器值。频率必须按计数方向、回绕、两次边沿差和 timer tick 计算：

```text
ticks = 两次捕获值的模计数差
frequency_hz = timer_tick_hz / ticks
```

本仓库测频优先用正式 Timer Capture 模块，不在每个应用重复处理回绕。

### Timer interrupt / event

`DL_TimerG_enableInterrupt(timer, mask)`、`DL_TimerG_getPendingInterrupt(timer)`、`DL_TimerG_clearInterruptStatus(timer, mask)` 处理 CPU 中断；`setPublisherChanID`、`setSubscriberChanID` 以及 event mask 处理 Event Fabric。事件路由一般由 SysConfig 一次性生成，特别是 Timer → ADC 和 Timer → DAC，不要只凭“相同 channel 数字”手工拼接。

## 8. E — ADC12：什么时候用

调试单次转换、理解生成代码或做特殊 ADC 功能时直接看 DriverLib。高速 `N` 点采样优先用现有 `ADC_DMA`，因为它已经处理 Timer/Event/ADC/DMA、buffer、sample count 和 sample rate。

### 转换和读取

```c
void DL_ADC12_enableConversions(ADC12_Regs *adc12);
void DL_ADC12_disableConversions(ADC12_Regs *adc12);
void DL_ADC12_startConversion(ADC12_Regs *adc12);
void DL_ADC12_stopConversion(ADC12_Regs *adc12);
uint16_t DL_ADC12_getMemResult(
    const ADC12_Regs *adc12, DL_ADC12_MEM_IDX idx);
```

- `adc12`：生成实例宏，如 `ADC12_0_INST`。
- `idx`：转换 memory 槽，如生成的 `ADC12_0_ADCMEM_0`，不是模拟引脚号。
- `enableConversions` 允许转换；`startConversion` 发起/启动配置好的转换序列。具体触发模式决定后续由软件还是事件继续触发。
- `getMemResult` 返回 `uint16_t` code；有效分辨率、对齐、参考和平均配置取决于 SysConfig。

**单次转换形态：**

```c
#include "ti_msp_dl_config.h"

volatile bool gAdcDone;

int main(void)
{
    uint16_t raw;
    SYSCFG_DL_init();
    NVIC_EnableIRQ(ADC12_0_INST_INT_IRQN);
    DL_ADC12_startConversion(ADC12_0_INST);
    while (!gAdcDone) { __WFI(); }
    raw = DL_ADC12_getMemResult(ADC12_0_INST, ADC12_0_ADCMEM_0);
    (void) raw;
    for (;;) {}
}

void ADC12_0_INST_IRQHandler(void)
{
    if (DL_ADC12_getPendingInterrupt(ADC12_0_INST) ==
        DL_ADC12_IIDX_MEM0_RESULT_LOADED) {
        gAdcDone = true;
    }
}
```

这段要求 SysConfig 已使能对应 MEM loaded interrupt。某些官方示例在 ISR 中再次 `DL_ADC12_enableConversions()`，那属于该示例具体 conversion mode 的重新武装流程，不能无条件复制。

理想单极性换算常写为 `V ≈ raw × VREF / (2^bits - 1)`，但真实系统还受参考误差、增益/偏置、前端分压和校准影响。正式应用使用 `ADC_ToVoltage`，不要把换算散落在 `main.c`。

### ADC 中断和 DMA

```c
void DL_ADC12_enableInterrupt(ADC12_Regs *adc12, uint32_t interruptMask);
DL_ADC12_IIDX DL_ADC12_getPendingInterrupt(const ADC12_Regs *adc12);
void DL_ADC12_clearInterruptStatus(ADC12_Regs *adc12, uint32_t interruptMask);
uint32_t DL_ADC12_getFIFOAddress(const ADC12_Regs *adc12);
void DL_ADC12_enableDMA(ADC12_Regs *adc12);
void DL_ADC12_setDMASamplesCnt(ADC12_Regs *adc12, uint8_t sampCnt);
void DL_ADC12_enableDMATrigger(ADC12_Regs *adc12, uint32_t dmaMask);
```

`interruptMask`/`dmaMask` 是对应头文件定义的位掩码。`sampCnt` 当前 SDK 声明有效范围 `0..24`，含义是一次 DMA trigger 对应的 ADC result 数，不等于应用总采样点数 `N`。DMA 源可使用 FIFO 地址或 MEM result 地址，必须与 ADC FIFO/序列、DMA width 和增量方向一致。普通用户不要绕过正式 `ADC_DMA` 重新组合这些细节。

### Power / reset

`DL_ADC12_enablePower()`、`DL_ADC12_reset()`、`DL_ADC12_isPowerEnabled()`、`DL_ADC12_isReset()` 确实存在，但通常出现在生成的初始化代码，不是每次采样前都调用。Reset 会丢失配置。

## 9. F — DAC12：什么时候用

输出固定 DC code、调试参考和读 TI 示例时可直接使用。连续周期波、DDS 和 replay 优先调用现有 `DDS`/`DAC_DMA`。

### 写一个 DAC code

```c
void DL_DAC12_output12(DAC12_Regs *dac12, uint32_t dataValue);
void DL_DAC12_enable(DAC12_Regs *dac12);
void DL_DAC12_disable(DAC12_Regs *dac12);
```

`dac12` 是生成实例宏；`dataValue` 有效范围 `0x000..0xFFF`，函数内部会掩成 12 bit。理想单极性估算：

```text
code ≈ Vout / Vref × 4095
```

必须检查 DAC 参考、放大器模式、输出范围、负载和校准。不要只凭 3.3 V 供电就假设 `Vref=3.3 V`。

```c
#include "ti_msp_dl_config.h"

int main(void)
{
    SYSCFG_DL_init();
    DL_DAC12_output12(DAC12_0_INST, 2048U);
    DL_DAC12_enable(DAC12_0_INST);
    for (;;) {}
}
```

### 参考、FIFO、trigger、DMA

```c
void DL_DAC12_setReferenceVoltageSource(
    DAC12_Regs *dac12, DL_DAC12_VREF_SOURCE refsVal);
void DL_DAC12_enableFIFO(DAC12_Regs *dac12);
void DL_DAC12_setFIFOThreshold(
    DAC12_Regs *dac12, DL_DAC12_FIFO_THRESHOLD fifoThreshold);
uint32_t DL_DAC12_fillFIFO12(
    DAC12_Regs *dac12, const uint16_t *buffer, uint32_t count);
void DL_DAC12_enableDMATrigger(DAC12_Regs *dac12);
```

`refsVal` 和 `fifoThreshold` 必须选 `dl_dac12.h` 的枚举；`buffer` 是 12-bit code 数组（存于 `uint16_t`）；`count` 是元素数。当前 SDK 明确要求：CPU 向 DAC 装数据时应保持 DMA trigger generator 关闭；DMA 模式下 FIFO、阈值、trigger source、DMA width/address/length 必须成套一致。故连续输出直接使用正式 `DAC_DMA`。

Power/reset API 同 ADC 一样通常由 SysConfig 生成，不能在每个输出周期 reset。

## 10. G — DMA：什么时候用

当 ADC、DAC、SPI、UART 等需要在外设和 RAM 间搬大量数据且不想每个元素触发 CPU 时使用。DMA 不理解“电压”或“波形”，只按地址、宽度、增量、触发和次数搬数据。

```c
void DL_DMA_setSrcAddr(DMA_Regs *dma, uint8_t channelNum, uint32_t srcAddr);
void DL_DMA_setDestAddr(DMA_Regs *dma, uint8_t channelNum, uint32_t destAddr);
void DL_DMA_setTransferSize(DMA_Regs *dma, uint8_t channelNum, uint16_t size);
void DL_DMA_enableChannel(DMA_Regs *dma, uint8_t channelNum);
void DL_DMA_disableChannel(DMA_Regs *dma, uint8_t channelNum);
void DL_DMA_startTransfer(DMA_Regs *dma, uint8_t channelNum);
```

- `dma`：通常为 `DMA`。
- `channelNum`：SysConfig 分配的 channel ID 宏，不是 IRQ number。
- `srcAddr`/`destAddr`：32-bit 地址值，常写 `(uint32_t)&object` 或 DriverLib 提供的外设数据地址。
- `size`：DMA transfer 数，当前声明范围 `0..65535`；每次 transfer 是 8/16/32 bit 由 channel width 决定。
- `startTransfer` 是软件触发；外设触发模式通常先 enable channel，等待 ADC/DAC/SPI/UART event。

常见错误是：ADC 为 16-bit result 但 DMA 配 8-bit；RAM 目的地址未递增；外设源地址错误地递增；重复 Start 前没重装地址/size；buffer 是局部变量且生命周期已结束；channel 与别的模块冲突。

## 11. H — SPI：什么时候用

连接片外 DDS、ADC、DAC、数字电位器、TFT/OLED 和高速传感器。先从 datasheet 确认 CPOL/CPHA、最大 SCLK、bit order、word length、CS 时序和读命令 dummy byte，再在 SysConfig 配 SPI controller。

### Blocking、non-blocking 和 FIFO

```c
void DL_SPI_transmitDataBlocking8(SPI_Regs *spi, uint8_t data);
uint8_t DL_SPI_receiveDataBlocking8(const SPI_Regs *spi);
bool DL_SPI_transmitDataCheck8(SPI_Regs *spi, uint8_t data);
bool DL_SPI_receiveDataCheck8(const SPI_Regs *spi, uint8_t *buffer);
uint32_t DL_SPI_fillTXFIFO8(
    SPI_Regs *spi, const uint8_t *buffer, uint32_t count);
uint32_t DL_SPI_drainRXFIFO8(
    const SPI_Regs *spi, uint8_t *buffer, uint32_t maxCount);
bool DL_SPI_isBusy(const SPI_Regs *spi);
```

**参数逐个看：**`spi` 传生成实例宏；`data` 是一帧最多 8 bit 的数据；`buffer` 是输入或输出数组；`count/maxCount` 是 byte 数；`receiveDataCheck8` 的 `buffer` 是单字节输出地址，成功返回 `true`；fill/drain 返回实际装入/读出的 byte 数，可能小于请求数。

**Blocking：**等待 FIFO 可用/有数据。`transmitDataBlocking8` 还会等发送完成且 SPI 不 busy。简单寄存器写、bring-up 很适合；故障时可能永久卡住，正式驱动应加状态机/超时。

**Check：**立即尝试，FIFO 满/空就返回 `false`，适合非阻塞状态机和 ISR。

**FIFO：**`fillTXFIFO8` 只尽可能填 FIFO，不保证整个 buffer 已发送；`drainRXFIFO8` 只读当前已有数据。剩余部分要循环、由 interrupt/DMA 推进，并检查返回数量。

**关键硬件事实：SPI 是全双工。**controller 只有发送数据才产生 SCLK。想读一个寄存器，通常需要先发命令/地址，再发送 datasheet 指定的 dummy byte，同时读 RX。单独调用 `receiveDataBlocking8()` 不会替 controller 产生新时钟，可能永远等不到数据。

### 最小片外寄存器写形态

```c
#include "ti_msp_dl_config.h"

static void spi_write_register(uint8_t reg, uint8_t value)
{
    DL_GPIO_clearPins(GPIO_DEVICE_CS_PORT, GPIO_DEVICE_CS_PIN);
    DL_SPI_transmitDataBlocking8(SPI_0_INST, reg);
    DL_SPI_transmitDataBlocking8(SPI_0_INST, value);
    while (DL_SPI_isBusy(SPI_0_INST)) {}
    DL_GPIO_setPins(GPIO_DEVICE_CS_PORT, GPIO_DEVICE_CS_PIN);
}
```

这里的 `reg` 是否要置 write bit、两个字节之间能否释放 CS、RX FIFO 是否要清空，都由片外芯片 datasheet 决定。

### 为什么常用 GPIO 控 CS

硬件 CS 可以降低 CPU 负担，但片外芯片经常要求“整条命令期间保持低”“命令与数据之间不可抬高”“写完后再等待 busy”。用 GPIO 更容易精确表达器件帧边界，也便于一条 SPI 总线挂多个器件。它不是绝对规则：若硬件 CS 的行为完全满足 datasheet，可以用硬件 CS。

### SPI interrupt / DMA

`DL_SPI_enableInterrupt(spi, interruptMask)`、`DL_SPI_getPendingInterrupt(spi)`、`DL_SPI_clearInterruptStatus(spi, interruptMask)` 的 mask 来自 `DL_SPI_INTERRUPT_*`。DMA 还要配置 TX/RX event、DMA channel 和 FIFO threshold。高速连续收发优先参考当前 SDK 的 `spi_controller_fifo_dma_interrupts`，不要把 polling 示例机械放大。

## 12. I — I2C：什么时候用

连接 EEPROM、小型 ADC/DAC、时钟、传感器和 GPIO 扩展。先确认电压、SCL/SDA 上拉、7/10-bit 地址、最高速率、寄存器地址宽度、repeated-start 和 clock stretching。

### 基础 controller transaction

```c
uint16_t DL_I2C_fillControllerTXFIFO(
    I2C_Regs *i2c, const uint8_t *buffer, uint16_t count);
void DL_I2C_startControllerTransfer(I2C_Regs *i2c,
    uint32_t targetAddr, DL_I2C_CONTROLLER_DIRECTION direction,
    uint16_t length);
uint32_t DL_I2C_getControllerStatus(const I2C_Regs *i2c);
bool DL_I2C_isControllerRXFIFOEmpty(const I2C_Regs *i2c);
uint8_t DL_I2C_receiveControllerData(const I2C_Regs *i2c);
void DL_I2C_transmitControllerData(I2C_Regs *i2c, uint8_t data);
```

**参数：**`i2c` 是生成实例；`buffer` 是待发 byte；`count` 是预填 FIFO 的 byte 数，返回实际写入数；`targetAddr` 是地址字段，7-bit 模式传 datasheet 的 7-bit 地址，不要传已经左移并含 R/W bit 的“8-bit 地址”；`direction` 取 `DL_I2C_CONTROLLER_DIRECTION_TX/RX`；`length` 是本次 burst byte 数，声明范围 `0..0xFFF`。

`DL_I2C_startControllerTransfer` 设置地址、方向、长度，并产生 START 和末尾 STOP；数据装入/读出另做。返回 void，不代表 target 已 ACK 或 transaction 已完成。用：

```c
uint32_t status = DL_I2C_getControllerStatus(I2C_0_INST);
bool busy = ((status & DL_I2C_CONTROLLER_STATUS_BUSY) != 0U);
bool error = ((status & DL_I2C_CONTROLLER_STATUS_ERROR) != 0U);
```

还可检查 `ADDR_ACK`、`DATA_ACK`、`ARBITRATION_LOST`、`IDLE`、`BUSY_BUS` 等位。`BUSY` 是本 controller transaction 正在进行；`BUSY_BUS` 表示总线处于 START 与 STOP 之间，两者不能混为一谈。

### 最小写寄存器形态

```c
#include "ti_msp_dl_config.h"

static bool i2c_write_reg(uint8_t address7, uint8_t reg, uint8_t value)
{
    uint8_t tx[2] = {reg, value};
    if (DL_I2C_fillControllerTXFIFO(I2C_0_INST, tx, 2U) != 2U) {
        return false;
    }
    DL_I2C_startControllerTransfer(I2C_0_INST, address7,
        DL_I2C_CONTROLLER_DIRECTION_TX, 2U);
    while ((DL_I2C_getControllerStatus(I2C_0_INST) &
            DL_I2C_CONTROLLER_STATUS_BUSY) != 0U) {}
    return ((DL_I2C_getControllerStatus(I2C_0_INST) &
             DL_I2C_CONTROLLER_STATUS_ERROR) == 0U);
}
```

该最小例子为说明调用关系，正式驱动必须加入超时、仲裁丢失/NACK 处理和 FIFO 清理策略。

### 读取寄存器与 repeated-start

许多器件要求 `START + 地址W + 寄存器地址 + RESTART + 地址R + 数据 + STOP`。基础 `startControllerTransfer` 会为每次 burst 配 START+STOP，因此不要把“写寄存器地址一次 + 读一次”默认当作 repeated-start。当前 SDK 提供：

```c
void DL_I2C_startControllerTransferAdvanced(I2C_Regs *i2c,
    uint32_t targetAddr, DL_I2C_CONTROLLER_DIRECTION direction,
    uint16_t length, DL_I2C_CONTROLLER_START start,
    DL_I2C_CONTROLLER_STOP stop, DL_I2C_CONTROLLER_ACK ack);
```

后 3 个枚举控制 START、STOP、ACK；必须按同版本官方 `i2c_controller_rw_repeated_start_fifo_interrupts` 示例和目标器件 datasheet 组合。不要猜枚举整数。官方某些 polling 示例还包含特定器件勘误延时；只在对应器件/SDK 勘误适用时使用，不能复制成所有 I2C 的固定步骤。

## 13. J — UART：什么时候用

用于调试输出、与上位机/串口屏/通信模块交换数据。baud、数据位、校验、停止位、TX/RX PinMux 和 FIFO 阈值通常由 SysConfig 配置。

当前 MSPM0G3507 头文件中通用 API 是 `DL_UART_*`；`dl_uart_main.h` 还把 `DL_UART_Main_*` 重定向到这些通用实现。看到两种写法先看工程 include 和官方同器件示例，不要混用不存在的后缀。

```c
void DL_UART_transmitDataBlocking(UART_Regs *uart, uint8_t data);
uint8_t DL_UART_receiveDataBlocking(const UART_Regs *uart);
bool DL_UART_transmitDataCheck(UART_Regs *uart, uint8_t data);
bool DL_UART_receiveDataCheck(const UART_Regs *uart, uint8_t *buffer);
uint32_t DL_UART_fillTXFIFO(
    UART_Regs *uart, const uint8_t *buffer, uint32_t count);
uint32_t DL_UART_drainRXFIFO(
    const UART_Regs *uart, uint8_t *buffer, uint32_t maxCount);
bool DL_UART_isBusy(const UART_Regs *uart);
```

**参数：**`uart` 是生成实例；`data` 是一个 byte；接收 `buffer` 是单字节输出地址；FIFO `buffer` 是数组，`count/maxCount` 是 byte 数，返回实际数量。

- Blocking 版可能无限等待，适合短 bring-up，不适合主采样循环。
- Check 版立即成功/失败，适合状态机或 ISR。
- `fillTXFIFO` 只填当前可用 FIFO，不表示所有 byte 已经上线路；用返回值推进数组索引。
- `DL_UART_isBusy()` 反映发送器仍忙，常用于确认最后一位已发完；它不是“RX 是否有数据”。
- 需要大量持续数据时用 interrupt/DMA/ring buffer，不要在 ADC 实时链路中逐字符 blocking 打印。

```c
static void uart_send_bytes(const uint8_t *data, uint32_t length)
{
    for (uint32_t i = 0U; i < length; ++i) {
        DL_UART_Main_transmitDataBlocking(UART_0_INST, data[i]);
    }
    while (DL_UART_Main_isBusy(UART_0_INST)) {}
}
```

若当前工程只 include `dl_uart.h`，使用对应的 `DL_UART_...`；若生成配置/官方例程使用 `DL_UART_Main_...`，保持该工程风格。

中断家族为 `DL_UART_enableInterrupt`、`DL_UART_getPendingInterrupt`、`DL_UART_clearInterruptStatus`；外设 mask 与 NVIC IRQ 都要启用。RX ISR 应尽快 drain FIFO，耗时解析放到主循环。

## 14. K — Comparator：什么时候用

把模拟电压与参考比较，得到干净数字边沿，用于硬件测频、过零、阈值触发、窗口比较和保护事件。模拟输入路由、模式、滞回、滤波、参考源和 Event 通常由 SysConfig 配。

常用运行时 API：

```c
void DL_COMP_enable(COMP_Regs *comp);
void DL_COMP_disable(COMP_Regs *comp);
void DL_COMP_setDACCode0(COMP_Regs *comp, uint32_t value);
DL_COMP_OUTPUT DL_COMP_getComparatorOutput(COMP_Regs *comp);
DL_COMP_IIDX DL_COMP_getPendingInterrupt(COMP_Regs *comp);
```

`value` 是内部 comparator DAC code，不是伏特；有效意义由 reference mode/source 和 DAC 配置决定。改变阈值前先按 `dl_comp.h` 和 datasheet 计算。硬件测频优先调用正式 Comparator + Timer Capture 链。

## 15. L — VREF：什么时候用

当 ADC、DAC 或 Comparator 使用片内 1.4 V/2.5 V 参考时需要。VREF 是共享模拟资源，稳定时间和负载关系会影响精度，默认交给 SysConfig。

真实常用 API 包括 `DL_VREF_enablePower(VREF)`、`DL_VREF_reset(VREF)`、`DL_VREF_configReference(VREF, &config)`、`DL_VREF_enableInternalRef(VREF)`、`DL_VREF_enableInternalRefADC(VREF)`、`DL_VREF_enableInternalRefCOMP(VREF)`、`DL_VREF_getStatus(VREF)`。`DL_VREF_Config` 中包含 enable、1.4/2.5 V buffer 配置和 sample/hold 参数。

不要在 ADC/DAC 使用过程中随意 disable/reset VREF；也不要因为 SysConfig 选了内部参考就把算法 VREF 常量当作精密实测值。高精度要校准。

## 16. M — OPA：什么时候用

使用 MSPM0G3507 片上运放做 buffer、PGA、反相/同相放大或内部模拟链。常见 `DL_OPA_enablePower`、`DL_OPA_reset`、`DL_OPA_enable/disable`、`DL_OPA_setGain`、`DL_OPA_setNonInvertingInputChannel`、`DL_OPA_setInvertingInputChannel`、`DL_OPA_isReady` 均真实存在。

但 OPA 的输入 MUX、增益、rail-to-rail、GBW、chopping 和输出 pin 是相互关联的模拟配置，官方 OPA 示例主要依靠 `SYSCFG_DL_init()` 完成。正常应用不要在 `main.c` 临时拼完整模拟路径；使用正式前端模块/SysConfig profile，并结合输入共模、输出摆幅、GBW 和稳定性实板验证。

## 17. N — GPAMP：什么时候用

使用通用可编程放大器做模拟缓冲/增益和 ADC 前置处理。当前头文件真实 API 包括 `DL_GPAMP_enable/disable()`、`DL_GPAMP_setInvertingInputChannel()`、`DL_GPAMP_enableNonInvertingInputChannel()`、`DL_GPAMP_setRailToRailInputMode()`、`DL_GPAMP_enableOutputToPad()`。

注意 GPAMP 一些 API 没有实例指针，因为该器件上的寄存器组织与 OPA 不同。不能据此照搬 OPA 调用。当前官方 GPAMP 示例同样以 SysConfig 生成初始化为主；模拟性能和引脚可用性必须以 MSPM0G3507 datasheet/SysConfig 为准。

## 18. O — Event：什么时候用

让一个外设直接触发另一个外设，不经过 CPU，例如 Timer 周期事件 → ADC 转换、ADC result → DMA、Timer → DAC FIFO 更新、Comparator → Timer Capture。

MSPM0 DriverLib 没有一个通用 `dl_event.h` 让你随意接线；publisher/subscriber API 分散在各外设头文件，例如：

```c
DL_TimerG_setPublisherChanID(
    timer, DL_TIMERG_PUBLISHER_INDEX_0, channelId);
DL_ADC12_setSubscriberChanID(adc, channelId);
DL_ADC12_setPublisherChanID(adc, channelId);
DL_DMA_setSubscriberChanID(
    DMA, DL_DMA_SUBSCRIBER_INDEX_0, channelId);
```

不同函数参数并不完全相同：上例 Timer 和 DMA 还要选择 publisher/subscriber **寄存器索引**，而 ADC12 的这个接口只有 channel ID。event mask、channel ID、publisher index、subscriber index 和 DMA channel number 都不是同一概念。默认让 SysConfig 生成整条路由，并在生成的 `.c/.h` 中核对；手工改一端很容易形成“channel 数字相同但 event 未 enable”的半连接。

## 19. P — Interrupt / NVIC：什么时候用

外设产生条件后让 CPU 立即处理少量工作，例如 DMA done、UART RX、Timer zero、GPIO edge。它有两层：

1. DriverLib 外设层：`DL_xxx_enableInterrupt(instance, mask)`。
2. Cortex-M NVIC 层：`NVIC_EnableIRQ(生成的_INST_INT_IRQN)`。

常见形态：

```c
SYSCFG_DL_init();
NVIC_ClearPendingIRQ(UART_0_INST_INT_IRQN);
NVIC_EnableIRQ(UART_0_INST_INT_IRQN);

void UART_0_INST_IRQHandler(void)
{
    switch (DL_UART_Main_getPendingInterrupt(UART_0_INST)) {
        case DL_UART_MAIN_IIDX_RX:
            /* 只做快速搬运/置标志 */
            break;
        default:
            break;
    }
}
```

IIDX 枚举名随外设/别名而变，必须从当前头文件和生成工程中取。`clearInterruptStatus(instance, mask)` 的 mask 不是 IIDX。ISR 中避免 blocking IO、浮点 FFT 和长 delay；共享标志通常要 `volatile`，复杂并发还需临界区。

`NVIC_EnableIRQ`、`NVIC_ClearPendingIRQ`、`__WFI()` 属于 Cortex CMSIS 核心接口，不是 `DL_` DriverLib，但在 TI 示例中经常与 DriverLib 一起出现。

## 20. Q — MathACL：什么时候用

需要直接使用 MSPM0G3507 数学加速器执行定点乘、除、平方、MAC、平方根、sin/cos/atan2 时使用。普通信号算法优先使用正式算法模块及既定 backend，**不要因本文修改算法内部 backend**。

```c
typedef struct {
    DL_MATHACL_OP_TYPE opType;
    DL_MATHACL_OPSIGN opSign;
    uint32_t iterations;
    uint32_t scaleFactor;
    DL_MATHACL_Q_TYPE qType;
} DL_MathACL_operationConfig;

DL_MathACL_startMpyOperation(MATHACL, &config, op1, op2);
DL_MathACL_startDivOperation(MATHACL, &config, numerator, denominator);
DL_MathACL_waitForOperation(MATHACL);
uint32_t result = DL_MathACL_getResultOne(MATHACL);
```

`startMpyOperation`/`startDivOperation` 是到 `DL_MathACL_configOperation` 的宏。配置必须准确说明 operation、signed/unsigned、Q 格式、迭代和除法 scale；输入输出是 bit pattern，不能把 `float` 直接强转。MAC/SAC 前当前头文件要求 `DL_MathACL_clearResults()`。启用 `--mathacl` 编译器选项时还要遵守头文件关于 reset/divide 的专门说明。

## 21. 其他会常见但不应优先手写的 API

| 类别 | 可能看到 | 初学者边界 |
|---|---|---|
| WWDT/IWDT | `DL_WWDT_*`、`DL_IWDT_*` | 系统可靠性；先基于官方 example 配置，喂狗不能掩盖死锁 |
| FlashCTL | `DL_FlashCTL_*` | 校准/参数持久化；擦写有地址、对齐、寿命和执行区限制 |
| CRC | `DL_CRC_*` | 通信/存储校验，不是模拟测量精度 |
| RTC | `DL_RTC_*` | 长时间日历/低功耗计时，不用于高精度采样触发 |

它们不是本信号库当前最常用底层接口；需要时按下一节方法查询官方资料，不凭经验补 API。

## 22. 看到陌生 `DL_...` 函数时的固定查法

1. **看前缀。** `DL_SPI_` 就是 SPI，`DL_ADC12_` 就是 ADC12。
2. **找头文件。** 在当前 SDK `source/ti/driverlib/` 找 `dl_spi.h`、`dl_adc12.h` 等。SYSCTL 位于器件家族子目录，但包含入口仍由 DriverLib 处理。
3. **精确搜索函数声明。** 用 CCS “Open Declaration” 或 `rg -n "DL_SPI_xxx" C:\ti\mspm0_sdk_2_11_00_07\source\ti\driverlib`。
4. **从声明往上读完整 Doxygen。** 看 `@param`、单位、范围、返回值、`@pre/@post/@note` 和相关 API。也可离线打开中文官方 API Guide：`C:\ti\mspm0_sdk_2_11_00_07\docs\chinese\driverlib\mspm0g1x0x_g3x0x_api_guide\html\index.html`。
5. **追参数类型。** `DL_SPI_...` 枚举、mask、struct 都在同一头文件；不要输入数字试运气。
6. **在当前芯片示例里搜真实调用。** 搜索根：`C:\ti\mspm0_sdk_2_11_00_07\examples\nortos\LP_MSPM0G3507\driverlib`。
7. **检查生成代码所有权。** 如果调用已在 `ti_msp_dl_config.c`，应用通常不要再初始化；只调用运行时动作。
8. **回到 datasheet/TRM。** DriverLib 参数说明“如何写硬件”，片外器件协议和模拟限制仍由器件资料决定。

## 23. 五个真实代码阅读案例

### 案例 1：`DL_Common_delayCycles(1000)`

在 `dl_common.h` 搜到声明后，向上读到：参数是 cycle 下限、0 是最大延时、非精确延时、精确定时推荐 GPTimer。然后查生成的 `CPUCLK_FREQ` 才能估算时间。结论：不能把 `1000` 读成 `1000 us`。

### 案例 2：`DL_GPIO_setPins(PORT, PIN)`

声明告诉你 `PORT` 是 `GPIO_Regs *`、`PIN` 是 `DL_GPIO_PIN` 位掩码；inline 实现写 `DOUTSET31_0`。再看 `ti_msp_dl_config.h` 确认自己工程的 port/pin 宏。结论：它是原子“置高选中位”，不是 GPIO 初始化。

### 案例 3：`DL_SPI_transmitDataBlocking8(SPI, data)`

头文件说明它等待 TX FIFO 空间，写入后还等 SPI 不 busy。再看 `spi_controller_multibyte_fifo_poll` 官方示例，确认初始化在 `SYSCFG_DL_init()`。结论：适合短事务；接收仍需 controller 发送来产生 clock。

### 案例 4：`DL_ADC12_getMemResult(ADC, IDX)`

声明表明第二参数是 `DL_ADC12_MEM_IDX`，inline 实现读取对应 `MEMRES[idx]`；生成头将 `ADC12_0_ADCMEM_0` 映射为 `DL_ADC12_MEM_IDX_0`。结论：IDX 不是物理 ADC channel，结果单位也不是 volt。

### 案例 5：`DL_TimerG_startCounter(TIMER)`

在 `dl_timerg.h` 会看到它重定向到通用 `DL_Timer_startCounter`；后者只设置 counter enable 位。Timer 的 mode/clock/load 已由 SysConfig 生成。结论：start 不会替你配置采样率，采样率来自此前的时钟、分频和 load。

## 24. 最后检查清单

调用一个 DriverLib API 前，只问这 8 件事：

1. 当前 SDK 头文件里真的存在吗？
2. 它属于哪个外设？
3. 参数单位、枚举、范围和返回值是什么？
4. 传的是外设实例、PINCM、pin mask、IIDX，还是 interrupt mask？
5. `SYSCFG_DL_init()` 是否已经完成静态初始化？
6. 官方 LP-MSPM0G3507 示例怎样调用？
7. blocking 调用会不会卡住实时采样？
8. 仓库是否已有正式 Signal Module 应该优先复用？

## 25. 本文依据

- DriverLib headers：`C:\ti\mspm0_sdk_2_11_00_07\source\ti\driverlib\`
- 中文官方 DriverLib API Guide：`C:\ti\mspm0_sdk_2_11_00_07\docs\chinese\driverlib\mspm0g1x0x_g3x0x_api_guide\html\index.html`
- 英文官方 DriverLib API Guide：`C:\ti\mspm0_sdk_2_11_00_07\docs\english\driverlib\mspm0g1x0x_g3x0x_api_guide\html\index.html`
- SYSCTL family header：`...\m0p\sysctl\dl_sysctl_mspm0g1x0x_g3x0x.h`
- LP-MSPM0G3507 examples：`C:\ti\mspm0_sdk_2_11_00_07\examples\nortos\LP_MSPM0G3507\driverlib\`
- 已核对代表例程：GPIO toggle/software poll、TIMG periodic/capture、ADC12 single/DMA、DAC12 fixed/DMA、SPI controller FIFO poll、I2C controller FIFO poll/repeated-start、UART FIFO poll、Comparator-to-Timer、OPA/GPAMP-to-ADC、MathACL multiply/divide。
- 当前项目生成证据：各应用/示例的 `.syscfg` 和生成 `ti_msp_dl_config.c/.h`。

若本文与新 SDK 的本地头文件冲突，以新 SDK 头文件、对应器件示例、datasheet/TRM 和 release notes 为准，并同步修订本文。
