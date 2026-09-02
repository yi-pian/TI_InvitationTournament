# DDS：按相位累加器产生可调频采样序列

> **概念纠正：MSPM0G3507 没有专用硬件 DDS peripheral。** 本目录是 Software DDS，只生成样本；模拟输出仍需 Timer/Event → DAC DMA → Internal DAC。先按每周期点数、DAC 建立时间和波形质量判断是否应改用外置 DDS，见 [MSPM0G3507 资源能力指南](../../00_docs/MSPM0G3507_RESOURCE_CAPABILITY_GUIDE.md) 和 [内部/外置选择指南](../../00_docs/INTERNAL_VS_EXTERNAL_SELECTION_GUIDE.md)。

> 不知道什么时候改 DDS frequency、什么时候改 DAC `Fupdate`/Timer，读 [Clock/Timer/ADC/DAC 保姆教程](../../00_docs/MSPM0_SYSCONFIG_CLOCK_TIMER_ADC_DAC_BEGINNER_GUIDE.md)；更新率倒推见 [采样率选择指南](../../00_docs/SAMPLE_RATE_SELECTION_GUIDE.md)，现场回算看 [一页速查](../../00_docs/CLOCK_TIMER_ADC_DAC_QUICK_REFERENCE.md)。

【优先使用】中低频任意周期波、软件可调频率/相位/幅度，且 1 MSPS DAC 下每周期样本充足。

【慎用】100 kHz 级正弦：1 MSPS 只有 10 点/周期，必须验证建立、重建滤波和 THD。

【不要用】500 kHz 高质量正弦或 MHz 波形；外置 DDS/高速 DAC 优先。

## 0. 什么时候用

当你要在固定 DAC 更新率下，软件修改输出频率/初相，并把查表样本交给 DAC DMA 时使用。DDS 本身不碰 DAC、不配置 Pin，也不会直接产生模拟电压。

## 1. 30 秒接入路线

你需要复制：`signal_dds.c`、`signal_dds.h`、`01_bsp/common/signal_status.h`。DDS 本身【不需要 SysConfig】。

复制 DDS 的 `.c/.h` 和 `signal_status.h`，准备一个 2 次幂长度波表，初始化频率与更新率，调用 `SignalDDS_Fill` 得到输出数组，再把数组交给 DAC DMA。

## 2. 输入和输出

- 输入：`uint16_t table[table_count]`、目标频率 Hz、DAC 更新率 Hz、初始相位累加值。
- 输出：`uint16_t output[count]`，可直接作为 DAC DMA 样本。
- `table_count` 必须至少 2 且为 2 的幂；`output_frequency_hz < update_rate_hz / 2`。

## 3. SysConfig / Pin

【DDS 本身不需要 SysConfig】。如果下一步接 MSPM0 内部 DAC，请按 DAC DMA README 配置；若接外部 DAC，则按对应 SPI/并口器件说明配置。

## 4. 复制哪些文件

从本目录复制：

- `signal_dds.c`
- `signal_dds.h`

再复制 `01_bsp/common/signal_status.h`。全部放入母版 `modules/`；不需要 `signal_math.h`、Adapter 或 Platform。

## 5. main.c 顶部复制什么

```c
#include <stdint.h>
#include "signal_dds.h"

// ===== 你需要根据题目修改 =====
#define SIGNAL_DDS_UPDATE_RATE_HZ  (100000.0f)
#define SIGNAL_DDS_FREQUENCY_HZ    (1000.0f)
#define SIGNAL_DDS_OUTPUT_COUNT    (1024U)

static const uint16_t g_sine_table[8] = {
    2048U, 3496U, 4095U, 3496U, 2048U, 600U, 0U, 600U
};

// ===== 一般不用改 =====
static uint16_t g_dds_output[SIGNAL_DDS_OUTPUT_COUNT];
static signal_dds_t g_dds;
volatile signal_result_t g_dds_status;
```

## 6. 比赛参数

| 题目变化 | 修改 |
|---|---|
| 输出频率变化 | `SIGNAL_DDS_FREQUENCY_HZ` 或运行时调用 `SignalDDS_SetFrequency` |
| DAC 更新率变化 | `SIGNAL_DDS_UPDATE_RATE_HZ`，必须与 DAC DMA 实际 rate 一致 |
| 幅值/偏置变化 | 重新生成波表中的 12-bit code |
| 初相变化 | `SignalDDS_Init` 的 `initial_phase` |
| 要更长 DMA block | `SIGNAL_DDS_OUTPUT_COUNT`，RAM 按 2N 字节增加 |

## 7. 初始化/处理区复制什么

放在 `SYSCFG_DL_init()` 之后；DDS 无硬件初始化，但母版仍先初始化系统：

