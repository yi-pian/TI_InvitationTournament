# Dual ADC：双通道同步采集

## 0. 一句话说明

用同一个 Timer 同时触发两个 ADC，再由两个 DMA 分别把数据写进 A、B 数组，使相同下标的两点尽可能代表同一个采样时刻。

正式的 MSPM0G3507 硬件入口是 `signal_dual_adc_mspm0g3507.c/.h`。同目录的 `signal_adc_dual_sync.c/.h` 只是把已有的交织数组拆成两路的通用小工具，不能采集 ADC，也不应替代本模块。

## 1. 什么时候用 / 什么时候不要用

**适合：**

- 测两个正弦信号的相位差，例如放大器输入 `Vin` 与输出 `Vout`。
- 同时比较两路幅值、波形或峰峰值。
- 一路模拟信号加一路猝发标志，要求样本下标能够对应相同时间。
- 后面要把 A、B 分别送进 ADC To Voltage、Phase、Correlation 或 FFT。

**不适合：**

- 只测一路直流电压或普通单通道波形，使用 `adc_dma` 更简单。
- 两路数据不需要时间对应，先后软件读取即可。
- 想把一个 ADC 的单次转换结果复制两份，这不是双通道采集。

## 2. 先看懂原理

```text
                    ADC A -> DMA A -> raw_a[0..N-1]
Timer ZERO Event ---+
                    ADC B -> DMA B -> raw_b[0..N-1]

第 i 次 Timer 事件：
    ADC A 转换 -> raw_a[i]
    ADC B 转换 -> raw_b[i]
```

Timer 是“拍子”。每到一个拍子，两个 ADC 同时开始各自的一次转换；DMA 不经过 CPU，直接把 ADC 原始码搬进 RAM。当两个 DMA 都搬完 `N` 个点，模块停止 Timer，`SignalDualADC_IsFinished()` 才会返回 `true`。

这不代表两颗 ADC 的模拟保持瞬间绝对没有硬件误差；它表示两路使用同一触发事件。若要做到高精度相位测量，还需校准前端和通道延迟。

## 2.5. 30 秒接入

1. 在 CCS 工程中配置两个 ADC、两个 DMA 和一个公共 Timer，具体步骤见下一节；先使用 `PROFILE_02_DUAL_ADC` 作为已验证参考。
2. 复制 `signal_dual_adc_mspm0g3507.c`、`signal_dual_adc_mspm0g3507.h` 和 `01_bsp/common/signal_status.h` 到工程，并将它们加入 Build。
3. 只选择一个示例作为工程的 `main.c`：第一次用选 `README_MINIMAL_EXAMPLE.c`；要了解所有 API 选 `README_FULL_EXAMPLE.c`。不要把两个 `main()` 同时加入 Build。
4. 先改 `SIGNAL_SAMPLE_RATE_HZ`、`SIGNAL_SAMPLE_COUNT` 以及 SysConfig 的两个输入 Pin/Channel，然后 Build。

本模块需要 SysConfig。不要直接改 `ti_msp_dl_config.c/.h`，这些文件由 CCS 生成，下一次保存 `.syscfg` 会覆盖手工修改。

DMA 完成中断由模块初始化函数统一接管：`SignalDualADC_Init()` 会打开 DMA IRQ 以及两路已配置 DMA 通道的完成中断。应用层不需要再手写 `DL_DMA_enableInterrupt()`；只要确保两个 DMA 使用不同的有效通道，并按下文打开 ADC 的 DMA done 触发配置即可。

## 3. SysConfig / Pin

以下按 CCS GUI 一步一步配置。

参考文件：[PROFILE_02_DUAL_ADC/profile.syscfg](../../09_examples/integration_profiles/PROFILE_02_DUAL_ADC/profile.syscfg)。其中 ADC0/PA25、ADC1/PA17、DMA_CH0/CH1、TIMG0 仅是参考资源，比赛接线改变时必须按芯片 PinMux 的合法映射选择，不能死记这些 Pin。

