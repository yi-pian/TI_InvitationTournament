# MSPM0G3507 SysConfig 新手手动配置教程

这份文档解决一个问题：模块 README 告诉你“需要 ADC、Timer、DMA、Event、PinMux”以后，你怎样在 CCS 的 SysConfig 图形界面里真正把它们配出来。

本文只以当前仓库和当前环境为准：

- 芯片/板卡：`MSPM0G3507`、`LQFP-64(PM)`、`LP_MSPM0G3507`
- SDK：`MSPM0 SDK 2.11.00.07`
- 已检查配置：`09_examples/integration_profiles/PROFILE_01` 到 `PROFILE_06`
- Application：`08_applications` 的 projectspec 实际链接上述 Profile，没有另一套 Application 私有 `.syscfg`

> 最重要的规则：`.syscfg` 是配置源。`ti_msp_dl_config.c` 和 `ti_msp_dl_config.h` 是生成文件，不要手改。保存 `.syscfg`、重新 Generate/Build 后，你对生成文件的手工修改会被覆盖。

现场只想看操作入口时，打开 [SYSCONFIG_QUICK_REFERENCE.md](SYSCONFIG_QUICK_REFERENCE.md)。

如果你还分不清 `MCLK`、Timer Clock、Timer Event、ADC Clock、ADC `Fs` 和 DAC `Fupdate`，先读专门的 [MSPM0_SYSCONFIG_CLOCK_TIMER_ADC_DAC_BEGINNER_GUIDE.md](MSPM0_SYSCONFIG_CLOCK_TIMER_ADC_DAC_BEGINNER_GUIDE.md)；只想选择采样率时读 [SAMPLE_RATE_SELECTION_GUIDE.md](SAMPLE_RATE_SELECTION_GUIDE.md)，比赛现场回算用 [CLOCK_TIMER_ADC_DAC_QUICK_REFERENCE.md](CLOCK_TIMER_ADC_DAC_QUICK_REFERENCE.md)。

SysConfig 负责“怎样生成静态硬件配置”；生成后怎样调用 `DL_GPIO_xxx`、`DL_SPI_xxx`、`DL_ADC12_xxx` 等运行时 API，见 [TI_DRIVERLIB_BEGINNER_GUIDE.md](TI_DRIVERLIB_BEGINNER_GUIDE.md)。

## 0A. 所有模块都从同一份初始母版开始

本库的统一起点是 [`08_applications/signal_contest_template/signal_contest_template.syscfg`](../08_applications/signal_contest_template/signal_contest_template.syscfg)。它已经包含：

| 母版已有项 | 在 GUI 中的作用 | 处理规则 |
|---|---|---|
| `ProjectConfig` | 工程生成选项，含 CMSIS-DSP 库 | 保留；不要为了某个模块重新添加第二个 ProjectConfig |
| `Board` | LP-MSPM0G3507 板级识别 | 保留；不要用别的板卡替换 |
| `SYSCTL` | 系统时钟树与默认时钟配置 | 保留；外设教程只在它的基础上补充外设 |

操作顺序固定为：

1. 复制/Import 比赛工程后，双击工程自己的 `.syscfg`；不要新建第二份 `.syscfg`。
2. 确认设备为 `MSPM0G3507`、封装为 `LQFP-64(PM)`，并在 Software 视图保留上述三个母版实例。
3. 按模块 README 的“Required resources”逐个点击 `Add`，使用 GUI 左侧显示的模块名（如 `ADC12`、`TIMER`、`DMA`、`EVENT`、`GPIO`、`SPI`、`DAC12`、`COMP`、`OPA`）。`DL_*` 是 C API/枚举前缀，不是 Add 菜单项。
4. 先选硬件实例，再配置 Basic/Advanced、DMA、Event、Interrupt 和 PinMux 页面。涉及同一条链时，先配置生产者，再配置消费者；Event 的 Publisher/Subscriber Channel ID 必须相同。
5. 保存后点击 Generate/保存触发生成，检查 Problems/资源冲突，再 Clean/Build。只有 `.syscfg`、生成文件和完整工程都通过，才算完成配置；不要手改 `ti_msp_dl_config.c/.h`。

下面的资源表是 20 个需要在模块 README 中配置或明确分支的正式模块的完整入口。`adc_continuous`、`opa_to_adc`、`trigger_capture` 的说明同时给出“无需本模块新增硬件”的分支，避免把上游资源误加到错误模块。

### README 中的 GUI 路径写法

文档中的 `A → B → C → 字段 → 选项` 表示 CCS SysConfig 左侧 Software 树和右侧配置页的逐级展开顺序，不是代码调用顺序。例如 ADC 输入必须按下面的层级找：

```text
ADC12
└─ Basic Configuration
   └─ Sampling Mode Configuration
      └─ ADC Conversion Memory Configurations
         └─ ADC Conversion Memory 0 Configuration
            └─ Input Channel = Channel 2
```

不同 SDK 小版本可能把 `Sampling Mode Configuration` 折叠在 Basic 页面或把复数形式省略；遇到这种情况，沿着同一外设实例的 Basic 页面展开包含 `ADC Conversion Memory` 的折叠项即可。README 会同时给出父页面、字段名和要达到的值，不能只写一个 DriverLib 枚举名。

