# ADC DMA：定时采样一帧波形

## 0. 什么时候用

当你要按固定采样率取得一帧 `N` 点 ADC 原始数据时使用。它把 Timer、Event、ADC 和 DMA 合成一次“启动—等待—得到 `raw[N]`”操作。只读一次 ADC 寄存器时不需要它，直接用 SysConfig + TI DriverLib。

> 不知道 `Fs` 为什么由 Timer event 决定，先读 [Clock/Timer/ADC/DAC 保姆教程](../../00_docs/MSPM0_SYSCONFIG_CLOCK_TIMER_ADC_DAC_BEGINNER_GUIDE.md)；不知道题目该选多少 `Fs/N`，读 [采样率选择指南](../../00_docs/SAMPLE_RATE_SELECTION_GUIDE.md)；现场回算看 [一页速查](../../00_docs/CLOCK_TIMER_ADC_DAC_QUICK_REFERENCE.md)。`ADC Clock` 不是 `Fs`。

## CCS SysConfig GUI Configuration

### Required resources

需要 `ADC12`、`DMA`、`TIMER`、`EVENT` 四个 SysConfig module。示例硬件 instance 是 `ADC0`、`DMA_CH0`、`TIMG0`；`SIGNAL_ADC`、`SIGNAL_ADC_DMA`、`SIGNAL_SAMPLE_TIMER` 是实例名。`DL_ADC12_*`/`DL_TimerG_*` 只是 DriverLib C 名称，不是 GUI module 名称。

### Step 1 - ADC12：逐级找到 MEM0 输入

GUI Path：左侧 `Software` -> `Add` -> `ADC12` -> 点击实例 `SIGNAL_ADC` -> `Basic Configuration` -> `Sampling Mode Configuration` -> `ADC Conversion Memory Configurations` -> `ADC Conversion Memory 0 Configuration`。

Action：在 `ADC Conversion Memory 0 Configuration` 内依次修改 `Input Channel = Channel 2`、`Device Pin Name = 当前接线对应的 ADC pin`、`Reference Voltage = 当前参考源`；展开同页 `Advanced Configuration`，设置 `Conversion Resolution = 12-bit`、`Power Down Mode = Manual`、`Desired Sample Time 0 = 62.5 ns`。回到 `Basic Configuration` -> `Sampling Mode Configuration`，将 `Trigger Source/Trigger Mode = Event`、`Repeat/Auto Start` 打开；再进入 `Event Configuration` -> `Event Subscriber Channel ID` 填入与 Timer Publisher 相同的号码。P01 的 `ADC0.2 / PA25 / Channel 2` 只用于说明位置，换 pin/instance 时以当前工程为准。

### Step 2 - Clock Configuration

GUI Path：`SIGNAL_ADC` -> `Basic Configuration` -> `Clock Configuration`（若折叠在 `Advanced Configuration`，展开包含 `Clock Source/Divider` 的子项）。选择 ADC functional clock source/divider，并读取页面显示的 calculated clock；ADC functional clock 只负责转换预算，不能当成 `Fs`。不要把 `DL_ADC12_CLOCK_ULPCLK` 当作 GUI 菜单文字。

共享教材：[MSPM0G3507 SysConfig 时钟、Timer、ADC 与 DAC 保姆教程](../../00_docs/MSPM0_SYSCONFIG_CLOCK_TIMER_ADC_DAC_BEGINNER_GUIDE.md)。

### Step 3 - DMA and Event

GUI Path：`SIGNAL_ADC` -> `DMA Configuration` -> `Configure DMA Trigger` -> `Enable DMA Trigger` -> `DMA Trigger Source/ADC Conversion Memory Result Loaded`，选择 `MEM0 result loaded`。点击自动出现的 DMA channel，进入 `DMA` -> `DMA Channels` -> `SIGNAL_ADC_DMA`，设置 `DMA Channel`、`Address Mode = Peripheral to Block`、`Source Length = Half Word`、`Destination Length = Half Word`、`Destination Address Direction = Block`、`Transfer Mode = Single`。