```c
g_dds_status = SignalDDS_Init(
    &g_dds,
    g_sine_table,
    sizeof(g_sine_table) / sizeof(g_sine_table[0]),
    SIGNAL_DDS_FREQUENCY_HZ,
    SIGNAL_DDS_UPDATE_RATE_HZ,
    0U);

if (g_dds_status == SIGNAL_RESULT_OK) {
    g_dds_status = SignalDDS_Fill(
        &g_dds, g_dds_output, SIGNAL_DDS_OUTPUT_COUNT);
}
```

## 8. while(1) / 自己的逻辑

DDS 预填充后，通常把 `g_dds_output` 一次交给 DAC DMA 循环播放。要改变频率时：停止 DAC DMA，调用 `SignalDDS_SetFrequency`，重新 Fill，再重新 Start。

```c
// ===== 这里写你自己的逻辑 =====
// SignalDACDMA_MSPM0_Start(g_dds_output,
//     SIGNAL_DDS_OUTPUT_COUNT, true);
```

完整独立示例见 `README_MINIMAL_EXAMPLE.c`。

## 9. 结果和下一步

```text
DDS -> uint16_t g_dds_output[N] -> DAC DMA -> DAC0/PA15
```

DDS 频率分辨率由 32-bit phase accumulator 决定，但最终模拟质量还受更新率、波表、DAC settling 和重建滤波器影响。

## 10. Build 与最小验证

先在 Debugger 查看 `g_dds_output[]` 是否周期变化，再接 DAC DMA 用示波器测频。隔离 COPY TEST：`SysConfig / Compile / Full Link PASS`，Flash 3368 B、SRAM（含栈）561 B；没有把“数组生成成功”冒充 DAC 板测，状态 `BUILD_VERIFIED`。

## 11. 常见错误

- `INVALID_ARGUMENT`：波表长度不是 2 次幂、频率非正、或频率达到 Nyquist。
- 实际输出频率不对：DDS 使用的 update rate 与 DAC Timer 真实 rate 不一致。
- 幅值/偏置不对：问题在波表 code、VREF 或模拟输出级，不在 tuning word。
- 波形断点：DMA block 太短或换表时没有先 Stop。

## 12. API Reference

- `SignalDDS_Init(dds, table, table_count, output_frequency_hz, update_rate_hz, initial_phase)`
- `SignalDDS_SetFrequency(dds, output_frequency_hz, update_rate_hz)`
- `SignalDDS_Next(dds)`：生成一个样本。
- `SignalDDS_Fill(dds, output, count)`：批量生成样本。
- `SignalDDS_GetConfiguredFrequency(dds, update_rate_hz)`
- `SignalDDS_GetModuleStatus()`

## 17. 统一 API 教程（已按当前头文件核对）

本节由当前公开头文件、实现中实际出现的状态码和正式模块注册表生成。它补充前文的场景教程；函数签名变化时必须重新运行 `tools/upgrade_formal_beginner_docs.ps1`，不要手工保留旧 API。

遵循仓库的 [Beginner README 标准](../../00_docs/BEGINNER_README_STANDARD.md)：先用最小示例完成一次正常数据流，再按需要阅读全功能示例和本节 API 细节。

### 什么时候用 / 什么时候不要用

当题目需要“dds”目录对应的公开功能，并且输入数据、单位和硬件资源满足前文约束时使用本模块。若只需要更简单的上游功能、输入尚未准备好，或需要不同的数据模型/外设资源，应先选择相邻模块而不是强行调用本 API。

### 输入 / 输出

输入由各 API 的只读数组、配置、频率/阈值和平台对象组成；输出写入 result/output/buffer 参数或由 Get API 返回。调用者负责数组容量、生命周期和物理单位；失败返回时输出不是有效结果。

### 调用顺序

```text
SignalDDS_Init -> SignalDDS_SetFrequency -> SignalDDS_GetConfiguredFrequency -> SignalDDS_GetModuleStatus -> SignalDDS_Next -> SignalDDS_Fill
```

`Init` 一般仅一次；`Set/Configure` 仅在参数变化且模块空闲时执行；`Start/Process/Generate` 是每帧或每次任务入口；`Get/Is` 用于读取已完成的结果；`Stop` 只在需要取消时调用。若本模块没有其中某类 API，以实际列出的函数为准。

### SysConfig 边界

本模块是纯软件/算法模块，**不需要 SysConfig**。ADC、DAC、Timer、DMA、引脚和时钟由上游模块配置；调用时只把真实的采样率、数组长度、单位等事实传入。

### 参数分级

- 【比赛必须会】输入/输出数组、`count/length/capacity`、采样率/频率、阈值/增益以及本 README 前文标出的 pin。它们直接影响题目范围、RAM、时间轴或物理单位。
- 【出问题再理解】Timer 时钟、DMA 通道、Event 路由、参考源和 IRQ。它们属于硬件链路，必须与 SysConfig 生成结果一致。
- 【以后进阶】多缓冲、运行时重配置、回调调度和 ISR 优化。先用最小示例完成一帧闭环，再处理吞吐或延迟。