### 第 1 步：添加两个 ADC12

在 `.syscfg` 左侧点击 `+`，添加两个 `ADC12`，实例名分别填：

```text
SIGNAL_ADC_A
SIGNAL_ADC_B
```

对每个 ADC，在 `ADC Conversion Memory Configurations -> Memory 0 -> Input Channel` 选择本路实际接线对应的通道，并在 `PinMux Peripheral and Pin Configuration` 选择合法模拟 Pin。

把两路的 `Trigger Source` 设为 Event，打开 Repeat Mode。含义是：每次 Timer Event 来时再转换一次，而不是软件调用一次只转换一次。示例选择 `Memory 0 result loaded` 作为 DMA 触发源，并打开 `DMA done` interrupt；本模块正是靠这个完成中断判断两路是否都采完。

### 第 2 步：给每个 ADC 配一个 DMA

在每个 ADC 的 `DMA Configuration` 中添加 DMA，名称分别为：

```text
SIGNAL_ADC_A_DMA
SIGNAL_ADC_B_DMA
```

两路 DMA 必须选不同的空闲 channel。设置：

```text
Source length / Destination length = Half Word
Address mode = Fixed address to Block address
Transfer mode = Single
```

`Half Word` 是一个 `uint16_t` ADC 原始码；“Fixed to Block”表示 ADC 结果寄存器地址不动、RAM 数组地址逐点增加；`Single` 表示每次 ADC 完成只搬一个样本。普通使用时不需要改这些三项。

### 第 3 步：添加公共 Timer

添加 `TIMER`，名称填：

```text
SIGNAL_DUAL_ADC_TIMER
```

设置周期模式，选择实际的 Timer 时钟源及 divider/prescaler。Timer 周期会被 `SignalDualADC_SetSampleRate()` 在运行时更新，因此 SysConfig 的初始 `10 us` 只用于生成基线；100 kSPS 对应 10 us。

记录 GUI 显示的 **Timer Clock Frequency**。它是 `timer_clock_hz`，不是每秒采样率。若使用参考 profile 的 BUSCLK / divider 1 / prescaler 1，且生成工程的 `CPUCLK_FREQ` 就是该 BUSCLK，可填 `CPUCLK_FREQ`；只要改过时钟树或分频，就应填 GUI 显示的实际频率。

### 第 4 步：把同一 Timer 事件接到两路 ADC

在 Timer 的 `Event Configuration` 开两个 `ZERO_EVENT` publisher，例如 Channel 1、2；在 A、B 两个 ADC 的 `Event Configuration -> Subscriber Channel ID` 中分别订阅相应 Channel。两个事件都来自同一个 Timer ZERO，所以两路触发同步；它们不必使用同一个 Event Channel 编号。

```text
TIMG0 ZERO_EVENT -> publisher 1 -> SIGNAL_ADC_A subscriber 1
TIMG0 ZERO_EVENT -> publisher 2 -> SIGNAL_ADC_B subscriber 2
```

保存 `.syscfg`，检查生成头文件是否有 `SIGNAL_ADC_A_*`、`SIGNAL_ADC_B_*`、两路 `*_DMA_CHAN_ID` 和 `SIGNAL_DUAL_ADC_TIMER_*` 宏。若实例名不同，正式源码无法找到这些宏。

## 4. 输入、输出和内存

- **模拟输入：** 两路均必须在 ADC 允许的电压范围内，不能把超出 VDDA/VSSA 的信号直接接入。
- **输入参数：** 每通道采样率 `Fs` 和每帧点数 `N`。
- **输出：** `uint16_t raw_a[N]` 与 `uint16_t raw_b[N]`。`raw_a[i]` 和 `raw_b[i]` 对应第 `i` 次 Timer 触发。
- **内存：** 两路原始码占 `2 * N * sizeof(uint16_t) = 4N` 字节。`N=1024` 时为 4096 B，不含栈和后续浮点数组。