GUI Path：`SIGNAL_ADC` -> `Event Configuration` -> `Event Subscriber Channel ID = 1`；左侧点击 `TIMER` 实例 `SIGNAL_SAMPLE_TIMER` -> `Event Configuration` -> `Channel 1 Publisher` -> `Event 1 Publisher Channel ID = 1`，再在 `Event 1 Enable Controller Interrupts` 选择 `ZERO_EVENT`。Publisher 与 Subscriber 的数字必须完全相同。

### Step 4 - Timer and PinMux

GUI Path：左侧 `Software` -> `Add` -> `TIMER` -> 实例 `SIGNAL_SAMPLE_TIMER` -> `Basic Configuration` -> `Timer Peripheral`；继续展开 `Clock Configuration` -> `Timer Clock Source`、`Timer Clock Divider`、`Timer Clock Prescaler`；回到 `Basic Configuration` -> `Timer Mode`、`Desired Timer Period`。P01 基线为 `TIMG0 / BUSCLK / 1 / 1 / Periodic Down Counting / 10 us`。最后在同一实例 `Event Configuration` 完成 Publisher。核对页面中的 `Timer Clock Frequency` 与 `Actual Timer Period`，代码运行时重设 load 仍必须使用真实计数时钟。

### Expected generated symbols

Generate 后在 `ti_msp_dl_config.h` 核对 `SIGNAL_ADC_INST`、`SIGNAL_ADC_DMA_CHAN_ID`、`SIGNAL_SAMPLE_TIMER_INST` 及相应 `*_LOAD_VALUE`/`*_IRQN`。PROJECT_AUDIT 记录 `GUI field -> .syscfg property -> generated symbol`。

### Final checklist

- ADC trigger、Timer publisher、Event subscriber、DMA trigger 完整闭合。
- ADC sample time 能满足 Timer event 间隔，Timer 实际周期与应用 rate 一致。
- 保存 `.syscfg` 后 CCS 已重新生成；资源冲突视图无冲突。

### Common mistakes

把 ADC clock 当 `Fs`；Event channel 不一致；DMA 宽度或方向错误；把 `TIMER`/`ADC12` 的 DriverLib 名称当 GUI module 名称。

### Do not change

不要直接编辑 `.syscfg` 或 `ti_msp_dl_config.c/.h`，不要在当前工程已有资源时照搬 profile 的 pin、instance、DMA channel 或 Event channel。

## 1. 30 秒接入路线

你需要复制：`signal_adc_dma.c`、`signal_adc_dma.h`、`01_bsp/common/signal_status.h`。本模块【需要 SysConfig】。

1. 按第 3 节配置 SysConfig。
2. 把第 4 节列出的 3 个文件复制到母版 `modules/`。
3. 把第 5、7、8 节代码放进 `main.c`。
4. 只先修改 `Fs`、`N` 和 ADC Pin。
5. Refresh 工程并 Build。

## 2. 输入和输出

- 输入：目标采样率 `sample_rate_hz`、采样点数 `sample_count`、ADC 模拟输入。
- 输出：`uint16_t raw[N]`、实际配置触发率 Hz 和完成状态。
- Buffer 在 DMA 完成前不能释放或改写；`N` 范围为 1～65535。

## 3. SysConfig / Pin

【需要 SysConfig】。`PROFILE_01_ADC_CAPTURE` 只作为配置项参考；实际操作是在 CCS 中双击目标工程 `.syscfg`，使用 SysConfig 图形界面配置。不要直接编辑 `.syscfg` 文本或生成的 `ti_msp_dl_config.c/.h`。

