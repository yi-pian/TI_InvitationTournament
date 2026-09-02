# DAC DMA：连续输出周期采样表

## CCS SysConfig GUI Configuration

### Required resources

需要 `DAC12`、`DMA`、`TIMER`、`EVENT`。`DAC12` 是 SysConfig module，`DAC0` 是硬件 DAC instance；`DMA_CH1`、`TIMG6` 是实际硬件资源；`DL_DAC12_*`/`DL_TimerG_*` 是 DriverLib C 名称。

### Step 1 - DAC12

GUI Path: 左侧 `Software` -> `Add` -> `DAC12` -> 实例 -> `Basic Operation Configuration`；依次展开 `DAC Output`、`Reference/Voltage`、`FIFO Configuration`、`Trigger Configuration`，再进入 `Advanced Configuration` 核对输出分辨率、数据格式和放大器。

Action: P03 已验证启用 DAC FIFO、DAC output、amplifier，硬件 trigger=`HWTRIG0`，FIFO threshold=`TWO_QTRS_EMPTY`，subscriber channel=`3`，输出为 `PA15`。字段名称以 DAC12 GUI 截图为准；固定 DC 不需要本模块的 FIFO/DMA/Timer 链。

### Step 2 - DMA

GUI Path: 左侧 `DAC12` -> 实例 -> `DMA Configuration` -> `Configure DMA Trigger`，选择 DAC FIFO/request；再进入左侧 `DMA` -> channel -> `Transfer Configuration` 设置 `Source Length`、`Destination Length`、`Address Mode`、`Transfer Mode`。

Action: 实例名 `SIGNAL_DAC_DMA`，P03 硬件 channel 为 `DMA_CH1`；`Transfer Mode = FULL_CH_REPEAT_SINGLE`、`Address Mode = block-to-fixed`、`Source Length = Half Word`、`Destination Length = Half Word`。Ping-pong/repeat 的波表生命周期仍是软件规则，不要虚构成新的 GUI mode。

### Step 3 - Timer clock and Event

GUI Path: 左侧 `Software` -> `Add` -> `TIMER` -> 实例 `SIGNAL_DAC_TIMER` -> `Basic Configuration` -> `Clock Configuration`；继续进入 `Event Configuration` -> `Event Publisher 0/1` -> `Publisher Channel ID`，再回到 `DAC12` -> `Event Configuration` -> `Subscriber Port/Channel`。

Set（P03 已验证）：`Timer Peripheral = TIMG6`、`Timer Clock Source = BUSCLK`、`Timer Clock Divider = 1`、`Timer Clock Prescaler = 1`、`Timer Mode = Periodic Down Counting`、`Desired Timer Period = 10 us`。在 `Event Configuration` 设置 `Event 1 Publisher Channel ID = 3`，并将其 `ZERO_EVENT` 连接到 DAC 的 `HWTRIG0` subscriber。`Fupdate = 1 / Actual Timer Period`，不是 DAC functional clock。

共享教材：[MSPM0G3507 SysConfig 时钟、Timer、ADC 与 DAC 保姆教程](../../00_docs/MSPM0_SYSCONFIG_CLOCK_TIMER_ADC_DAC_BEGINNER_GUIDE.md)。先从 Clock Source -> Divider -> Prescaler -> Actual Timer Period 得到真实 `Fupdate`，再按 `Fout = Fupdate / table_count` 或 DDS tuning word 计算输出频率。

### Expected generated symbols

Generate 后核对 `SIGNAL_DAC_DMA_CHAN_ID`、DMA transfer/trigger 宏、`SIGNAL_DAC_TIMER_INST`、`*_LOAD_VALUE`、DAC instance/output 宏和 `DAC12_INT_IRQN`。PROJECT_AUDIT 记录 `GUI field -> .syscfg property -> generated symbol`。

### Final checklist / Common mistakes / Do not change

- DAC FIFO trigger、Timer publisher、Event subscriber、DMA direction/width 完整闭合。
- 实际 Timer clock/period 已回算为 `Fupdate`，波表 `static` 生命周期覆盖 DMA 使用期。
- 不把 DAC clock 当 update rate；不直接编辑 `.syscfg` 或 `ti_msp_dl_config.c/.h`。