采集未完成时 DMA 正在写数组，不能读取、改写、传给 FFT 或用于显示。完成后到下一次 `Start()` 前才是安全处理期。

## 5. 参数教程

### 【比赛必须会】`sample_rate_hz` / `SIGNAL_SAMPLE_RATE_HZ`

**是什么：** 每一路 ADC 每秒采样次数 `Fs`，单位 Hz。`100000U` 是 100 kSPS，不是两路合起来 100 kSPS。

**影响：** 能观察的最高频率、每周期点数、采样窗口长度、DMA 数据速率。基本关系：

```text
每周期点数 = Fs / f_signal
每帧观察时间 Tframe = N / Fs
```

最高输入频率 10 kHz 时：

```text
Fs = 20 kHz   -> 每周期约 2 点，只达到理论下限，不适合看波形
Fs = 100 kHz  -> 每周期约 10 点，常用起点
Fs = 500 kHz  -> 每周期约 50 点，细节更好但数据更密
```

观察波形或做相位，先取最高频率的 10~50 倍，再检查 ADC 转换时间、前端带宽和 DMA 资源。Fs 太低会混叠、相位不稳；Fs 太高会缩短同一 N 的观察时间并增大处理压力。题目最高频率变化时，这是最常改参数。

### 【比赛必须会】`sample_count` / `SIGNAL_SAMPLE_COUNT`

**是什么：** 一轮每路采多少点 `N`，类型为 `uint16_t`，合法范围 1~65535。

**怎么选：** 初次使用可从 `N=1024` 开始。若要测最低频率，保证 `Tframe=N/Fs` 能覆盖至少数个周期；例如测 10 Hz，相位算法希望约 3 个周期，`Fs=1 kHz` 时可选 `N >= 300`，选 1024 会得到 1.024 s 窗口。N 增大：低频观察更好、RAM 更多、处理刷新更慢。后面直接接 FFT 时，还需选 FFT 实现支持的 N（常见为 2 的幂）。

### 【比赛必须会】A/B 输入 Pin 和 ADC Channel

在 SysConfig 修改，而不是改模块 `.c`。换线时最常见错误是 Pin 看似可用却不属于选中的 ADC instance/channel；以 PinMux 提示为准。

### 【出问题再理解】`timer_clock_hz`

这是采样 Timer **分频后的实际输入时钟**，不是 ADC 时钟，也不总是 `CPUCLK_FREQ`。模块用近似公式计算：

```text
timer_count = round(timer_clock_hz / sample_rate_hz)
实际 Fs = timer_clock_hz / timer_count
```

例如 Timer 输入为 32 MHz、目标 Fs 为 100 kHz，计数为 320，实际 Fs 为 100 kHz。目标不是整除时，`GetConfiguredRate()` 返回取整后的实际值，后续频率或相位算法应使用它。Timer 时钟树、divider 或 prescaler 改了才需要改此参数。

### 【出问题再理解】`timer_max_count = 65536U`

此值告诉模块 Timer 一次周期最多能表示多少个计数。参考工程使用 16 位 Timer，0 到 65535 的 load 值对应最大 65536 个计数，因此填 `65536U`。普通用户通常不要修改；只有确认改用了不同计数宽度的 Timer 时才按硬件上限改。Fs 太低导致所需计数超过它时，`Init` 或 `SetSampleRate` 返回 `SIGNAL_RESULT_OUT_OF_RANGE`。

### 【以后进阶】DMA Channel、Event Channel、连续缓冲

DMA Channel 和 Event Channel 只要不冲突即可，通常不因为题目频率变化而改。连续显示/处理应设计双缓冲或三缓冲；本模块每次 `Start()` 只采一帧，不提供自动 buffer swap。

## 6. 调用顺序