| 模块 | 从母版新增的 GUI 模块 | 图形化配置重点 |
|---|---|---|
| `adc_continuous` | 按 backend：无 / `ADC12` / `ADC12`+`DMA`+`TIMER`+`EVENT` | 先选 backend，再按对应链配置；不能三个分支一起加 |
| `adc_dma` | `ADC12`、`DMA`、`TIMER`、`EVENT` | ADC MEM0 → DMA request；Timer ZERO publisher → ADC subscriber |
| `adc_dual_sync` | 两个 `ADC12`、`TIMER`、`EVENT`（硬件适配器另需两 DMA channel） | 两 ADC 共用同一 Timer 事件，各自 MEM0/Pin/搬运通道独立 |
| `adc_fifo_dma` | `ADC12`、`DMA` | ADC FIFO/连续转换、DMA threshold；不添加 Timer/Event |
| `adc_pingpong_dma` | `ADC12`、`DMA`（定时后端再加 `TIMER`、`EVENT`） | 两个 buffer 是软件对象；SysConfig 只配置一次 ADC→DMA 搬运链 |
| `button` | `GPIO` | 一个输入 pin、方向、上拉/下拉、PinMux；消抖周期不在 SysConfig |
| `comparator_threshold` | `COMP` | 输入正/负端、参考/阈值、迟滞、滤波、可选 output event |
| `comparator_zero_cross` | `COMP`；测频时再加 `CAPTURE`/`TIMER-CAPTURE` 与 `EVENT` | 比较器输出边沿接 Capture subscriber，通道号成对设置 |
| `dac_dma` | `DAC12`、`DMA`、`TIMER`、`EVENT` | DAC FIFO/HWTRIG0、DMA block→fixed、Timer 更新事件 |
| `latching_button_switch` | `GPIO` | 一个输入 pin、稳定电平/上拉、PinMux；锁存状态是软件逻辑 |
| `matrix_keypad_4x4` | `GPIO` | 4 个行输出 + 4 个列输入，八根线逐 pin 检查冲突 |
| `opa_dac_bias` | `DAC12`、`OPA` | DAC 参考与 code、OPA 输入/反馈/输出路由；固定偏置不加时序外设 |
| `opa_inverting` | `OPA` | 反相 topology、PSEL/NSEL/MSEL、反馈/偏置与输出路由 |
| `opa_noninverting_pga` | `OPA` | 同相/PGA topology、增益档、输入/反馈/输出路由 |
| `opa_to_adc` | 本 helper 不新增；真实 OPA→ADC 链按 OPA 与 ADC README 分别配置 | 不能从范围检查函数反推硬件字段 |
| `rotary_encoder` | `GPIO` | A/B（可选 SW）输入、上下拉、极性、PinMux；扫描周期是应用调度 |
| `tft_ili9341` | `SPI`、`GPIO` | SPI SCLK/PICO/CS0，DC/BLK 输出，SPI 时钟/baud 与 PinMux |
| `tft_waveform` | 复用 `tft_ili9341` 的 `SPI`+`GPIO` | 不新增 Timer/DMA/Event；采样时基属于上游 ADC |
| `timer_capture` | `CAPTURE`/`TIMER-CAPTURE`，以及实际比较器链所需的 `COMP`+`EVENT` | Capture 时钟/周期、Trigger、subscriber port/channel、边沿与中断 |
| `trigger_capture` | 本模块不新增；只配置上游采集模块 | 这是已采 raw buffer 的软件搜索，不能虚构 Timer 页面 |

模块 README 中的示例 Pin、instance 和 channel 只用于说明字段位置；如果当前工程已有 owner，必须保留现有资源或先处理冲突，不能整段复制 profile 覆盖母版。

## 0. 先选一份最接近的真实 Profile

从空白开始学习可以，但做比赛工程时优先复制最接近的已验证资源组合，再在 GUI 中删减和修改。

| 硬件链 | 参考 Profile | 当前真实资源 | 当前证据 |
|---|---|---|---|
| 单 ADC 定时 DMA 采集 | `PROFILE_01_ADC_CAPTURE` | ADC0.2/PA25、DMA_CH0、TIMG0、Event 1、UART0 | SysConfig/compile/link PASS |
| 双 ADC | `PROFILE_02_DUAL_ADC` | ADC0.2/PA25 + ADC1.2/PA17、DMA_CH0/1、TIMG0、Event 1/2 | SysConfig/compile/link PASS |
| DAC DMA 发生 | `PROFILE_03_DAC_GENERATOR` | DAC0/PA15、DMA_CH1、TIMG6、Event 3 | SysConfig/compile/link PASS |
| ADC + DAC | `PROFILE_04_ADC_DAC` | P01 + P03 | SysConfig/compile/link PASS |
| 比较器测频 | `PROFILE_05_FREQUENCY` | COMP0/PA27、Event 4、TIMG6 Capture | SysConfig/compile/link PASS |
| 双 ADC + DAC + Capture | `PROFILE_06_FULL_SIGNAL` | DMA_CH0/1/2、TIMG0/6/7、Event 1..4 | SysConfig/compile/link PASS |

这些状态不等于 `BOARD_VERIFIED`。P01–P06 的验证脚本没有烧写开发板。

当前 `08_applications` projectspec 的真实引用关系如下；同一应用的 Q31/其他 Backend 变体仍使用同一硬件 Profile：

| Application | 链接的 Profile |
|---|---|
| `signal_meter`、`spectrum_analyzer`、`harmonic_thd_analyzer` | P01 |
| `frequency_meter` Method A | P05 |
| `frequency_meter` Method B/C | P01 |
| `dual_channel_phase_meter` | P02 |
| `dds_generator` | P03 |
| `sweep_analyzer`、`waveform_capture_replay` | P04 |
| `signal_analyzer` | P02 |
| `signal_contest_template` | P06 |
| `peripheral_system_template` | P01 |

## 1. 打开 SysConfig 后每个区域是做什么的

### 1.1 打开方法

1. 在 CCS Project Explorer 中找到工程链接进来的 `profile.syscfg`。
2. 双击它，等待 SysConfig 编辑器加载设备和 SDK 产品。
3. 先确认文件顶部/编辑器中的 Device 是 `MSPM0G3507`，Package 是 `LQFP-64(PM)`，Board 是 `LP_MSPM0G3507`。
4. 如果界面窄，Software、Hardware、PinMux、Problems 等面板可能折叠成标签；名称不变，但位置会随 CCS/SysConfig 窗口布局变化。

### 1.2 Software

这里添加 DriverLib 配置模块，例如 `ADC12`、`TIMER`、`CAPTURE`、`DAC12`、`COMP`、`UART`、`GPIO`、`OPA`、`GPAMP`、`VREF`。

最常见操作是：

1. 点 Software。
2. 在搜索框输入外设名称。
3. 点 `+`/Add 添加模块实例。
4. 点已添加的实例，在右侧修改它的 Basic、Advanced、Interrupt、DMA、Event 配置。

“模块”与“实例”不要混淆：`ADC12` 是配置模块；`SIGNAL_ADC` 是仓库给某一个 ADC 实例起的名字；`ADC0` 是实际硬件外设。

### 1.3 Hardware

Hardware 用于板级器件或板卡资源视图。普通信号工程的大多数 DriverLib 外设从 Software 添加；Hardware 更适合查看板载 LED、按键、调试串口等板级对象。不要因为 Hardware 中没看到 ADC 就认为芯片没有 ADC。

### 1.4 Peripheral 实例页

点一个已添加实例后，右侧配置页通常包含：

- Quick Profiles：快速预设；选完仍要逐项检查。
- Basic Configuration：时钟、工作模式、通道等主要参数。
- Advanced Configuration：采样时间、滤波、分频等高级参数。
- Interrupts Configuration：哪些条件产生 CPU 中断。
- DMA Configuration：是否让外设向 DMA 发请求。
- Event Configuration：Event Fabric 的 Publisher/Subscriber 通道。
- PinMux/Pin Configuration：硬件实例和封装引脚。