### 常见错误 FAQ

- 参数错误：先检查指针非空、count/capacity 的单位是元素数、频率/阈值单位与上游一致。
- 硬件无结果：不要修改生成文件；回到 SysConfig 核对 pin、instance、时钟、Timer、DMA 和 Event 的完整链路。
- 结果异常：确认上一轮异步采集已经完成，真实 Fs/N/参考电压已传到算法，且没有在 DMA 使用期间改写 buffer。

### `signal_result_t SignalDDS_Init(signal_dds_t *dds, const uint16_t *table, size_t table_count, float output_frequency_hz, float update_rate_hz, uint32_t initial_phase);`

**它做什么：** 根据调用者提供的配置或对象完成一次初始化；通常在 `SYSCFG_DL_init()` 之后且仅调用一次。

**什么时候调用：** 根据调用者提供的配置或对象完成一次初始化；通常在 `SYSCFG_DL_init()` 之后且仅调用一次。

| 参数 | 类型 | 初学者解释 |
|---|---|---|
| `dds` | `signal_dds_t *` | `dds`（`signal_dds_t `）是该 API 的输入/输出参数；按本 README 前面的数据单位和边界条件准备。 |
| `table` | `const uint16_t *` | 调用者持有的数据或缓冲区。容量、单位和读写方向以函数名及本 README 的输入输出说明为准；异步硬件操作完成前不得改写。 |
| `table_count` | `size_t` | 调用者持有的数据或缓冲区。容量、单位和读写方向以函数名及本 README 的输入输出说明为准；异步硬件操作完成前不得改写。 |
| `output_frequency_hz` | `float` | 由调用者分配的输出对象/数组。成功返回后才读取其中内容。 |
| `update_rate_hz` | `float` | 频率/速率参数，单位 Hz。必须传入实际配置或测得的上游数值，不能把 ADC 时钟名称直接当采样率。 |
| `initial_phase` | `uint32_t` | `initial_phase`（`uint32_t`）是该 API 的输入/输出参数；按本 README 前面的数据单位和边界条件准备。 |

**返回：** 返回 signal_result_t 类型结果；调用者应检查该值。

**最小调用形状：** `SignalDDS_Init(...);`。可直接从 README_MINIMAL_EXAMPLE.c 复制正常流程；README_FULL_EXAMPLE.c 展示全部公开 API，其中取消类 API 会以 #if 0 隔离。

**注意：** 所有指针和数组都由调用者拥有；先检查返回值。异步采集、DMA 或回调还在使用 buffer 时，不能读取结果或改写该 buffer。

### `signal_result_t SignalDDS_SetFrequency(signal_dds_t *dds, float output_frequency_hz, float update_rate_hz);`

**它做什么：** 修改模块的一个运行参数；若模块有 BUSY/RUNNING 状态，应在空闲时修改。

**什么时候调用：** 修改模块的一个运行参数；若模块有 BUSY/RUNNING 状态，应在空闲时修改。

| 参数 | 类型 | 初学者解释 |
|---|---|---|
| `dds` | `signal_dds_t *` | `dds`（`signal_dds_t `）是该 API 的输入/输出参数；按本 README 前面的数据单位和边界条件准备。 |
| `output_frequency_hz` | `float` | 由调用者分配的输出对象/数组。成功返回后才读取其中内容。 |
| `update_rate_hz` | `float` | 频率/速率参数，单位 Hz。必须传入实际配置或测得的上游数值，不能把 ADC 时钟名称直接当采样率。 |

**返回：** 返回 signal_result_t 类型结果；调用者应检查该值。

**最小调用形状：** `SignalDDS_SetFrequency(...);`。可直接从 README_MINIMAL_EXAMPLE.c 复制正常流程；README_FULL_EXAMPLE.c 展示全部公开 API，其中取消类 API 会以 #if 0 隔离。

**注意：** 所有指针和数组都由调用者拥有；先检查返回值。异步采集、DMA 或回调还在使用 buffer 时，不能读取结果或改写该 buffer。

### `uint16_t SignalDDS_Next(signal_dds_t *dds);`

**它做什么：** 执行该模块公开的功能；具体数据流以本节参数表和本 README 前面的场景说明为准。

**什么时候调用：** 执行该模块公开的功能；具体数据流以本节参数表和本 README 前面的场景说明为准。

| 参数 | 类型 | 初学者解释 |
|---|---|---|
| `dds` | `signal_dds_t *` | `dds`（`signal_dds_t `）是该 API 的输入/输出参数；按本 README 前面的数据单位和边界条件准备。 |