## 0. 什么时候用

当你已经有一组 `uint16_t samples[]`，要由 Timer 定速、DMA 自动连续送到 MSPM0G3507 DAC 时使用。只输出一个固定电压时不要复制本模块，直接 SysConfig + `DL_DAC12_output12(...)`。

> 不知道 DAC Clock、Timer event、`Fupdate` 和 `Fout` 的区别，先读 [Clock/Timer/ADC/DAC 保姆教程](../../00_docs/MSPM0_SYSCONFIG_CLOCK_TIMER_ADC_DAC_BEGINNER_GUIDE.md)；不知道该选多少更新率，读 [采样率选择指南](../../00_docs/SAMPLE_RATE_SELECTION_GUIDE.md)；现场回算看 [一页速查](../../00_docs/CLOCK_TIMER_ADC_DAC_QUICK_REFERENCE.md)。固定 DC 不需要 Timer/DMA。

比赛主入口是 `signal_dac_dma_mspm0g3507.*`，不需要 Application Platform 或 callback。

## 1. 30 秒接入路线

你需要复制：`signal_dac_dma_mspm0g3507.c`、`signal_dac_dma_mspm0g3507.h`、`01_bsp/common/signal_status.h`。本模块【需要 SysConfig】。

配置 DAC0 + DMA + TIMG6 Event，复制比赛入口与 `signal_status.h`，粘贴下方代码，改更新率和波表，然后 Build。

## 2. 输入和输出

- 输入：`const uint16_t samples[count]`，每个值为 12-bit DAC code（通常 0～4095）。
- 输出：DAC0 引脚上的模拟阶梯波；示例输出脚为 PA15。
- `repeat=true` 时数组必须一直存在且不能修改；它通常应是 `static const`。

## 3. SysConfig / Pin

【需要 SysConfig】。`PROFILE_03_DAC_GENERATOR` 只作为配置项参考；实际操作是在 CCS 中双击目标工程 `.syscfg`，使用 SysConfig 图形界面配置。不要直接编辑 `.syscfg` 文本或生成的 `ti_msp_dl_config.c/.h`：

1. 添加 DAC12/DAC0，打开 Analog Output、FIFO、DMA trigger，硬件触发选 HWTRIG0。
2. 示例 DAC 输出为 PA15；比赛接线变化时在 SysConfig 确认器件唯一合法 DAC 输出脚，不能把 DAC 随意搬到普通 GPIO。
3. DAC DMA 名称必须为 `SIGNAL_DAC_DMA`；示例 DMA_CH1、Half Word、block-to-peripheral、repeat single。
4. 添加周期 Timer，实例名 `SIGNAL_DAC_TIMER`；示例 TIMG6、BUSCLK/1，并把 ZERO event 发布到 DAC HWTRIG0 对应 Event channel。
5. 打开 DAC DMA-done interrupt；保存 `.syscfg`，等待 CCS 自动重新生成，再检查 `SIGNAL_DAC_DMA_CHAN_ID`、`SIGNAL_DAC_TIMER_INST`、`DAC12_INT_IRQN` 等宏。
6. 在资源冲突视图检查 DMA_CH1、TIMG6、Event channel 和 PA15 是否冲突，再 Clean/Build。

模块通过生成宏寻找 Timer/DMA；DAC0 和 DAC12 IRQ 是 MSPM0G3507 固定资源。不要修改生成的 `ti_msp_dl_config.*`。

## 4. 复制哪些文件

从本目录复制：

- `signal_dac_dma_mspm0g3507.c`
- `signal_dac_dma_mspm0g3507.h`

再复制 `01_bsp/common/signal_status.h`。全部放入母版 `modules/`。不需要复制 `signal_dac_dma.c/h`、Platform、Adapter 或其他 Application 源码。

## 5. main.c 顶部复制什么

```c
#include <stdint.h>
#include "ti_msp_dl_config.h"
#include "signal_dac_dma_mspm0g3507.h"

// ===== 你需要根据题目修改 =====
#define SIGNAL_DAC_UPDATE_RATE_HZ  (100000U)
static const uint16_t g_wave[] = {512U, 2048U, 3584U, 2048U};

// ===== 一般不用改 =====
volatile signal_result_t g_dac_status;
```