### 1.5 Clock / Clock Tree

Clock Tree 显示 SYSOSC、BUSCLK、ULPCLK、LFCLK、MFPCLK 等来源和计算结果。修改 Timer、ADC、UART、DAC 前，先看其页面中的 Calculated Clock/Actual Period，不要只凭 CPU 主频猜外设时钟。

当前 P01 的采样 Timer 使用 `BUSCLK / 1 / 1`。生成结果显示计数时钟为 32 MHz，`10 us` 周期生成 Load=`319`。

### 1.6 Event

MSPM0 Event Fabric 让一个外设直接触发另一个外设，不必先让 CPU 进中断。

你通常不需要添加一个“Event 外设”。在生产者实例和消费者实例各自的 `Event Configuration` 中设置：

```text
Publisher：谁发布事件、发布到哪个 Channel ID
Subscriber：谁订阅同一个 Channel ID
```

例如 P01：TIMG0 的 ZERO_EVENT 发布到 Channel 1，ADC0 的 Event Subscriber Channel ID 也选 1。

### 1.7 DMA

在 ADC12/DAC12 实例中勾选 `Configure DMA Trigger` 后，SysConfig 会创建一个从属 DMA Channel 配置。这里选择 DMA_CHx、方向、宽度和传输模式。

注意：`raw` 地址、波表地址和 N 通常在运行时由正式模块设置，不是在 SysConfig 里写死。

### 1.8 Interrupt

Interrupt 是“让 CPU 来处理”的通知；Event 是“外设直接通知外设”。两者可以同时存在。

P01 的逐点采样不进 CPU；ADC 在整帧 DMA 完成后用 `DL_ADC12_INTERRUPT_DMA_DONE` 通知 CPU。

### 1.9 PinMux

PinMux 显示外设信号最终占用哪个封装引脚，并立即检查冲突。ADC/DAC/Comparator/OPA/GPAMP 的模拟连接也会在这里体现，但有些片上内部路由不占外部 Pin。

### 1.10 Generated Files

Generated Files 是生成结果的预览/输出入口。至少会生成：

- `ti_msp_dl_config.h`：实例、Pin、IRQ、DMA Channel、Load 等宏。
- `ti_msp_dl_config.c`：时钟、PinMux、外设和中断初始化。

正确工作流：改 `.syscfg` → 保存 → Generate/Build → 看 Problems → 必要时只读检查生成文件。不要反过来编辑生成文件。

<a id="pinmux"></a>
## 2. PinMux / 引脚配置保姆教程

### 2.1 怎样知道某外设可以用哪些 Pin

1. 先在 Software 中添加并选择外设实例。
2. 在实例的 PinMux/Pin Configuration 中选择具体硬件实例，例如 ADC0、UART0。
3. 展开该外设信号的 Pin 下拉框。这里列出的才是“当前 MSPM0G3507 + LQFP-64 封装 + 已占用资源”下可选的 Pin。
4. 灰色/不可选项通常表示该 Pin 不支持该信号或已被其他 owner 占用。
5. 再用 LP_MSPM0G3507 原理图确认这个 MCU Pin 是否真的引到排针、是否与板载器件相连。

不要从别的 MSPM0 型号、别的封装或 STM32 经验抄 Pin 表。SysConfig 当前设备的下拉框是第一依据，板卡原理图是第二依据。

### 2.2 真实 Profile 中已用 Pin

| 用途 | 实例/通道 | 当前 Pin | 来源 |
|---|---|---|---|
| 单 ADC 输入 | ADC0 MEM0 Channel 2 | PA25 | P01/P04/P06 |
| 第二 ADC 输入 | ADC1 MEM0 Channel 2 | PA17 | P02/P06 |
| DAC 输出 | DAC0 | PA15 | P03/P04/P06 |
| Comparator 外部负输入 | COMP0 IN0- | PA27 | P05/P06 |
| UART0 TX | UART0 TX | PA10 | P01–P06 |
| UART0 RX | UART0 RX | PA11 | P01–P06 |

这张表只说明现有 Profile 的真实选择，不表示只有这些 Pin 可用。其他候选必须在当前 SysConfig 下拉框和板卡原理图中确认。

### 2.3 Pin conflict 是什么

一个普通封装 Pad 同一时刻通常只能被一个外设功能驱动/采样。例如 PA15 已作为 DAC_OUT，就不能同时作为普通 GPIO 输出。SysConfig 看到两个 owner 抢同一 Pin 会报 Resource conflict。

解决步骤：

1. 点错误信息，记下 Pin 和两个 owner。
2. 判断哪一条功能链的 Pin 是硬约束，例如 DAC0 输出可能可选范围很少。
3. 优先移动更灵活的 GPIO、UART 或未接线的功能。
4. 在被移动实例的 PinMux 下拉框选另一项。
5. 同步修改物理接线。
6. 保存并重新生成，直到 Problems 无冲突。

不要用 `$assignAllowConflicts` 或 suppress 把真正的外部 Pad 冲突藏起来。TI 的 `gpamp_buffer_to_adc` SDK 例外使用冲突许可，是因为 GPAMP_OUT 与 ADC 输入有意在同一个 PA22 节点相连；这不适用于两个数字输出争用一脚。

### 2.4 Analog Pin 和 Digital Pin

- Analog 功能让 Pad 接入 ADC、DAC、Comparator、OPA/GPAMP 的模拟网络。不要同时打开数字输出；数字切换会污染模拟信号。
- Digital 功能用于 GPIO、UART、SPI、Timer Capture/PWM 等逻辑电平。
- 同一 PAxx 可能在器件上支持多种复用功能，但你的 `.syscfg` 只能选择当前所需的一种外部连接。

### 2.5 怎样分别换 ADC、DAC、UART、Comparator Pin

**ADC：**先改 `ADCMEM0 Configuration → Input Channel`，再在 PinMux 选择与该 Channel 匹配的 ADC Pin。不能只改 Channel 数字，也不能只换 PAxx。

**DAC：**在 DAC12 中启用 Output Pin，并在 PinMux 选择 DAC 可用输出。当前 P03 是 PA15；换 Pin 后必须改外部接线并重新 Generate。

**UART：**分别选择 TX 和 RX。当前 Profile 是 UART0 TX=PA10、RX=PA11。接电脑串口时还要确保 `Enable Internal Loopback=false`；当前 P01–P06 的 Quick Profile 生成了内部回环，只能作为资源/Build 基线，不能直接当外部串口终端已验证配置。