**返回：** 返回 uint16_t 类型结果；调用者应检查该值。

**最小调用形状：** `SignalDDS_Next(...);`。可直接从 README_MINIMAL_EXAMPLE.c 复制正常流程；README_FULL_EXAMPLE.c 展示全部公开 API，其中取消类 API 会以 #if 0 隔离。

**注意：** 所有指针和数组都由调用者拥有；先检查返回值。异步采集、DMA 或回调还在使用 buffer 时，不能读取结果或改写该 buffer。

### `signal_result_t SignalDDS_Fill(signal_dds_t *dds, uint16_t *output, size_t count);`

**它做什么：** 执行该模块公开的功能；具体数据流以本节参数表和本 README 前面的场景说明为准。

**什么时候调用：** 执行该模块公开的功能；具体数据流以本节参数表和本 README 前面的场景说明为准。

| 参数 | 类型 | 初学者解释 |
|---|---|---|
| `dds` | `signal_dds_t *` | `dds`（`signal_dds_t `）是该 API 的输入/输出参数；按本 README 前面的数据单位和边界条件准备。 |
| `output` | `uint16_t *` | 由调用者分配的输出对象/数组。成功返回后才读取其中内容。 |
| `count` | `size_t` | 元素数量或容量，单位是“元素个数”而不是字节；必须与实际数组大小一致。 |

**返回：** 返回 signal_result_t 类型结果；调用者应检查该值。

**最小调用形状：** `SignalDDS_Fill(...);`。可直接从 README_MINIMAL_EXAMPLE.c 复制正常流程；README_FULL_EXAMPLE.c 展示全部公开 API，其中取消类 API 会以 #if 0 隔离。

**注意：** 所有指针和数组都由调用者拥有；先检查返回值。异步采集、DMA 或回调还在使用 buffer 时，不能读取结果或改写该 buffer。

### `float SignalDDS_GetConfiguredFrequency(const signal_dds_t *dds, float update_rate_hz);`

**它做什么：** 读取最近一次操作保存的状态、结果或配置；先确认前置操作已经成功。

**什么时候调用：** 读取最近一次操作保存的状态、结果或配置；先确认前置操作已经成功。

| 参数 | 类型 | 初学者解释 |
|---|---|---|
| `dds` | `const signal_dds_t *` | `dds`（`const signal_dds_t `）是该 API 的输入/输出参数；按本 README 前面的数据单位和边界条件准备。 |
| `update_rate_hz` | `float` | 频率/速率参数，单位 Hz。必须传入实际配置或测得的上游数值，不能把 ADC 时钟名称直接当采样率。 |

**返回：** 返回 float 类型结果；调用者应检查该值。

**最小调用形状：** `SignalDDS_GetConfiguredFrequency(...);`。可直接从 README_MINIMAL_EXAMPLE.c 复制正常流程；README_FULL_EXAMPLE.c 展示全部公开 API，其中取消类 API 会以 #if 0 隔离。

**注意：** 所有指针和数组都由调用者拥有；先检查返回值。异步采集、DMA 或回调还在使用 buffer 时，不能读取结果或改写该 buffer。

### `signal_module_status_t SignalDDS_GetModuleStatus();`

**它做什么：** 读取模块当前的验证成熟度或静态状态，不会启动硬件操作。

**什么时候调用：** 读取模块当前的验证成熟度或静态状态，不会启动硬件操作。

**参数：** 无。

**返回：** 返回 signal_module_status_t 类型结果；调用者应检查该值。

**最小调用形状：** `SignalDDS_GetModuleStatus(...);`。可直接从 README_MINIMAL_EXAMPLE.c 复制正常流程；README_FULL_EXAMPLE.c 展示全部公开 API，其中取消类 API 会以 #if 0 隔离。

**注意：** 所有指针和数组都由调用者拥有；先检查返回值。异步采集、DMA 或回调还在使用 buffer 时，不能读取结果或改写该 buffer。

### 示例、模块链与验收

- 最小入门：`README_MINIMAL_EXAMPLE.c`，只保留正常入口和结果读取。
- 全功能：`README_FULL_EXAMPLE.c`，以正确顺序展示当前头文件全部公开 API；`Stop` 等非常规路径不会默认执行。
- 模块链：先由上游提供单位、采样率和有效数据，再调用本模块；成功后将输出交给显示、控制、测量或下一步 DSP。硬件资源仍以 SysConfig 合约为唯一来源。
- 文档验收：README/API、两份示例和头文件会由 `tools/validate_beginner_documentation.ps1` 覆盖检查；这只表示文档与源码签名一致，不代替未进行的实板验证。

### 模块链

`上游采集/配置 -> 本模块 -> 检查返回值和结果 -> 测量、显示、控制或下一步 DSP`。只有确认本模块的输出单位和有效状态后，才交给下一模块。