```text
上电
  -> SYSCFG_DL_init()                 只一次
  -> SignalDualADC_Init(&config)      只一次
  -> SetSampleRate(Fs)                仅 Fs 改变且未运行时
循环：
  -> SignalDualADC_Start(A, B, N)     每一帧
  -> IsFinished()                     等待两路 DMA 完成
  -> GetChannelA/B, GetSampleCount    读取结果
  -> 自己处理数据
  -> 下一次 Start()
```

## 7. API 教程

### `SignalDualADC_Init(const signal_dual_adc_config_t *config)`

**做什么：** 保存 Timer 参数，打开两个 ADC 完成中断，并设置初始采样率。**何时调用：** `SYSCFG_DL_init()` 后，上电时一次。**参数：** `config` 指向含 `sample_rate_hz`、`timer_clock_hz`、`timer_max_count` 的配置，不能为 `NULL`。**返回：** `SIGNAL_RESULT_OK` 成功；`SIGNAL_RESULT_INVALID_ARGUMENT` 配置/Timer 参数为 0；`SIGNAL_RESULT_OUT_OF_RANGE` 当前 Fs 无法由 Timer 表示。初始化会停止正在进行的采集，因此不应在正常采集中调用。

```c
g_result = SignalDualADC_Init(&config);
```

### `SignalDualADC_SetSampleRate(uint32_t sample_rate_hz)`

**做什么：** 改写 Timer 周期，给下一帧设置 Fs。**何时调用：** Init 后、Start 前，或者上一帧已经完成后。**参数：** `sample_rate_hz` 是目标每路 Fs，必须大于 0 且不高于 Timer 输入时钟。**返回：** `SIGNAL_RESULT_OK`、`SIGNAL_RESULT_NOT_INITIALIZED`、`SIGNAL_RESULT_BUSY`、`SIGNAL_RESULT_INVALID_ARGUMENT` 或 `SIGNAL_RESULT_OUT_OF_RANGE`。成功后用 `GetConfiguredRate()` 读取实际 Fs。

### `SignalDualADC_Start(uint16_t *channel_a, uint16_t *channel_b, uint16_t sample_count)`

**做什么：** 设置两路 DMA 目的地址和长度，然后启动 ADC 与 Timer。**何时调用：** 每一帧采集的开始。**参数：** `channel_a`/`channel_b` 是各自至少有 `sample_count` 个 `uint16_t` 的数组；`sample_count` 是每路点数。**返回：** `SIGNAL_RESULT_OK`、`SIGNAL_RESULT_NOT_INITIALIZED`、`SIGNAL_RESULT_BUSY`、`SIGNAL_RESULT_INVALID_ARGUMENT`。返回 OK 后两个 Buffer 在完成前不可使用。

### `SignalDualADC_IsFinished(void)` 与 `SignalDualADC_GetStatus(void)`

**做什么：** 前者只回答本帧是否两路都完成；后者返回更完整的 `MODULE_IDLE`、`MODULE_RUNNING`、`MODULE_DONE` 或 `MODULE_ERROR`。**何时调用：** Start 后等待和调试时。`IsFinished()` 为 `true` 后才可处理数组。没有返回码。

```c
while (!SignalDualADC_IsFinished()) { __WFI(); }
```

### `SignalDualADC_GetChannelA/B(void)`、`SignalDualADC_GetSampleCount(void)`、`SignalDualADC_GetConfiguredRate(void)`

**做什么：** 查询最近一次 Start 的 A/B Buffer 地址与点数，查询 Timer 取整后的实际 Fs。**何时调用：** 完成后取结果，或在 Init/SetSampleRate 后检查实际 Fs。A/B 在尚未 Start 时返回 `NULL`；点数在尚未 Start 时为 0；它们都没有返回码。getter 不复制数据，返回的是你原来传给 Start 的地址。

### `SignalDualADC_Stop(void)`

**做什么：** 立即停 Timer、ADC 和 DMA，状态回到 `MODULE_IDLE`。**何时调用：** 超时、用户取消或切换模式。**返回：** 无。主动停止的当前数组可能只写了一部分，不能当测量结果。正常一帧采集无需调用它，模块完成时会自行停止。