**Comparator：**先确定信号接正输入还是负输入，再启用对应输入通道并选择 Pin。P05 只启用了外部负输入 PA27，正端使用内部参考；因此它和“信号进正端”的边沿极性相反，应用必须根据实际极性解释边沿。

### 2.6 OPA/GPAMP 内部路由为什么不一定有 Pin

OPA/GPAMP 的输入多路器可以选外部 Pin，也可以选 `DAC8_OUT`、`VREF`、`RTOP/RTAP`、`GPAMP_OUT` 等片上节点。内部节点不经过封装 Pad，所以不会要求外接导线。

是否把输出送到外部，取决于 `OPA Output Pin` / `GPAMP Output Pin Enable`。如果只走内部链到 ADC，可不一定启用输出 Pin；但必须根据当前器件连接图和 ADC internal connection 选项确认。

### 2.7 换 Pin 后代码要不要改

- 应用只使用 `SIGNAL_ADC_INST`、`GPIO_...` 等 SysConfig 生成宏时，通常不用改底层调用代码。
- 如果实例 `$name` 改了，生成宏名也会变，正式模块可能编译失败。因此换 Pin 时保留 `SIGNAL_ADC`、`SIGNAL_SAMPLE_TIMER` 等实例名。
- 外部接线、README 接线表和测试点必须同步改。
- 如果代码硬编码了 PAxx/PinCM（不推荐），必须改代码；先搜索再 Build。

<a id="timer"></a>
## 3. Timer 保姆教程

### 3.1 先把名词说清楚

| 名词 | 小白解释 |
|---|---|
| Timer Clock | Timer 每秒得到多少个计数脉冲 |
| Clock Divider | 先把输入时钟除以一个比例 |
| Prescaler | 再做一次预分频，让低频周期能放进计数器 |
| Period / Load | 数到哪里归零/重装；SysConfig 可直接填期望时间 |
| Counter | 当前已经数到的值 |
| Compare | Counter 到指定值时产生动作，常用于 PWM/定时点 |
| Capture | 外部/事件边沿到来时锁存 Counter，用于测周期/脉宽 |
| Interrupt | Timer 条件满足后叫 CPU 处理 |
| Event | Timer 条件满足后直接通知另一个外设 |

### 3.2 产生固定 100 kHz 事件

目标频率与周期：

```text
T = 1 / F = 1 / 100000 = 10 us
Timer计数时钟 = 输入时钟 / Divider / Prescaler
计数次数 = Timer计数时钟 × T
Load = 计数次数 - 1
```

当前 P01 的真实配置：

1. Software → 添加 `TIMER`。
2. 实例名设为 `SIGNAL_SAMPLE_TIMER`。
3. Hardware Instance 选 `TIMG0`。
4. `Timer Clock Source` 选 `BUSCLK`。
5. `Timer Clock Divider=1`，`Timer Clock Prescaler=1`。
6. `Timer Mode=Periodic Down Counting`。
7. `Desired Timer Period=10 us`。
8. 检查 `Actual Timer Period`。
9. 展开 `Event Configuration`，`Event 1 Publisher Channel ID=1`。
10. 在 Event 1 条件中选 `ZERO_EVENT`。

当前生成结果的 Timer 计数时钟为 32 MHz，所以计数次数=320，Load=319。不要把 32 MHz 写成所有工程永久不变的事实；只要 Clock Tree、Divider 或 Prescaler 变了，就重新看 Calculated Timer Clock Values。

### 3.3 Timer 的三种常见角色

- 周期触发器：Periodic + ZERO_EVENT，例如固定 Fs 触发 ADC/DAC。
- Capture 计时器：边沿到来时锁存时间戳，例如 Comparator 测频。P05 使用 CAPTURE 模块的 Trigger 输入。
- PWM：用 Period 决定 PWM 基频，Compare 决定占空比；它和“只发布固定采样事件”不是同一套用途。

### 3.4 修改 Fs 到底改哪里

对于正式 `adc_dma` 和 DAC Platform，应用运行时可以重写 Timer Load。因此：

- 只改请求 Fs：改 `signal_config.h`/模块 config，并确认模块传入的真实 `timer_clock_hz`；通常不用改 PinMux/Event。
- 改 Timer Clock Source/Divider/Prescaler/实例：必须改 SysConfig，并同步软件里的 `timer_clock_hz`。
- 想让 Profile 默认生成就是新 Fs：改 `Desired Timer Period`，然后重新 Generate。

无论哪种方式，下游算法使用的 Fs 必须与 `SignalADC_GetConfiguredTriggerRate()` 或 DAC Platform 的实际配置率一致。

<a id="adc"></a>
## 4. ADC 保姆教程

### 4.1 ADC 页面各项是什么意思

| SysConfig 项 | 含义 | 当前 P01 |
|---|---|---|
| Hardware Instance | 使用 ADC0 还是 ADC1 | ADC0 |
| Input Channel | ADCMEMx 采哪个模拟通道 | MEM0 Channel 2 |
| Device Pin Name / PinMux | 通道对应的外部脚 | PA25 |
| Reference Voltage | raw 满量程对应哪一参考 | 当前默认 VDDA；Board 默认显示 3.3 V |
| Conversion Resolution | 8/10/12 bit | 当前默认 12 bit |
| Desired Sample Time 0 | 采样保持时间 | 62.5 ns |
| Conversion Mode | Single channel 或 Sequence | Single |
| Sampling Mode | Auto/Manual 控制采样时间 | 当前 Auto 默认 |
| Trigger Source | 软件调用还是 Event Fabric | Event |
| Repeat Mode | 一次触发链是否继续等待下一次 | 开启 |
| ADC Conversion Memory | 每个结果存放的 ADCMEM | MEM0 |
| Interrupts | 哪些 ADC 条件通知 CPU | DMA_DONE |
| DMA Triggers | 哪个 ADC 结果请求 DMA | MEM0_RESULT_LOADED |

当前 MSPM0G3507 ADC12 配置的结果入口是 ADC Conversion Memory（本链使用 ADCMEM0），不是一项名为“ADC FIFO”的通用队列。DAC12 才在 P03 中明确启用了 FIFO。读单点时读 MEM0；采一帧时 DMA 的 source 是 MEM0 result address。

12-bit + VDDA=3.3 V 时常见换算是 `V=raw/4095×3.3`，但只有在实际参考确实为 3.3 V、前端增益/偏置已计入时才成立。SysConfig 的 ADC reference 必须和 `ADC_ToVoltage` 配置同步。

### 4.2 示例 A：最简单的软件触发单次 ADC

这组步骤来自 SDK 2.11.00.07 的 `adc12_single_conversion` 示例，不是凭空设计。