1. 打开母版 `.syscfg`，添加 `ADC12`，实例名必须为 `SIGNAL_ADC`。
2. 在 ADC 的 Conversion Memory 0 选择合法模拟输入。示例是 `ADC0.2 / PA25`。
3. ADC 触发源选 Event，Repeat Mode 打开，DMA trigger 选 `MEM0 result loaded`。
4. 给 ADC 配 DMA，名称为 `SIGNAL_ADC_DMA`；source/destination 长度均为 Half Word，方向 peripheral-to-block。
5. 添加周期 Timer，实例名必须为 `SIGNAL_SAMPLE_TIMER`；示例用 `TIMG0`、BUSCLK/1/1。
6. Timer ZERO event 发布到 ADC 订阅的同一 Event Channel。
7. 打开 ADC DMA-done interrupt，保存 `.syscfg`，等待 CCS 自动重新生成；确认生成了 `SIGNAL_ADC_*`、`SIGNAL_ADC_DMA_*` 和 `SIGNAL_SAMPLE_TIMER_*` 宏。
8. 在资源冲突视图检查 Pin、DMA channel、Timer 和 Event channel 没有被其他模块占用，再 Clean/Build。

换输入脚时，只在 SysConfig 重新选择合法 ADC Pin/Channel；模块 `.c` 不写死 PA25，不需要同步改源码。Timer 页面里的初始周期不决定最终 `Fs`，`SignalADC_Init` 会用 `sample_rate_hz` 重设 load 值。

对于 10 Hz 这类低频信号，硬件 Timer 不需要改成 10 Hz；它仍按 ADC 采样率运行。需要改变的是采样窗口：过零测频建议至少覆盖 3 个最低频率周期，FFT 还要满足 `delta_f=Fs/N`。例如 `Fs=1 kHz、N=1024` 的窗口为 1.024 s，适合从 10 Hz 开始验证；默认 `100 kHz、1024点` 只有 10.24 ms，不能装下一个 10 Hz 周期。

## 4. 复制哪些文件

从本目录复制：

- `signal_adc_dma.c`
- `signal_adc_dma.h`

再复制：

- `01_bsp/common/signal_status.h`

全部放到母版 `modules/`。不需要复制 Adapter、Platform、Application 或其他 BSP 文件。母版已包含 `${PROJECT_ROOT}/modules`；在 CCS 中 Refresh，并确认 `.c` 没有 Exclude from Build。

## 5. main.c 顶部复制什么

```c
#include <stdint.h>
#include "ti_msp_dl_config.h"
#include "signal_adc_dma.h"

// ===== 你需要根据题目修改 =====
#define SIGNAL_SAMPLE_RATE_HZ  (100000U)
#define SIGNAL_SAMPLE_COUNT    (1024U)

// ===== 一般不用改 =====
static uint16_t g_raw[SIGNAL_SAMPLE_COUNT];
volatile signal_result_t g_adc_status;
```

## 6. 比赛参数

| 题目出现什么变化 | 修改什么 | 在哪里改 |
|---|---|---|
| 最高输入频率变高 | 增大 `SIGNAL_SAMPLE_RATE_HZ`，同时检查 ADC 建立时间 | `main.c` |
| 要更快刷新/少用 RAM | 减小 `SIGNAL_SAMPLE_COUNT` | `main.c` |
| FFT 分辨率要提高 | 通常增大 `N` 或调整 `Fs` | `main.c`，并同步 FFT buffer |
| 换模拟输入端子 | ADC Pin/Channel | `.syscfg` |
| 时钟树或 Timer 分频改变 | `timer_clock_hz` 必须改成真实计数时钟 | config 初始化 |

`uint16_t raw[N]` 占 `2N` 字节；例如 N=1024 占 2048 字节。

## 7. 初始化区复制什么

放在 `SYSCFG_DL_init();` 之后、进入 `while (1)` 之前：

```c
const signal_adc_dma_config_t adc_config = {
    SIGNAL_SAMPLE_RATE_HZ,
    CPUCLK_FREQ,  // 仅当 Timer 真的是 BUSCLK/1/1
    65536U
};

SYSCFG_DL_init();
g_adc_status = SignalADC_Init(&adc_config);
if (g_adc_status != SIGNAL_RESULT_OK) {
    while (1) { }
}
```