## 6. 比赛参数

| 题目变化 | 修改 |
|---|---|
| 输出频率变化且波表长度不变 | DDS tuning word，或令 `fout = update_rate / table_count` |
| 波形更平滑 | 增大每周期点数，并确认 update rate/CPU/DAC settling 能承受 |
| 幅值/偏置变化 | 重算 `g_wave[]` 的 DAC code，保持在 0～4095 |
| Timer 时钟/分频变化 | config 的真实 `timer_clock_hz` |
| 只播放一次 | `repeat=false` |

## 7. 初始化区复制什么

```c
const signal_dac_dma_mspm0_config_t dac_config = {
    SIGNAL_DAC_UPDATE_RATE_HZ, CPUCLK_FREQ, 65536U
};

SYSCFG_DL_init();
g_dac_status = SignalDACDMA_MSPM0_Init(&dac_config);
if (g_dac_status != SIGNAL_RESULT_OK) {
    while (1) { }
}
```

## 8. 启动代码复制什么

放在初始化成功后；循环播放只需启动一次：

```c
g_dac_status = SignalDACDMA_MSPM0_Start(
    g_wave, sizeof(g_wave) / sizeof(g_wave[0]), true);
if (g_dac_status != SIGNAL_RESULT_OK) {
    while (1) { }
}

while (1) {
    // ===== 这里写你自己的逻辑 =====
    // DMA 正在循环输出；需要换表/频率时先 Stop。
    __WFI();
}
```

完整代码见 `README_MINIMAL_EXAMPLE.c`。

## 9. 结果和下一级

DAC 输出是未经重建滤波的阶梯波。若题目要求低失真正弦，常见链路为：

```text
Wave Table -> DDS Fill -> DAC DMA -> PA15 -> 模拟低通/放大器
```

## 10. Build 与最小验证

先用 4～16 点简单表，示波器确认更新率、重复周期和 PA15 电平，再接模拟滤波器。隔离 COPY TEST：`SysConfig / Compile / Full Link PASS`，Flash 2584 B、SRAM（含栈）688 B；本轮未上板，状态 `BUILD_VERIFIED`。

## 11. 常见错误

- 没有波形：DAC output/FIFO/DMA trigger/Event route 中有一步没打开。
- 只输出第一个点：Timer event 没到 DAC trigger，或 DMA transfer mode 错。
- 波形频率差固定倍数：Timer 实际计数时钟与 `timer_clock_hz` 不一致。
- 周期播放后数组失效：把波表定义成了局部自动变量。
- 输出削顶：DAC code 超出 0～4095，或后级模拟电路量程不足。

## 12. API Reference

- `SignalDACDMA_MSPM0_Init(config)` / `SignalDACDMA_MSPM0_SetUpdateRate(hz)`
- `SignalDACDMA_MSPM0_Start(samples, count, repeat)`
- `SignalDACDMA_MSPM0_Stop()`
- `SignalDACDMA_MSPM0_IsFinished()` / `SignalDACDMA_MSPM0_GetStatus()`
- `SignalDACDMA_MSPM0_GetConfiguredRate()`
- `SignalDACDMA_MSPM0_GetModuleMaturity()`

## 17. 统一 API 教程（已按当前头文件核对）

本节由当前公开头文件、实现中实际出现的状态码和正式模块注册表生成。它补充前文的场景教程；函数签名变化时必须重新运行 `tools/upgrade_formal_beginner_docs.ps1`，不要手工保留旧 API。

遵循仓库的 [Beginner README 标准](../../00_docs/BEGINNER_README_STANDARD.md)：先用最小示例完成一次正常数据流，再按需要阅读全功能示例和本节 API 细节。

### 什么时候用 / 什么时候不要用

当题目需要“dac_dma”目录对应的公开功能，并且输入数据、单位和硬件资源满足前文约束时使用本模块。若只需要更简单的上游功能、输入尚未准备好，或需要不同的数据模型/外设资源，应先选择相邻模块而不是强行调用本 API。

### 输入 / 输出