1. 打开 `.syscfg` → Software → 搜索并添加 `ADC12`。
2. 实例名设为你的工程约定；如果要复用 `adc_dma`，不要用这个软件触发例子，应直接看下一节。
3. Hardware Instance 选 `ADC0`。
4. `ADC Clock Source=ULPCLK`；SDK 单次示例还用了 `Sample Clock Divider=Divide by 8`。
5. `Conversion Mode=Single`。
6. `Trigger Source=Software`。
7. `ADCMEM0 → Input Channel=Channel 2`，PinMux 选 PA25。
8. `Reference Voltage` 按硬件选 VDDA 或 VREF。
9. `Conversion Resolution=12-bits`。
10. 在 `Interrupts Configuration` 选择 `MEM0_RESULT_LOADED`，或者不用中断而在代码轮询。
11. 不勾 `Configure DMA Trigger`。
12. 保存并 Build；代码在需要时调用 ADC start conversion，再读 MEM0 结果。

为什么这样选：一次只要一个值，CPU 主动发起最简单，不需要 Timer/Event/DMA。它不适合固定 Fs 的 N 点波形。

### 4.3 示例 B：Timer 定时触发 ADC

1. 先按 Timer 章节建立 Periodic Timer。
2. Timer `Event 1 Publisher Channel ID` 选一个未占用 Channel，例如 P01 的 1；Event 条件选 ZERO_EVENT。
3. ADC `Trigger Source=Event`。
4. ADC `Event Subscriber Channel ID` 选择同一个 Channel。
5. `Enable Repeat Mode` 打开，让 ADC 接收连续 Timer 触发。
6. 只读少量点可用 MEM0 interrupt；采 N 点应继续配置 DMA。
7. 保存后看 Event Channel 是否没有冲突，Build 后检查生成宏。

这里的关键不是“两个地方都写 Event”，而是 Publisher ID 与 Subscriber ID 必须相同。

<a id="adc-timer-dma"></a>
## 5. 从空白 SysConfig 配出 ADC + Timer + Event + DMA → raw[N]

这是 `02_acquisition/adc_dma` 的正式硬件契约，按 P01 的真实字段和资源写成。

```text
TIMG0 ZERO_EVENT --Event Channel 1--> ADC0 trigger
ADC0 MEM0 result loaded ------------> DMA_CH0 request
ADC MEM0 result address --DMA-------> uint16_t raw[N]
DMA done ----------------------------> ADC IRQ/模块 DONE
```

### STEP 1：添加 ADC12

1. Software → 搜索 `ADC12` → Add。
2. 实例名 `$name` 设为 `SIGNAL_ADC`。正式 `signal_adc_dma.c` 依赖这个生成宏前缀。
3. Hardware Instance 选 `ADC0`。
4. `ADC Clock Source=ULPCLK`。
5. `Conversion Mode=Single`，Start/End Address 保持 MEM0。
6. `ADCMEM0 Input Channel=Channel 2`，PinMux=PA25。
7. `Desired Sample Time 0=62.5 ns`。
8. `Conversion Resolution=12-bits`、Data Format=Unsigned right-aligned。
9. `Trigger Source=Event`。
10. 开启 `Enable Repeat Mode`。
11. 将 MEM0 trigger mode 设为 Trigger Next（Profile 字段 `adcMem0trig=DL_ADC12_TRIGGER_MODE_TRIGGER_NEXT`）。

为什么：每个 Timer Event 只推进一次 MEM0 转换，得到一个 `uint16_t` raw code。

### STEP 2：配置 ADC 的 Event Subscriber

1. 在 `SIGNAL_ADC → Event Configuration` 展开。
2. `Event Subscriber Channel ID=1`。
3. Publisher 不需要 ADC 来做，所以保持 0，除非你的另一条链确实需要 ADC 发布事件。

为什么：Channel 1 是这条采样链的“专用门铃号码”。

### STEP 3：配置 ADC 的 DMA Trigger

1. 展开 `DMA Configuration`。
2. 勾选 `Configure DMA Trigger`。
3. 勾选/启用 DMA Trigger。
4. `DMA Samples Count=1`。
5. `Enable DMA Triggers` 选择 `MEM0_RESULT_LOADED`。
6. 在 Interrupts 中选择 `DMA_DONE`。

为什么：每产生一个 MEM0 结果，DMA 搬一个 half-word；N 次以后由 DMA_DONE 告诉模块一帧完成。`DMA Samples Count=1` 不是整帧 N。

### STEP 4：配置 DMA_CH0

1. 展开 ADC12 下自动出现的 DMA Channel。
2. 名称设为 `SIGNAL_ADC_DMA`。
3. Channel 选 `DMA_CH0`。
4. Address Mode 选 Peripheral to Block（Profile 字段 `f2b`）。
5. Source Length=`Half Word`，Destination Length=`Half Word`。
6. Transfer Mode=`Single`。

运行时正式模块会做：

- Source：`DL_ADC12_getMemResultAddress(...MEM0)`，即 ADC MEM0 结果寄存器地址。
- Destination：调用者传给 `SignalADC_Start(raw,N)` 的 `raw[0]`。
- Transfer Count：`N`。

因此不要在 SysConfig 里寻找固定 `raw` 地址，也不要把 N 错写成 `DMA Samples Count`。

### STEP 5：添加采样 Timer

1. Software → 添加 `TIMER`。
2. 实例名设为 `SIGNAL_SAMPLE_TIMER`。
3. Hardware Instance=`TIMG0`。
4. Clock Source=`BUSCLK`，Divider=1，Prescaler=1。
5. Timer Mode=`Periodic Down Counting`。
6. Desired Timer Period=`10 us`，即 Profile 默认 100 kHz。
7. Event 1 Publisher Channel ID=`1`。
8. Event 1 条件选 `ZERO_EVENT`。

### STEP 6：PinMux 和资源检查

确认：

- ADC0 Channel 2 是 PA25。
- DMA_CH0 没被别的模块使用。
- TIMG0 没被 PWM/Capture/另一条采样链使用。
- Event Channel 1 的 Publisher 是 TIMG0，Subscriber 是 ADC0。
- 没有第二个无关 owner 复用 Channel 1。

### STEP 7：Generate 与 Build 检查

保存 `.syscfg` 并完整 Build。只读检查生成的 `ti_msp_dl_config.h` 应出现：

- `SIGNAL_ADC_INST`
- `SIGNAL_ADC_DMA_CHAN_ID`
- `SIGNAL_SAMPLE_TIMER_INST`
- 对应 PA25/ADC channel、IRQ、Load 宏