## 8. while(1) 中复制什么

```c
g_adc_status = SignalADC_Start(g_raw, SIGNAL_SAMPLE_COUNT);
if (g_adc_status != SIGNAL_RESULT_OK) {
    while (1) { }
}

while (!SignalADC_IsFinished()) {
    __WFI();
}

// ===== 这里写你自己的逻辑 =====
// 现在 g_raw[0] ... g_raw[N-1] 已经有效。
```

完整、与当前头文件一致的可编译版本见 `README_MINIMAL_EXAMPLE.c`。

## 9. 结果在哪里

最直接使用 `g_raw[]`。也可以调用 `SignalADC_GetBuffer()`、`SignalADC_GetSampleCount()` 和 `SignalADC_GetConfiguredTriggerRate()`。最后一个值是 Timer 整数分频推导值，不是示波器实测采样率。

## 10. 接下一个模块

```text
g_raw uint16_t[N]
  -> ADC To Voltage
  -> float voltage_v[N]
  -> VPP / RMS / AC RMS / Remove DC
```

做频谱时继续 `Remove DC -> Window -> FFT -> FFT Magnitude`。

## 11. Build 与最小验证

Clean/Build 后先在断点处看 `g_adc_status == SIGNAL_RESULT_OK`，再看 `g_raw[]` 是否随输入变化。板上验证采样率时应给已知频率信号并用波形/FFT 或外部仪器核对。

隔离 COPY TEST：`SysConfig PASS / Compile PASS / Full Link PASS`；测试镜像 Flash 2656 B、SRAM（含栈）565 B。该测试不等于新接线已实板验证；本模块另有既有 TMP6131 板测记录，成熟度为 `BOARD_VERIFIED`。

## 12. 常见错误

- `signal_adc_dma.h not found`：文件没放入 `modules/`，或工程未 Refresh。
- `SIGNAL_ADC_*` 宏不存在：SysConfig 实例名不一致。
- 一直等不到完成：检查 Event route、ADC DMA trigger、DMA channel 和中断。
- 频率不准：`timer_clock_hz` 写成 CPU 主频，但 Timer 实际有分频。
- 10 Hz 过零/FFT 无结果：采样窗口太短；按 `frame_time=N/Fs` 重新选择 Fs/N，而不是把 ADC 采样 Timer 直接改成 10 Hz。
- 第二次启动返回 BUSY：上一次 DMA 尚未完成。

## 13. API Reference

- `SignalADC_Init(config)`：在 `SYSCFG_DL_init()` 后初始化。
- `SignalADC_SetSampleRate(hz)`：空闲时修改采样率。
- `SignalADC_Start(buffer, count)`：启动一帧 DMA。
- `SignalADC_Stop()`：中止采集。
- `SignalADC_IsFinished()` / `SignalADC_GetStatus()`：查询状态。
- `SignalADC_GetBuffer()` / `SignalADC_GetSampleCount()`：取得最近一帧描述。
- `SignalADC_GetConfiguredTriggerRate()`：取得整数 Timer 配置对应的 Hz。
- `SignalADC_GetModuleMaturity()`：取得验证成熟度。

## 17. 统一 API 教程（已按当前头文件核对）

本节由当前公开头文件、实现中实际出现的状态码和正式模块注册表生成。它补充前文的场景教程；函数签名变化时必须重新运行 `tools/upgrade_formal_beginner_docs.ps1`，不要手工保留旧 API。

遵循仓库的 [Beginner README 标准](../../00_docs/BEGINNER_README_STANDARD.md)：先用最小示例完成一次正常数据流，再按需要阅读全功能示例和本节 API 细节。

### 什么时候用 / 什么时候不要用

当题目需要“adc_dma”目录对应的公开功能，并且输入数据、单位和硬件资源满足前文约束时使用本模块。若只需要更简单的上游功能、输入尚未准备好，或需要不同的数据模型/外设资源，应先选择相邻模块而不是强行调用本 API。