输入由各 API 的只读数组、配置、频率/阈值和平台对象组成；输出写入 result/output/buffer 参数或由 Get API 返回。调用者负责数组容量、生命周期和物理单位；失败返回时输出不是有效结果。

### 调用顺序

```text
SignalDACDMA_Init -> SignalDACDMA_Start -> SignalDACDMA_GetModuleStatus -> SignalDACDMA_Stop
```

`Init` 一般仅一次；`Set/Configure` 仅在参数变化且模块空闲时执行；`Start/Process/Generate` 是每帧或每次任务入口；`Get/Is` 用于读取已完成的结果；`Stop` 只在需要取消时调用。若本模块没有其中某类 API，以实际列出的函数为准。

### SysConfig 边界

本模块需要 SysConfig。先在 CCS 的 .syscfg 添加并核对：DMA、EVENT、TIMER；再按前文的模块专用 GUI 步骤选择实际 pin/instance。保存后让 CCS 重新生成配置，核对生成宏；不要直接修改 	i_msp_dl_config.c/.h，也不要照抄示例 pin 或 DMA/Event 编号。

### 参数分级

- 【比赛必须会】输入/输出数组、`count/length/capacity`、采样率/频率、阈值/增益以及本 README 前文标出的 pin。它们直接影响题目范围、RAM、时间轴或物理单位。
- 【出问题再理解】Timer 时钟、DMA 通道、Event 路由、参考源和 IRQ。它们属于硬件链路，必须与 SysConfig 生成结果一致。
- 【以后进阶】多缓冲、运行时重配置、回调调度和 ISR 优化。先用最小示例完成一帧闭环，再处理吞吐或延迟。

### 常见错误 FAQ

- 参数错误：先检查指针非空、count/capacity 的单位是元素数、频率/阈值单位与上游一致。
- 硬件无结果：不要修改生成文件；回到 SysConfig 核对 pin、instance、时钟、Timer、DMA 和 Event 的完整链路。
- 结果异常：确认上一轮异步采集已经完成，真实 Fs/N/参考电压已传到算法，且没有在 DMA 使用期间改写 buffer。

### `signal_result_t SignalDACDMA_Init(signal_dac_dma_t *module, void *context, signal_dac_dma_start_fn start, signal_dac_dma_stop_fn stop);`

**它做什么：** 根据调用者提供的配置或对象完成一次初始化；通常在 `SYSCFG_DL_init()` 之后且仅调用一次。

**什么时候调用：** 根据调用者提供的配置或对象完成一次初始化；通常在 `SYSCFG_DL_init()` 之后且仅调用一次。

| 参数 | 类型 | 初学者解释 |
|---|---|---|
| `module` | `signal_dac_dma_t *` | `module`（`signal_dac_dma_t `）是该 API 的输入/输出参数；按本 README 前面的数据单位和边界条件准备。 |
| `context` | `void *` | 传给平台回调的用户上下文，由应用创建并保证在调用期间有效。 |
| `start` | `signal_dac_dma_start_fn` | `start`（`signal_dac_dma_start_fn`）是该 API 的输入/输出参数；按本 README 前面的数据单位和边界条件准备。 |
| `stop` | `signal_dac_dma_stop_fn` | `stop`（`signal_dac_dma_stop_fn`）是该 API 的输入/输出参数；按本 README 前面的数据单位和边界条件准备。 |

**返回：** 返回 signal_result_t 类型结果；调用者应检查该值。

**最小调用形状：** `SignalDACDMA_Init(...);`。可直接从 README_MINIMAL_EXAMPLE.c 复制正常流程；README_FULL_EXAMPLE.c 展示全部公开 API，其中取消类 API 会以 #if 0 隔离。

**注意：** 所有指针和数组都由调用者拥有；先检查返回值。异步采集、DMA 或回调还在使用 buffer 时，不能读取结果或改写该 buffer。

### `signal_result_t SignalDACDMA_Start(signal_dac_dma_t *module, const uint16_t *samples, size_t count, bool repeat);`

**它做什么：** 启动一轮新的硬件操作或异步传输；成功后按对应的完成查询 API 等待结果。