当前 P01 生成 Load=319。出现 undefined `SIGNAL_ADC_*` 往往是实例名改错；出现 `ti_msp_dl_config.h not found` 是 SysConfig 未生成或 include path 问题。

### STEP 8：哪些参数在 SysConfig，哪些不在

| 参数 | 修改位置 | 原因 |
|---|---|---|
| ADC0/ADC1、Channel、Pin、Reference | SysConfig | 属于物理外设和连线 |
| Timer instance/clock/divider/prescaler | SysConfig | 决定真实计数硬件 |
| Event Channel、DMA Channel、DMA width/direction、IRQ | SysConfig | 属于片上资源路由 |
| N / raw 数组长度 | `signal_config.h`/Application | 运行时 DMA transfer count；RAM=`2N` bytes |
| 请求 Fs | `signal_config.h`/`SignalADC_Init` | 模块会重算 Timer Load |
| `timer_clock_hz` | Application config，必须与 SysConfig 同步 | 模块计算 Load 的依据 |
| VREF 数值、前端 gain/offset | ADC_ToVoltage config | raw→V 的数学换算；必须与硬件配置同步 |
| FFT N、窗口、阈值 | 算法 config | 不属于外设配置 |

<a id="dac"></a>
## 6. DAC 保姆教程

### 6.1 固定电压输出

SDK 2.11.00.07 的 `dac12_fixed_voltage_vref_internal` 使用 DAC12 + VREF；仓库 `dac_dc` 负责电压到 code，底层 `signal_dac`/平台负责写 DAC。

1. Software → 添加 `DAC12`。
2. 打开 `Enable DAC`。
3. 选择 Positive/Negative Voltage Reference。最简单可用 VDDA/VSSA；使用 VREF+/- 时还要添加 VREF 并正确接/配 PA23、PA21。
4. 打开 `DAC Output Pin`，在 PinMux 选择实际输出。当前 P03 为 PA15。
5. 固定电压不需要 FIFO、Timer、Event、DMA；由代码把计算后的 12-bit code 写入 DAC data register。
6. 用万用表/示波器检查 PA15；不要用“Build PASS”代替模拟电压验证。

理论码值：

```text
code = (Vout - Vneg) / (Vpos - Vneg) × 4095
```

必须限制 Vout 在真实参考范围内，并考虑 DAC 缓冲、负载和模拟误差。

### 6.2 Timer/Event/DMA 连续波形

按 P03：

```text
uint16_t wave[N] --DMA_CH1--> DAC FIFO --> PA15
TIMG6 ZERO_EVENT --Event Channel 3--> DAC HWTRIG0 每次取一个样点
DAC FIFO 低于阈值 -------------------> DMA 请求补数
```

操作：

1. DAC12：Output Pin enable、PA15、Amplifier ON。
2. Enable FIFO。
3. FIFO Trigger Source=`Hardware trigger 0 event fabric`（字段 `HWTRIG0`）。
4. Configure DMA Trigger=true；P03 FIFO Threshold=`2/4 locations empty`。
5. Event Subscriber Channel 0 ID=`3`。
6. DMA Channel 名称=`SIGNAL_DAC_DMA`，Hardware=`DMA_CH1`。
7. Address Mode=Block to Peripheral（`b2f`）。
8. Source/Destination Length 都是 Half Word。
9. 循环输出用 Full Channel Repeat Single；当前 G3507 的 repeat mode 需要 Full DMA Channel。
10. 添加 `SIGNAL_DAC_TIMER`：TIMG6、BUSCLK、Periodic、10 us。
11. Timer Event 1 Publisher Channel ID=3，条件 ZERO_EVENT。
12. Generate/Build 后由 `signal_dac_dma_platform` 在运行时设置 wave buffer、count 和 update rate。

### 6.3 三个频率不要混

| 名称 | 含义 | 公式/位置 |
|---|---|---|
| DAC 更新率 | 每秒输出多少个 DAC 样点 | Timer Event rate；例如 100 kS/s |
| 整周期表重复频率 | 一张含一周期的 N 点表循环多快 | `Fwave=Fupdate/N` |
| DDS 输出频率 | 32-bit 相位累加器推进多快 | `Fdds=tuning_word×Fupdate/2^32` |

DDS 可能跨表跳读，所以不能把 DDS 频率一律写成 `Fupdate/N`。DDS 的 `update_rate_hz` 必须与真实 DAC 更新率一致。

<a id="dma"></a>
## 7. DMA 保姆教程

| 项 | 小白解释 | ADC→RAM | RAM→DAC |
|---|---|---|---|
| DMA Channel | 一条独立搬运通道 | P01 DMA_CH0 | P03 DMA_CH1 |
| Source | 从哪里读 | ADC MEM0 result address | `uint16_t wave[]` |
| Destination | 写到哪里 | `uint16_t raw[]` | DAC FIFO/data register |
| Transfer Width | 每次搬几位 | Half Word | Half Word |
| Transfer Count | 搬多少个元素 | N，运行时设置 | count，运行时设置 |
| Trigger Source | 什么时候搬一次 | MEM0 result loaded | DAC FIFO 请求 |
| Address Mode | 哪边地址递增 | peripheral→block | block→peripheral |
| Single/Repeat | 一帧结束还是循环 | Single | Repeat Single 常用于循环波 |
| Completion | 整块结束通知 | ADC DMA_DONE/状态 DONE | one-shot 状态或平台 IRQ |

为什么不能共用同一个 DMA Channel：Channel 只有一套 source、destination、count、trigger 和状态。ADC 正在写 RAM 时，DAC 若重配同一 Channel，会直接破坏 ADC 搬运。

P06 的 DMA_CH0、DMA_CH1、DMA_CH2 已分别给 ADC-A、DAC、ADC-B，三条 Full Channel 全占用。添加新 DMA 功能前应先删除不使用的链，而不是随便指定一个重复 Channel。

<a id="event"></a>
## 8. Event 保姆教程

把 Event 当作芯片内部的“门铃线”：

```text
谁按门铃？       Publisher
门铃号码是多少？ Channel ID
谁听这个号码？   Subscriber
```

### 8.1 Timer → ADC

1. Timer Event Configuration：Publisher Channel ID=1；条件 ZERO_EVENT。
2. ADC Event Configuration：Subscriber Channel ID=1。
3. ADC Trigger Source=Event。

### 8.2 Comparator → Timer Capture

1. Comparator Event Configuration：Publisher Channel ID=4；事件 OUTPUT_EDGE。
2. Capture：`Capture Select=Trigger`、Subscriber Port=`FSUB0`、Subscriber Channel=4。

### 8.3 ADC result → DMA