### 输入 / 输出

输入由各 API 的只读数组、配置、频率/阈值和平台对象组成；输出写入 result/output/buffer 参数或由 Get API 返回。调用者负责数组容量、生命周期和物理单位；失败返回时输出不是有效结果。

### 调用顺序

```text
SignalADC_Init -> SignalADC_SetSampleRate -> SignalADC_Start -> SignalADC_IsFinished -> SignalADC_GetStatus -> SignalADC_GetBuffer -> SignalADC_GetSampleCount -> SignalADC_GetConfiguredTriggerRate -> SignalADC_GetModuleMaturity -> SignalADC_Stop
```

`Init` 一般仅一次；`Set/Configure` 仅在参数变化且模块空闲时执行；`Start/Process/Generate` 是每帧或每次任务入口；`Get/Is` 用于读取已完成的结果；`Stop` 只在需要取消时调用。若本模块没有其中某类 API，以实际列出的函数为准。

### SysConfig 边界

本模块需要 SysConfig。先在 CCS 的 .syscfg 添加并核对：ADC12、DMA、EVENT、TIMER；再按前文的模块专用 GUI 步骤选择实际 pin/instance。保存后让 CCS 重新生成配置，核对生成宏；不要直接修改 	i_msp_dl_config.c/.h，也不要照抄示例 pin 或 DMA/Event 编号。

### 参数分级

- 【比赛必须会】输入/输出数组、`count/length/capacity`、采样率/频率、阈值/增益以及本 README 前文标出的 pin。它们直接影响题目范围、RAM、时间轴或物理单位。
- 【出问题再理解】Timer 时钟、DMA 通道、Event 路由、参考源和 IRQ。它们属于硬件链路，必须与 SysConfig 生成结果一致。
- 【以后进阶】多缓冲、运行时重配置、回调调度和 ISR 优化。先用最小示例完成一帧闭环，再处理吞吐或延迟。

### 常见错误 FAQ

- 参数错误：先检查指针非空、count/capacity 的单位是元素数、频率/阈值单位与上游一致。
- 硬件无结果：不要修改生成文件；回到 SysConfig 核对 pin、instance、时钟、Timer、DMA 和 Event 的完整链路。
- 结果异常：确认上一轮异步采集已经完成，真实 Fs/N/参考电压已传到算法，且没有在 DMA 使用期间改写 buffer。

### `signal_result_t SignalADC_Init(const signal_adc_dma_config_t *config);`

**它做什么：** 初始化 ADC+DMA 模块的运行时状态。

**什么时候调用：** 根据调用者提供的配置或对象完成一次初始化；通常在 `SYSCFG_DL_init()` 之后且仅调用一次。

| 参数 | 类型 | 初学者解释 |
|---|---|---|
| `config` | `const signal_adc_dma_config_t *` | 采样率、定时器计数时钟和最大周期计数。 |

**返回：** SIGNAL_RESULT_OK 表示成功；其他值表示参数或范围错误。

**最小调用形状：** `SignalADC_Init(...);`。可直接从 README_MINIMAL_EXAMPLE.c 复制正常流程；README_FULL_EXAMPLE.c 展示全部公开 API，其中取消类 API 会以 #if 0 隔离。

**注意：** 所有指针和数组都由调用者拥有；先检查返回值。异步采集、DMA 或回调还在使用 buffer 时，不能读取结果或改写该 buffer。

### `signal_result_t SignalADC_SetSampleRate(uint32_t sample_rate_hz);`

**它做什么：** 修改目标采样事件率并计算最近的整数 Timer 配置触发率。

**什么时候调用：** 修改模块的一个运行参数；若模块有 BUSY/RUNNING 状态，应在空闲时修改。

| 参数 | 类型 | 初学者解释 |
|---|---|---|
| `sample_rate_hz` | `uint32_t` | 目标采样事件率，单位 Hz。 |