### `SignalDualADC_GetModuleMaturity(void)`

**做什么：** 返回验证等级，不是运行状态。当前返回 `MODULE_STATUS_BUILD_VERIFIED`，表示隔离工程已通过 SysConfig、编译和链接；不表示本轮已经实板校准。**何时调用：** 做诊断或在产品信息中展示时。无参数、无返回码以外的错误状态。

`README_FULL_EXAMPLE.c` 在正确顺序中覆盖以上全部公开 API；`README_MINIMAL_EXAMPLE.c` 只保留最短正常流程。

## 8. 第一次使用建议值与速查

完全不知道如何起步时，先用：

```text
Fs = 100 kHz
N  = 1024
Timer clock = SysConfig 显示的实际频率（参考 profile 是 CPUCLK_FREQ）
timer_max_count = 65536U
```

这会得到约 10.24 ms 的窗口，适合约 10 kHz 附近的波形初查；它不适合直接测 10 Hz。题目说“最高频率升高”时先检查 Fs；说“要测更低频”时先用 `N/Fs` 检查窗口；说“换接线”时只改 SysConfig Pin/Channel；说“RAM 不够”时先减 N。

```text
常改：  Fs、N、两个输入 Pin/Channel
偶尔改：timer_clock_hz（时钟树变动时）
通常不改：65536U、DMA Channel、Event Channel
```

## 9. 完整使用案例：测放大器输入/输出相位差

```text
Vin -> ADC A -> raw_a[] -> ADC To Voltage -> voltage_a[]
Vout -> ADC B -> raw_b[] -> ADC To Voltage -> voltage_b[]
voltage_a[] + voltage_b[] -> Phase / Correlation -> 相位差
```

先让两路接同一稳定信号：完成后两组 raw 的波形趋势应一致。再在一路加入已知相移，检查相位算法的正负号和角度。实际测量前确认前端偏置、幅度范围和两个通道的延迟校准。

## 10. 不要这样做 / FAQ

- 不要把 ADC Clock 当成 Fs；Fs 由 Timer 事件间隔决定。
- 不要在 `IsFinished()` 为 false 时读、显示、FFT 或重写 Buffer。
- 不要在上一帧 `MODULE_RUNNING` 时再次 Start 或 SetSampleRate。
- 不要手改生成的 `ti_msp_dl_config.c/.h`，也不要照搬 PA25/PA17。
- 不要在 ADC DMA 完成中断中执行 FFT、TFT 刷屏或浮点大运算；本模块中断只做完成标记和停止硬件。
- 若只一路完成，检查该 ADC 的 Event subscriber、DMA trigger/channel、DMA done interrupt 和实例名。
- 若两路不对齐，检查二者是否都由同一个 Timer ZERO_EVENT 触发，而不是软件先后启动。
- 若 `Init` 返回 OUT_OF_RANGE，检查 `timer_clock_hz`、Fs 和 65536U 的关系；低 Fs 可能超出 16 位 Timer 周期。
- 若宏找不到，检查 SysConfig 实例名是否严格为 `SIGNAL_ADC_A`、`SIGNAL_ADC_B`、`SIGNAL_DUAL_ADC_TIMER` 及两路 DMA 名称。

## 11. 验证状态

参考 profile 已在隔离复制工程完成 SysConfig、编译与完整链接验证。当前等级为 `MODULE_STATUS_BUILD_VERIFIED`；本模块没有在本 README 中宣称双路同步精度已经完成实板校准。

## 17. 统一 API 教程（已按当前头文件核对）

本节由当前公开头文件、实现中实际出现的状态码和正式模块注册表生成。它补充前文的场景教程；函数签名变化时必须重新运行 `tools/upgrade_formal_beginner_docs.ps1`，不要手工保留旧 API。