ADC 到 DMA 的请求由 ADC `DMA Configuration → Enable DMA Triggers → MEM0_RESULT_LOADED` 配置。它和 Event Fabric 的 Timer→ADC Channel 1 是两层不同触发：

```text
Timer Event：决定何时开始一次 ADC conversion
ADC DMA trigger：决定结果出来后何时搬进 RAM
```

最常见错误：Publisher=1 而 Subscriber=2；选了 Channel 但没选事件条件；ADC 仍是 Software Trigger；两个无关 Publisher 使用同一 Channel。

<a id="comparator"></a>
## 9. Comparator + Timer Capture 教程

### 9.1 名词

- Positive/Negative input：比较器的 `+`、`-` 输入。
- Reference/Threshold：与信号比较的电压，可来自 VDDA DAC/VREF DAC/外部输入。
- Hysteresis：上下翻转阈值留一点间隔，防止噪声在阈值附近反复抖动。
- Output Filter：要求状态保持一定时间再认作边沿，可抑制窄毛刺，但会增加延迟。
- Event：比较器输出边沿直接送 Timer Capture。
- IRQ：需要 CPU 处理比较器本身事件时使用；仅做 Capture 可主要依赖 Event。

### 9.2 按 P05 建链

1. Software → 添加 `COMP`，实例名 `SIGNAL_COMP`，Hardware=`COMP0`。
2. 当前 P05 打开外部 Negative channel，Pin=`PA27`。
3. Mode=`ULP`，Hysteresis=`30`，Output Filter Delay=`1200`。
4. Reference Source=`VDDA DAC`，DAC Control=`Software`。
5. Event Publisher Channel ID=`4`，Enable Event=`OUTPUT_EDGE`。
6. Software → 添加 `CAPTURE`，实例名 `SIGNAL_CAPTURE`，Hardware=`TIMG6`。
7. Capture Select=`Trigger`，Subscriber Port=`FSUB0`，Subscriber Channel=`4`。
8. Enable Capture/overflow interrupts：P05 为 `CC0_DN` 和 `ZERO`。
9. Build 后确认 COMP Publisher 4 与 Capture Subscriber 4 一致。

```text
Analog signal on PA27 (COMP negative input)
    -> comparator threshold crossing
    -> digital output edge
    -> Event 4
    -> TIMG6 captures timestamp
    -> period ticks / timer_clock_hz
    -> frequency_hz
```

重要限制：P05 当前生成的内部 DACCODE0 是 0，且信号在负输入；它只证明 SysConfig/compile/link 闭合，不代表阈值、边沿极性和输入电气范围已经适合你的真实信号。上板前必须按偏置电压设置 threshold，并用函数发生器/示波器验证。

<a id="uart"></a>
## 10. UART 教程

### 10.1 最小外部串口配置

1. Software → 添加 `UART`，选择 `UART0`。
2. 设实例名，例如仓库约定 `SIGNAL_UART`。
3. Clock Source 选能满足目标 Baud 的时钟，并看 `Calculated Baud Rate` 警告。
4. `Target Baud Rate=115200`（当前 Profile）。
5. Data Bits=8、Parity=None、Stop Bits=1。
6. 只做简单发送时可开启 FIFO，不需要 DMA。
7. PinMux：TX=PA10、RX=PA11。
8. **外接串口时关闭 Enable Internal Loopback。**
9. 只发测量结果可以不开 RX interrupt；需要命令输入再打开 RX/FIFO interrupt。
10. 连接 USB-UART：MCU TX→转接器 RX，MCU RX→转接器 TX，GND→GND；电平必须兼容 3.3 V。

P01–P06 使用的 `CONFIG_PROFILE_1` 当前生成了 Internal Loopback。它们的 UART Pin/实例能通过 Generate/compile/link，但不是外部串口终端的板测证据。SDK 的真实外部 UART 参考 `uart_echo_interrupts_standby.syscfg`，其中 UART0 PA10/PA11 且 `enableInternalLoopback=false`。

### 10.2 Interrupt 与 DMA 什么时候用

- 偶尔发一行结果：轮询 TX ready 或简单发送 API。
- 不定时接收命令：RX interrupt。
- 连续高速发送大 buffer：UART TX DMA；必须再分配独立 DMA Channel。
- DMA/IRQ 并不会自动把 float 格式化成字符串；格式化仍是应用层工作。

<a id="opa-gpamp"></a>
## 11. OPA / GPAMP 教程

P01–P06 没有 OPA/GPAMP 集成 Profile，因此本节只使用 SDK 2.11.00.07 的 MSPM0G3507 SysConfig 元数据和官方示例说明；不能把它写成仓库 `BUILD_VERIFIED` Profile。

### 11.1 它们是什么

- OPA：片上可编程运放，支持输入路由、内部电阻梯形网络和多种增益。
- GPAMP：通用低噪声/斩波放大器，可做外部反馈或缓冲，强调轨到轨/低频精度路径。
- Buffer：输出跟随输入，主要提高驱动能力、隔离信号源。
- Non-Inverting Gain：同相放大，输出极性不反。
- Bias：给交流信号抬到 ADC 能接受的共模范围。

### 11.2 OPA 非反相增益

可参考 SDK `opa_non_inverting_pga.syscfg` 的真实 G3507 配置：

1. 添加 `OPA`，选择 `OPA1`。
2. `Non-Inverting Channel (PSEL)=IN1_POS`，外部输入 PA18。
3. `Inverting Channel (NSEL)=RTAP`。
4. `Input MUX (MSEL)=GND`。
5. Gain 选择 `N15_P16`，SysConfig 显示同相 16x/反相 -15x。
6. `OPA Output Pin=Enabled`，输出 PA16。
7. 检查输入、输出没有超出 OPA 共模和输出摆幅；这些电气限制要看数据手册并实测。

### 11.3 OPA 内部链到 ADC

SDK `opa_signal_chain_to_adc.syscfg` 展示了内部/外部混合路由：PSEL 可以选 DAC8_OUT，NSEL 可以选外部 PA27，另一级 OPA 输出可进入 ADC internal Channel 13。关键判断方法：

1. 在 OPA Quick Profiles/连接图选 Buffer 或 PGA 拓扑。
2. PSEL/NSEL/MSEL 逐项判断是外部 Pin 还是内部节点。
3. 如果 ADC `Input Channel` 选择内部 connection，就不要再虚构一根 OPA_OUT→ADC_IN 的外部线。
4. 如果启用 Output Pin，则它仍占一个封装 Pin，可用于示波器观察。

### 11.4 GPAMP Buffer 到 ADC

SDK `gpamp_buffer_to_adc.syscfg` 的真实做法：