**返回：** SIGNAL_RESULT_OK 表示成功；运行中返回 SIGNAL_RESULT_BUSY。

**最小调用形状：** `SignalADC_SetSampleRate(...);`。可直接从 README_MINIMAL_EXAMPLE.c 复制正常流程；README_FULL_EXAMPLE.c 展示全部公开 API，其中取消类 API 会以 #if 0 隔离。

**注意：** 所有指针和数组都由调用者拥有；先检查返回值。异步采集、DMA 或回调还在使用 buffer 时，不能读取结果或改写该 buffer。

### `signal_result_t SignalADC_Start(uint16_t *buffer, uint16_t sample_count);`

**它做什么：** 启动一次 N 点采集。

**什么时候调用：** 启动一轮新的硬件操作或异步传输；成功后按对应的完成查询 API 等待结果。

| 参数 | 类型 | 初学者解释 |
|---|---|---|
| `buffer` | `uint16_t *` | DMA 目标缓冲区，元素类型必须为 uint16_t。 |
| `sample_count` | `uint16_t` | 采样点数，范围 1~65535。 |

**返回：** SIGNAL_RESULT_OK 表示已启动。

**最小调用形状：** `SignalADC_Start(...);`。可直接从 README_MINIMAL_EXAMPLE.c 复制正常流程；README_FULL_EXAMPLE.c 展示全部公开 API，其中取消类 API 会以 #if 0 隔离。

**注意：** 所有指针和数组都由调用者拥有；先检查返回值。异步采集、DMA 或回调还在使用 buffer 时，不能读取结果或改写该 buffer。

### `void SignalADC_Stop();`

**它做什么：** 立即停止当前采集并把模块恢复为空闲状态。

**什么时候调用：** 主动终止当前操作并释放模块占用的运行状态；只在需要取消本轮任务时调用。

**参数：** 无。

**返回：** 无。

**最小调用形状：** `SignalADC_Stop(...);`。可直接从 README_MINIMAL_EXAMPLE.c 复制正常流程；README_FULL_EXAMPLE.c 展示全部公开 API，其中取消类 API 会以 #if 0 隔离。

**注意：** 所有指针和数组都由调用者拥有；先检查返回值。异步采集、DMA 或回调还在使用 buffer 时，不能读取结果或改写该 buffer。

### `bool SignalADC_IsFinished();`

**它做什么：** 查询本次采集是否完成。

**什么时候调用：** 查询一个布尔条件，例如一帧数据是否已准备好；它不会等待也不会处理数据。

**参数：** 无。

**返回：** 完成返回 true，否则返回 false。

**最小调用形状：** `SignalADC_IsFinished(...);`。可直接从 README_MINIMAL_EXAMPLE.c 复制正常流程；README_FULL_EXAMPLE.c 展示全部公开 API，其中取消类 API 会以 #if 0 隔离。

**注意：** 所有指针和数组都由调用者拥有；先检查返回值。异步采集、DMA 或回调还在使用 buffer 时，不能读取结果或改写该 buffer。

### `signal_status_t SignalADC_GetStatus();`

**它做什么：** 查询模块当前状态。

**什么时候调用：** 读取最近一次操作保存的状态、结果或配置；先确认前置操作已经成功。

**参数：** 无。

**返回：** MODULE_IDLE、MODULE_RUNNING、MODULE_DONE 或 MODULE_ERROR。

**最小调用形状：** `SignalADC_GetStatus(...);`。可直接从 README_MINIMAL_EXAMPLE.c 复制正常流程；README_FULL_EXAMPLE.c 展示全部公开 API，其中取消类 API 会以 #if 0 隔离。

**注意：** 所有指针和数组都由调用者拥有；先检查返回值。异步采集、DMA 或回调还在使用 buffer 时，不能读取结果或改写该 buffer。

### `const uint16_t * SignalADC_GetBuffer();`

**它做什么：** 取得最近一次采集使用的缓冲区。