**什么时候调用：** 启动一轮新的硬件操作或异步传输；成功后按对应的完成查询 API 等待结果。

| 参数 | 类型 | 初学者解释 |
|---|---|---|
| `module` | `signal_dac_dma_t *` | `module`（`signal_dac_dma_t `）是该 API 的输入/输出参数；按本 README 前面的数据单位和边界条件准备。 |
| `samples` | `const uint16_t *` | 调用者持有的数据或缓冲区。容量、单位和读写方向以函数名及本 README 的输入输出说明为准；异步硬件操作完成前不得改写。 |
| `count` | `size_t` | 元素数量或容量，单位是“元素个数”而不是字节；必须与实际数组大小一致。 |
| `repeat` | `bool` | `repeat`（`bool`）是该 API 的输入/输出参数；按本 README 前面的数据单位和边界条件准备。 |

**返回：** 返回 signal_result_t 类型结果；调用者应检查该值。

**最小调用形状：** `SignalDACDMA_Start(...);`。可直接从 README_MINIMAL_EXAMPLE.c 复制正常流程；README_FULL_EXAMPLE.c 展示全部公开 API，其中取消类 API 会以 #if 0 隔离。

**注意：** 所有指针和数组都由调用者拥有；先检查返回值。异步采集、DMA 或回调还在使用 buffer 时，不能读取结果或改写该 buffer。

### `signal_result_t SignalDACDMA_Stop(signal_dac_dma_t *module);`

**它做什么：** 主动终止当前操作并释放模块占用的运行状态；只在需要取消本轮任务时调用。

**什么时候调用：** 主动终止当前操作并释放模块占用的运行状态；只在需要取消本轮任务时调用。

| 参数 | 类型 | 初学者解释 |
|---|---|---|
| `module` | `signal_dac_dma_t *` | `module`（`signal_dac_dma_t `）是该 API 的输入/输出参数；按本 README 前面的数据单位和边界条件准备。 |

**返回：** 返回 signal_result_t 类型结果；调用者应检查该值。

**最小调用形状：** `SignalDACDMA_Stop(...);`。可直接从 README_MINIMAL_EXAMPLE.c 复制正常流程；README_FULL_EXAMPLE.c 展示全部公开 API，其中取消类 API 会以 #if 0 隔离。

**注意：** 所有指针和数组都由调用者拥有；先检查返回值。异步采集、DMA 或回调还在使用 buffer 时，不能读取结果或改写该 buffer。

### `signal_module_status_t SignalDACDMA_GetModuleStatus();`

**它做什么：** 读取模块当前的验证成熟度或静态状态，不会启动硬件操作。

**什么时候调用：** 读取模块当前的验证成熟度或静态状态，不会启动硬件操作。

**参数：** 无。

**返回：** 返回 signal_module_status_t 类型结果；调用者应检查该值。

**最小调用形状：** `SignalDACDMA_GetModuleStatus(...);`。可直接从 README_MINIMAL_EXAMPLE.c 复制正常流程；README_FULL_EXAMPLE.c 展示全部公开 API，其中取消类 API 会以 #if 0 隔离。

**注意：** 所有指针和数组都由调用者拥有；先检查返回值。异步采集、DMA 或回调还在使用 buffer 时，不能读取结果或改写该 buffer。

### 示例、模块链与验收

- 最小入门：`README_MINIMAL_EXAMPLE.c`，只保留正常入口和结果读取。
- 全功能：`README_FULL_EXAMPLE.c`，以正确顺序展示当前头文件全部公开 API；`Stop` 等非常规路径不会默认执行。
- 模块链：先由上游提供单位、采样率和有效数据，再调用本模块；成功后将输出交给显示、控制、测量或下一步 DSP。硬件资源仍以 SysConfig 合约为唯一来源。
- 文档验收：README/API、两份示例和头文件会由 `tools/validate_beginner_documentation.ps1` 覆盖检查；这只表示文档与源码签名一致，不代替未进行的实板验证。

### 模块链

`上游采集/配置 -> 本模块 -> 检查返回值和结果 -> 测量、显示、控制或下一步 DSP`。只有确认本模块的输出单位和有效状态后，才交给下一模块。