1. 添加 GPAMP，`PSEL=IN_POS`。
2. `NSEL=OUTPUT_PIN`，形成电压跟随反馈。
3. `GPAMP Output Pin=Enabled`，当前例子输出 PA22。
4. ADC0 Channel 7 也在 PA22 采样。
5. SDK 对同一 PA22 使用有意的 conflict allowance，因为这是一条模拟节点链；不要把这个技巧复制到无关数字外设。

GPAMP 的 Chopping 可降低 offset/漂移/1/f 噪声，但 Standard chopping 可能需要外部滤波；ADC Assisted 还要求 ADC averaging 配合。初学者不要在不知道增益、带宽和滤波的情况下随便打开。

### 11.5 Bias 和输出范围

SysConfig 只配置路由/增益，不替你保证模拟电气正确。上板前至少计算：

```text
Vout = gain × (Vin around bias) + bias
Vout_min/max 必须位于 ADC、OPA/GPAMP 允许范围
```

并检查源阻抗、稳定性、带宽、负载和输入保护。

<a id="conflicts"></a>
## 12. 资源冲突怎么查、按什么顺序解决

| 冲突 | 表现 | 先检查 |
|---|---|---|
| Pin | PinMux 红色冲突/两个 owner | 外部接线、模拟专用 Pin、UART/SPI/GPIO |
| DMA Channel | Channel 已被占用 | ADC-A、ADC-B、DAC、UART DMA owner |
| Timer | TIMGx 已给采样/PWM/Capture | 是否可换另一个 Timer，位宽/事件能力是否满足 |
| ADC instance | ADC0/ADC1 重复配置不兼容 | 能否合并 MEM 配置，双通道是否应参考 P02 |
| Event Channel | Publisher/Subscriber 错配或复用 | Channel ID 和事件条件 |
| IRQ | ISR 名重复、同实例多个 owner | generated IRQ 宏、正式模块 ISR ownership |

推荐解决顺序：

1. 先删除根本不需要的外设链。
2. 固定物理上难换的模拟 Pin、DAC/OPA/GPAMP 内部路由。
3. 再安排 ADC instance 和 Timer/Capture 能力。
4. 再分配 DMA Channel；P06 已用完三条 Full Channel。
5. 再统一 Event Channel。
6. 最后移动较灵活的 UART/GPIO Pin。
7. 重新 Generate → compile → final link。

不能随便换的内容：正式模块依赖的实例名、DMA full/basic channel 能力、模拟输入/输出实际可用 Pin、Timer 类型/位宽、应用 ISR ownership。换之前先读模块 README 和生成宏。

<a id="task-table"></a>
## 13. “我要做什么 → SysConfig 改什么”

| 我要做什么 | SysConfig 操作 | 最接近 Profile | 还要同步的软件参数 |
|---|---|---|---|
| 换 ADC 输入脚 | ADCMEM Input Channel + PinMux | P01 | ADC_ToVoltage 的 VREF/前端比例 |
| 改采样率 | Timer Clock/Divider/Prescaler/Period；仅运行时改 Load 时可不动 `.syscfg` | P01 | `sample_rate_hz`、`timer_clock_hz`、下游 Fs |
| N 512→1024 | 通常不改 SysConfig | P01 | raw/float/workspace 容量和所有 count |
| 单 ADC 变双 ADC | 添加 ADC1 + 独立 DMA + 第二 Event subscriber | P02 | 两块 buffer、双通道 adapter |
| 输出固定 DAC 电压 | DAC12 + reference + output Pin | SDK fixed-voltage | DAC code/VREF |
| 输出连续 DAC 波形 | DAC FIFO + Timer + Event + DMA | P03 | update rate、buffer count、DDS update rate |
| ADC 同时配 DAC | 合并两条独立资源链 | P04 | ADC Fs、DAC update rate分别管理 |
| Comparator 测频 | COMP input/reference/hysteresis + Event + CAPTURE | P05 | timer tick Hz、边沿极性、threshold |
| 串口显示 | UART baud + TX/RX Pin，关闭 internal loopback | P01 的资源；外部功能看 SDK UART echo | 发送格式、终端 baud |
| OPA 缓冲/放大到 ADC | OPA PSEL/NSEL/MSEL/Gain/route + ADC input | SDK OPA examples | gain、bias、ADC 电压换算 |
| GPAMP 缓冲到 ADC | GPAMP input/feedback/output route + ADC | SDK GPAMP example | gain、bias、chopping/带宽 |

<a id="verify"></a>
## 14. 每次修改后的检查清单

1. `.syscfg` 的 Device/Package/Board 仍正确。
2. Problems 中没有 Pin、DMA、Timer、Event、IRQ 冲突。
3. Calculated Clock、Actual Timer Period、Calculated Baud Rate 合理。
4. 生成 `ti_msp_dl_config.c/h` 成功；只读检查实例宏名没有变化。
5. 正式模块要求的 `SIGNAL_ADC`、`SIGNAL_ADC_DMA`、`SIGNAL_SAMPLE_TIMER` 等名字仍存在。
6. 全部 translation units compile，并完成 final link；不能只做单 `.c` 检查。
7. 用 `.map` 检查 Flash/SRAM/Stack。
8. 上板后用示波器/逻辑分析仪/串口分别验证真实 Fs、Pin、电压、边沿和波形。
9. 只有实板证据才能写 `BOARD_VERIFIED`；SysConfig/compile/link PASS 仍只是 Build 基线。

## 15. 真实依据索引

- 仓库 Profile：`09_examples/integration_profiles/PROFILE_01...PROFILE_06/profile.syscfg`
- ADC DMA 正式模块：`02_acquisition/adc_dma/`
- DAC DMA 平台：`08_applications/common/dac_dma_platform_adapter/`
- SDK 软件 ADC：`examples/nortos/LP_MSPM0G3507/driverlib/adc12_single_conversion/`
- SDK 固定 DAC：`examples/nortos/LP_MSPM0G3507/driverlib/dac12_fixed_voltage_vref_internal/`
- SDK UART 外部收发：`examples/nortos/LP_MSPM0G3507/driverlib/uart_echo_interrupts_standby/`
- SDK OPA：`opa_non_inverting_pga/`、`opa_signal_chain_to_adc/`
- SDK GPAMP：`gpamp_buffer_to_adc/`、`gpamp_general_purpose_rri/`
- SysConfig 字段定义：SDK `source/ti/driverlib/.meta/`

如果升级 SDK、换器件或换封装，重新以新 SDK 的 `.meta`、Device SysConfig PinMux 和新板原理图为准，本文件中的 PAxx 只代表当前 MSPM0G3507/LQFP-64/LP 配置。