**什么时候调用：** 读取最近一次操作保存的状态、结果或配置；先确认前置操作已经成功。

**参数：** 无。

**返回：** 缓冲区只读指针；尚未启动过采集时返回空指针。

**最小调用形状：** `SignalADC_GetBuffer(...);`。可直接从 README_MINIMAL_EXAMPLE.c 复制正常流程；README_FULL_EXAMPLE.c 展示全部公开 API，其中取消类 API 会以 #if 0 隔离。

**注意：** 所有指针和数组都由调用者拥有；先检查返回值。异步采集、DMA 或回调还在使用 buffer 时，不能读取结果或改写该 buffer。

### `uint16_t SignalADC_GetSampleCount();`

**它做什么：** 取得最近一次采集的点数。

**什么时候调用：** 读取最近一次操作保存的状态、结果或配置；先确认前置操作已经成功。

**参数：** 无。

**返回：** 采样点数；尚未启动过采集时返回 0。

**最小调用形状：** `SignalADC_GetSampleCount(...);`。可直接从 README_MINIMAL_EXAMPLE.c 复制正常流程；README_FULL_EXAMPLE.c 展示全部公开 API，其中取消类 API 会以 #if 0 隔离。

**注意：** 所有指针和数组都由调用者拥有；先检查返回值。异步采集、DMA 或回调还在使用 buffer 时，不能读取结果或改写该 buffer。

### `uint32_t SignalADC_GetConfiguredTriggerRate();`

**它做什么：** 取得由 Timer 整数计数配置推导出的事件触发率。

**什么时候调用：** 读取最近一次操作保存的状态、结果或配置；先确认前置操作已经成功。

**参数：** 无。

**返回：** 配置触发率，单位 Hz；初始化失败时返回 0。

**最小调用形状：** `SignalADC_GetConfiguredTriggerRate(...);`。可直接从 README_MINIMAL_EXAMPLE.c 复制正常流程；README_FULL_EXAMPLE.c 展示全部公开 API，其中取消类 API 会以 #if 0 隔离。

**注意：** 所有指针和数组都由调用者拥有；先检查返回值。异步采集、DMA 或回调还在使用 buffer 时，不能读取结果或改写该 buffer。

### `signal_module_status_t SignalADC_GetModuleMaturity();`

**它做什么：** 返回模块证据成熟度；当前来自 2026-08-07 板载 TMP6131 验收。

**什么时候调用：** 读取模块当前的验证成熟度或静态状态，不会启动硬件操作。

**参数：** 无。

**返回：** 当前实现中出现的返回/成熟度枚举值：`MODULE_STATUS_BOARD_VERIFIED`。

**最小调用形状：** `SignalADC_GetModuleMaturity(...);`。可直接从 README_MINIMAL_EXAMPLE.c 复制正常流程；README_FULL_EXAMPLE.c 展示全部公开 API，其中取消类 API 会以 #if 0 隔离。

**注意：** 所有指针和数组都由调用者拥有；先检查返回值。异步采集、DMA 或回调还在使用 buffer 时，不能读取结果或改写该 buffer。

### 示例、模块链与验收

- 最小入门：`README_MINIMAL_EXAMPLE.c`，只保留正常入口和结果读取。
- 全功能：`README_FULL_EXAMPLE.c`，以正确顺序展示当前头文件全部公开 API；`Stop` 等非常规路径不会默认执行。
- 模块链：先由上游提供单位、采样率和有效数据，再调用本模块；成功后将输出交给显示、控制、测量或下一步 DSP。硬件资源仍以 SysConfig 合约为唯一来源。
- 文档验收：README/API、两份示例和头文件会由 `tools/validate_beginner_documentation.ps1` 覆盖检查；这只表示文档与源码签名一致，不代替未进行的实板验证。

### 模块链

`上游采集/配置 -> 本模块 -> 检查返回值和结果 -> 测量、显示、控制或下一步 DSP`。只有确认本模块的输出单位和有效状态后，才交给下一模块。