遵循仓库的 [Beginner README 标准](../../00_docs/BEGINNER_README_STANDARD.md)：先用最小示例完成一次正常数据流，再按需要阅读全功能示例和本节 API 细节。

### 什么时候用 / 什么时候不要用

当题目需要“adc_dual_sync”目录对应的公开功能，并且输入数据、单位和硬件资源满足前文约束时使用本模块。若只需要更简单的上游功能、输入尚未准备好，或需要不同的数据模型/外设资源，应先选择相邻模块而不是强行调用本 API。

### 输入 / 输出

输入由各 API 的只读数组、配置、频率/阈值和平台对象组成；输出写入 result/output/buffer 参数或由 Get API 返回。调用者负责数组容量、生命周期和物理单位；失败返回时输出不是有效结果。

### 调用顺序

```text
SignalADCDualSync_GetModuleStatus -> SignalADCDualSync_Deinterleave
```

`Init` 一般仅一次；`Set/Configure` 仅在参数变化且模块空闲时执行；`Start/Process/Generate` 是每帧或每次任务入口；`Get/Is` 用于读取已完成的结果；`Stop` 只在需要取消时调用。若本模块没有其中某类 API，以实际列出的函数为准。

### SysConfig 边界

本模块需要 SysConfig。先在 CCS 的 .syscfg 添加并核对：ADC12、EVENT、TIMER；再按前文的模块专用 GUI 步骤选择实际 pin/instance。保存后让 CCS 重新生成配置，核对生成宏；不要直接修改 	i_msp_dl_config.c/.h，也不要照抄示例 pin 或 DMA/Event 编号。

### 参数分级

- 【比赛必须会】输入/输出数组、`count/length/capacity`、采样率/频率、阈值/增益以及本 README 前文标出的 pin。它们直接影响题目范围、RAM、时间轴或物理单位。
- 【出问题再理解】Timer 时钟、DMA 通道、Event 路由、参考源和 IRQ。它们属于硬件链路，必须与 SysConfig 生成结果一致。
- 【以后进阶】多缓冲、运行时重配置、回调调度和 ISR 优化。先用最小示例完成一帧闭环，再处理吞吐或延迟。

### 常见错误 FAQ

- 参数错误：先检查指针非空、count/capacity 的单位是元素数、频率/阈值单位与上游一致。
- 硬件无结果：不要修改生成文件；回到 SysConfig 核对 pin、instance、时钟、Timer、DMA 和 Event 的完整链路。
- 结果异常：确认上一轮异步采集已经完成，真实 Fs/N/参考电压已传到算法，且没有在 DMA 使用期间改写 buffer。

### `signal_result_t SignalADCDualSync_Deinterleave(const uint16_t *interleaved, size_t pair_count, uint16_t *channel_a, size_t channel_a_capacity, uint16_t *channel_b, size_t channel_b_capacity);`

**它做什么：** 执行该模块公开的功能；具体数据流以本节参数表和本 README 前面的场景说明为准。

**什么时候调用：** 执行该模块公开的功能；具体数据流以本节参数表和本 README 前面的场景说明为准。

| 参数 | 类型 | 初学者解释 |
|---|---|---|
| `interleaved` | `const uint16_t *` | `interleaved`（`const uint16_t `）是该 API 的输入/输出参数；按本 README 前面的数据单位和边界条件准备。 |
| `pair_count` | `size_t` | 元素数量或容量，单位是“元素个数”而不是字节；必须与实际数组大小一致。 |
| `channel_a` | `uint16_t *` | 索引或通道号；范围由相应数组长度、FFT bin 数或当前硬件配置决定。 |
| `channel_a_capacity` | `size_t` | 元素数量或容量，单位是“元素个数”而不是字节；必须与实际数组大小一致。 |
| `channel_b` | `uint16_t *` | 索引或通道号；范围由相应数组长度、FFT bin 数或当前硬件配置决定。 |
| `channel_b_capacity` | `size_t` | 元素数量或容量，单位是“元素个数”而不是字节；必须与实际数组大小一致。 |

**返回：** 返回 signal_result_t 类型结果；调用者应检查该值。

**最小调用形状：** `SignalADCDualSync_Deinterleave(...);`。可直接从 README_MINIMAL_EXAMPLE.c 复制正常流程；README_FULL_EXAMPLE.c 展示全部公开 API，其中取消类 API 会以 #if 0 隔离。

**注意：** 所有指针和数组都由调用者拥有；先检查返回值。异步采集、DMA 或回调还在使用 buffer 时，不能读取结果或改写该 buffer。

### `signal_module_status_t SignalADCDualSync_GetModuleStatus();`

**它做什么：** 读取模块当前的验证成熟度或静态状态，不会启动硬件操作。

**什么时候调用：** 读取模块当前的验证成熟度或静态状态，不会启动硬件操作。

**参数：** 无。

**返回：** 返回 signal_module_status_t 类型结果；调用者应检查该值。

**最小调用形状：** `SignalADCDualSync_GetModuleStatus(...);`。可直接从 README_MINIMAL_EXAMPLE.c 复制正常流程；README_FULL_EXAMPLE.c 展示全部公开 API，其中取消类 API 会以 #if 0 隔离。

**注意：** 所有指针和数组都由调用者拥有；先检查返回值。异步采集、DMA 或回调还在使用 buffer 时，不能读取结果或改写该 buffer。

### 示例、模块链与验收

- 最小入门：`README_MINIMAL_EXAMPLE.c`，只保留正常入口和结果读取。
- 全功能：`README_FULL_EXAMPLE.c`，以正确顺序展示当前头文件全部公开 API；`Stop` 等非常规路径不会默认执行。
- 模块链：先由上游提供单位、采样率和有效数据，再调用本模块；成功后将输出交给显示、控制、测量或下一步 DSP。硬件资源仍以 SysConfig 合约为唯一来源。
- 文档验收：README/API、两份示例和头文件会由 `tools/validate_beginner_documentation.ps1` 覆盖检查；这只表示文档与源码签名一致，不代替未进行的实板验证。

### 模块链

`上游采集/配置 -> 本模块 -> 检查返回值和结果 -> 测量、显示、控制或下一步 DSP`。只有确认本模块的输出单位和有效状态后，才交给下一模块。

## 18. 连续双 DMA 采集（24_C 已收录接口）

当题目既要常规有限帧分析、又要连续等待短暂猝发时，使用同一模块的连续接口；它不需要新增 SysConfig 外设或 DMA 通道。仍使用本 README 前面已配置的同一 TIMG0 Event、ADC0/ADC1、DMA_CH0/DMA_CH1。

```c
uint32_t sequence;
uint8_t completed_block;

SignalDualADC_StartContinuous(&adc_a[0][0], &adc_b[0][0],
    SAMPLES_PER_BLOCK, BLOCK_COUNT);

if (SignalDualADC_GetContinuousSnapshot(&sequence, &completed_block)) {
    /* 只读取 completed_block；DMA 此时已经转去写下一块。 */
    ProcessBlock(adc_a[completed_block], adc_b[completed_block]);
}
```

`StartContinuous` 的两个数组必须分别包含 `block_count * samples_per_block`
个 `uint16_t` 元素，且两个数组的块数和每块长度一致。DMA 中断只有在
ADC0 和 ADC1 对同一块都完成时，才递增 `sequence` 并发布 `completed_block`；
这保证两个通道的同一采样下标来自同一个 Timer Event。

`GetContinuousSnapshot` 是无阻塞读取：返回 `false` 表示尚无新块或同一块已被
读取过；返回 `true` 时输出刚发布的序号和块号。主循环应保存上次处理的
`sequence`，禁止读 DMA 正在写入的块，也禁止在 DMA 中断中做 FFT、浮点计算或 TFT
绘制。需要退出连续模式时调用 `SignalDualADC_Stop()`。

24_C 用三块缓冲：一块由 DMA 写入，一块刚完成可供分析，一块留作 ISR 轮转余量。
这只是软件缓冲策略，SysConfig 仍完全按第 3 节的两路 ADC 和两路 DMA 配置。

## 19. 比赛通用功能代码

本 README 已提供 `SignalDualADC_SetSampleRate()`、`SignalDualADC_GetConfiguredRate()`、
连续三缓冲和“只能在帧边界改 Fs”的规则；没有把“根据被测频率自动选 Fs、限幅、无效
频率保持旧值”的应用层组合逻辑塞进冻结模块。请复制统一手册
[CONTEST_FUNCTIONAL_CODE_COOKBOOK.md](../../00_docs/CONTEST_FUNCTIONAL_CODE_COOKBOOK.md)
的第 2 节。手册中明确标出：哪些行来自本模块 README，哪些行是 `main.c` 自写，以及
每行的变量和边界作用。

## 20. 可直接复制：动态采样率最小闭环

下面这一段是第 2 节的现场版，适合题目先测出输入频率、再为下一帧选择采样率的情况。
它只放在 `main.c`，不会修改本模块的 `.c/.h`。模块 README 已提供的 API 是
`SignalDualADC_SetSampleRate()` 和 `SignalDualADC_GetConfiguredRate()`；宏、变量和
`App_UpdateSampleRate()` 是应用层自写。

```c
#define APP_POINTS_PER_CYCLE       (20U)       /* 每周期保留的最低点数。 */
#define APP_MIN_SAMPLE_RATE_HZ     (20000U)    /* 采样率下限。 */
#define APP_MAX_SAMPLE_RATE_HZ     (200000U)   /* 采样率上限。 */

static uint32_t g_sample_rate_hz = 100000U;    /* 后续算法实际使用的 Fs。 */

static void App_UpdateSampleRate(uint32_t measured_frequency_hz)
{
    uint32_t target_sample_rate_hz;             /* 目标 Fs。 */
    uint32_t actual_sample_rate_hz;             /* Timer 整数分频后的 Fs。 */

    if (measured_frequency_hz == 0U) return;     /* 无效频率保持旧值。 */
    if (measured_frequency_hz >
        APP_MAX_SAMPLE_RATE_HZ / APP_POINTS_PER_CYCLE) return;
                                                   /* 防止乘法超过上限。 */

    target_sample_rate_hz = measured_frequency_hz * APP_POINTS_PER_CYCLE;
                                                   /* 频率乘每周期点数。 */
    if (target_sample_rate_hz < APP_MIN_SAMPLE_RATE_HZ) {
        target_sample_rate_hz = APP_MIN_SAMPLE_RATE_HZ;
    }
                                                   /* 不让 Fs 过低。 */
    if (target_sample_rate_hz > APP_MAX_SAMPLE_RATE_HZ) {
        target_sample_rate_hz = APP_MAX_SAMPLE_RATE_HZ;
    }
                                                   /* 不让 Fs 超出硬件上限。 */

    if (SignalDualADC_SetSampleRate(target_sample_rate_hz) !=
        SIGNAL_RESULT_OK) return;                 /* 忙或越界时保留旧配置。 */

    actual_sample_rate_hz = SignalDualADC_GetConfiguredRate();
                                                   /* 读取真实整数化 Fs。 */
    if (actual_sample_rate_hz != 0U) {
        g_sample_rate_hz = actual_sample_rate_hz;
    }
                                                   /* 时间轴和测量统一用真实 Fs。 */
}
```

调用位置固定为“上一帧完成、下一帧 `SignalDualADC_Start()` 之前”。不要在 DMA/ADC
中断中调用，也不要在 `MODULE_RUNNING` 状态下调用。`g_sample_rate_hz` 的每一行赋值
都表示后续时间轴、FFT 或相位算法使用的实际采样率；它不是 SysConfig 中的目标值。上述
边界、返回值和调用时机与本 README 前文一致。

