# ADC Ping-Pong DMA：连续采集时交替使用两块缓冲区

## 0. 一句话说明

当 DMA 正在写下一帧时，用另一块已经写完的数组给 CPU 处理，避免“处理数据时 ADC 停采”。

## 1. 什么时候用 / 什么时候不要用

适合：连续显示波形、连续 FFT、持续 RMS；DMA 一帧完成后 CPU 仍要处理较长时间；或需要发现 CPU 未处理完而 DMA 已绕回来的 overrun。

不适合：只采一次 N 点（用 `adc_dma` 更简单）；想把本纯软件状态模块直接当 ADC + DMA 驱动；或无法给两块 buffer 各自完整一帧的 RAM。

## 2. 先看懂原理

```text
DMA 正在写 A                CPU 处理 B
DMA 完成 A
    |  A 标记 ready
    v
DMA 改写 B                  CPU Acquire(A) -> 使用 A -> Release(A)
```

两块数组都由应用提供。`OnDmaComplete()` 只更新状态并告诉驱动下一块地址；真正把 `next_destination` 装入 DMA 寄存器是硬件适配层职责。

## 3. 参数分级

### 🟢 小白必须理解

| 参数 | 含义 | 建议 | 影响 |
|---|---|---|---|
| `buffer_a`、`buffer_b` | 两块不同的 `uint16_t[]` | 静态分配、等长 | RAM 为 `2 * N * 2` 字节 |
| `sample_count` | 每块的元素数 N | 与一次 DMA block 完全一致 | N 大：处理时间/频率分辨率更好，但延迟/RAM 更大 |
| `id` | Acquire 得到的块编号 | 原样传给 Release | 传错可能提前归还仍在使用的块 |
| `next_destination` | 下一次 DMA 的写地址 | 立即交给 DMA 驱动 | 忽略会重复写同一块 |

例：`N=1024` 时两块 raw 缓冲区占 `2 * 1024 * sizeof(uint16_t) = 4096` 字节。初学者通常只改 N，不要手改 `ready`、`dma_target`。

### 🟡 出问题时再理解

`completed_blocks` 为 DMA 完成次数；DMA 再完成一块尚未 Release 的块时 `overrun_blocks` 增加。OnDmaComplete 返回 `BUSY` 还表示下一目标仍 ready，继续写会覆盖未处理数据。

### 🔴 高级参数

状态机假定单生产者（DMA ISR）和单消费者（主循环）。RTOS 多任务消费时，应用必须在 Acquire/Release 外自行同步，不能同时由多个任务取同一块。

## 4. SysConfig 教程

本状态模块不含 DriverLib 配置，实际采集链按当前工程设置：

1. CCS `.syscfg` 添加 **ADC12**，选择实际模拟输入 channel/pin、参考源、分辨率和 sample time。
2. 给 ADC result 配置空闲 **DMA channel**，source 为 ADC result，destination width 为 half word，destination 为 block increment。
3. 需要准确 Fs 时再加 **Timer + Event**，让 Timer event 触发 ADC；自由运行 ADC 的 Fs 则由 ADC 配置决定。
4. DMA 完成 ISR 调用 `SignalADCPingPong_OnDmaComplete()`，把返回的 `next_destination` 写入下一次 DMA 配置，立刻退出 ISR。

不要照抄其它工程的 ADC 引脚、DMA channel 或 Timer 实例。保存 `.syscfg` 后重新 Generate，让硬件适配层使用本工程生成的宏。

## 5. 函数调用顺序

```text
Init -> DMA 首次目标 buffer A -> DMA 完成 ISR: OnDmaComplete
 -> 主循环 Acquire -> 处理 samples[0..count-1] -> Release -> 循环
```

## 6. API 教程

### `SignalADCPingPong_Init(module, buffer_a, buffer_b, sample_count)`

绑定两块不同的非空数组，清零 ready、完成和 overrun 计数，首次 DMA 目标为 A。必须在开启 DMA 前调用。N 是每块 `uint16_t` 元素数，须非零。指针为空、两块相同或 N=0 返回 `INVALID_ARGUMENT`；成功为 `OK`。

### `SignalADCPingPong_OnDmaComplete(module, next_destination)`

只在 DMA 完成 ISR（或已确认完成的位置）调用。它把刚完成块标 ready、切换目标，令 `*next_destination` 指向另一块。next_destination 不得为 NULL。成功为 `OK`；若下一块仍 ready 返回 `BUSY`，表示 CPU 已跟不上，不能只靠等待解决。

### `SignalADCPingPong_Acquire(module, id, samples, count)`

在主循环取得一块完整数据。成功时 id 为块编号，samples 为只读首地址，count 等于 Init 的 N。没有完整块返回 `NO_DATA`，空参数返回 `INVALID_ARGUMENT`。处理期间不得 Release 该 id，也不要把 samples 指针长期交给异步任务。

### `SignalADCPingPong_Release(module, id)`

处理完后用 Acquire 返回的**同一个** id 调用，使该块可被 DMA 再次覆盖。成功为 `OK`；非法 id 或 module 为空为 `INVALID_ARGUMENT`。

### `SignalADCPingPong_GetModuleStatus()`

返回 `MODULE_STATUS_BUILD_VERIFIED` 证据等级，不是 DMA 实时状态。

## 7. 示例与模块链

- [最小示例](README_MINIMAL_EXAMPLE.c)：模拟 DMA 完成、获取并归还 A 块。
- [全功能示例](README_FULL_EXAMPLE.c)：五个公开 API 的正确调用位置。

```text
ADC + DMA -> Ping-Pong 完成块 -> ADC 转电压 / 去直流 / 窗函数 / FFT
```

## 8. 常见错误

- A、B 是同一数组：Init 返回 INVALID_ARGUMENT；必须是两块内存。
- DMA 连续写同一块：没有将 next_destination 写回硬件配置。
- BUSY 或 overrun_blocks 增长：主循环未及时 Acquire/Release；降低帧率、增大 N 或简化处理。
- 数据处理中变化：提前 Release，或驱动没有尊重当前目标块。
- RAM 不够：N 减半可使双缓冲 RAM 减半，但 CPU 唤醒/处理更频繁。

## 9. 验证边界

模块为 `BUILD_VERIFIED`。示例验证软件状态转换；ADC 时钟、DMA 重装、连续采样和物理引脚仍须在当前 SysConfig/目标板验证。

## 17. 统一 API 教程（已按当前头文件核对）

本节由当前公开头文件、实现中实际出现的状态码和正式模块注册表生成。它补充前文的场景教程；函数签名变化时必须重新运行 `tools/upgrade_formal_beginner_docs.ps1`，不要手工保留旧 API。

遵循仓库的 [Beginner README 标准](../../00_docs/BEGINNER_README_STANDARD.md)：先用最小示例完成一次正常数据流，再按需要阅读全功能示例和本节 API 细节。

### 什么时候用 / 什么时候不要用

当题目需要“adc_pingpong_dma”目录对应的公开功能，并且输入数据、单位和硬件资源满足前文约束时使用本模块。若只需要更简单的上游功能、输入尚未准备好，或需要不同的数据模型/外设资源，应先选择相邻模块而不是强行调用本 API。

### 输入 / 输出

输入由各 API 的只读数组、配置、频率/阈值和平台对象组成；输出写入 result/output/buffer 参数或由 Get API 返回。调用者负责数组容量、生命周期和物理单位；失败返回时输出不是有效结果。

### 调用顺序

```text
SignalADCPingPong_Init -> SignalADCPingPong_GetModuleStatus -> SignalADCPingPong_OnDmaComplete -> SignalADCPingPong_Acquire -> SignalADCPingPong_Release
```

`Init` 一般仅一次；`Set/Configure` 仅在参数变化且模块空闲时执行；`Start/Process/Generate` 是每帧或每次任务入口；`Get/Is` 用于读取已完成的结果；`Stop` 只在需要取消时调用。若本模块没有其中某类 API，以实际列出的函数为准。

### SysConfig 边界

本模块需要 SysConfig。先在 CCS 的 .syscfg 添加并核对：ADC12、DMA；再按前文的模块专用 GUI 步骤选择实际 pin/instance。保存后让 CCS 重新生成配置，核对生成宏；不要直接修改 	i_msp_dl_config.c/.h，也不要照抄示例 pin 或 DMA/Event 编号。

### 参数分级

- 【比赛必须会】输入/输出数组、`count/length/capacity`、采样率/频率、阈值/增益以及本 README 前文标出的 pin。它们直接影响题目范围、RAM、时间轴或物理单位。
- 【出问题再理解】Timer 时钟、DMA 通道、Event 路由、参考源和 IRQ。它们属于硬件链路，必须与 SysConfig 生成结果一致。
- 【以后进阶】多缓冲、运行时重配置、回调调度和 ISR 优化。先用最小示例完成一帧闭环，再处理吞吐或延迟。

### 常见错误 FAQ

- 参数错误：先检查指针非空、count/capacity 的单位是元素数、频率/阈值单位与上游一致。
- 硬件无结果：不要修改生成文件；回到 SysConfig 核对 pin、instance、时钟、Timer、DMA 和 Event 的完整链路。
- 结果异常：确认上一轮异步采集已经完成，真实 Fs/N/参考电压已传到算法，且没有在 DMA 使用期间改写 buffer。

### `signal_result_t SignalADCPingPong_Init(signal_adc_pingpong_dma_t *module, uint16_t *buffer_a, uint16_t *buffer_b, size_t sample_count);`

**它做什么：** 绑定两块不同的等长缓冲区，并把 DMA 下一目标设为 A。

**什么时候调用：** 根据调用者提供的配置或对象完成一次初始化；通常在 `SYSCFG_DL_init()` 之后且仅调用一次。

| 参数 | 类型 | 初学者解释 |
|---|---|---|
| `module` | `signal_adc_pingpong_dma_t *` | `module`（`signal_adc_pingpong_dma_t *`）是该 API 的输入/输出参数；按本 README 前面的数据单位和边界条件准备。 |
| `buffer_a` | `uint16_t *` | 调用者持有的数据或缓冲区。容量、单位和读写方向以函数名及本 README 的输入输出说明为准；异步硬件操作完成前不得改写。 |
| `buffer_b` | `uint16_t *` | 调用者持有的数据或缓冲区。容量、单位和读写方向以函数名及本 README 的输入输出说明为准；异步硬件操作完成前不得改写。 |
| `sample_count` | `size_t` | 每块的 uint16_t 元素数，必须非零。 |

**返回：** 当前实现中出现的返回/成熟度枚举值：`SIGNAL_RESULT_INVALID_ARGUMENT`、`SIGNAL_RESULT_OK`。

**最小调用形状：** `SignalADCPingPong_Init(...);`。可直接从 README_MINIMAL_EXAMPLE.c 复制正常流程；README_FULL_EXAMPLE.c 展示全部公开 API，其中取消类 API 会以 #if 0 隔离。

**注意：** 所有指针和数组都由调用者拥有；先检查返回值。异步采集、DMA 或回调还在使用 buffer 时，不能读取结果或改写该 buffer。

### `signal_result_t SignalADCPingPong_OnDmaComplete(signal_adc_pingpong_dma_t *module, uint16_t **next_destination);`

**它做什么：** 在一块 DMA 完成时标记该块就绪并给出下一块目标地址。

**什么时候调用：** 执行该模块公开的功能；具体数据流以本节参数表和本 README 前面的场景说明为准。

| 参数 | 类型 | 初学者解释 |
|---|---|---|
| `module` | `signal_adc_pingpong_dma_t *` | `module`（`signal_adc_pingpong_dma_t *`）是该 API 的输入/输出参数；按本 README 前面的数据单位和边界条件准备。 |
| `next_destination` | `uint16_t **` | 成功时返回应立即装入 DMA 的另一块缓冲区地址。 |

**返回：** BUSY 表示下一块尚未被应用 Release，DMA 若继续写会覆盖未处理数据。

**最小调用形状：** `SignalADCPingPong_OnDmaComplete(...);`。可直接从 README_MINIMAL_EXAMPLE.c 复制正常流程；README_FULL_EXAMPLE.c 展示全部公开 API，其中取消类 API 会以 #if 0 隔离。

**注意：** 所有指针和数组都由调用者拥有；先检查返回值。异步采集、DMA 或回调还在使用 buffer 时，不能读取结果或改写该 buffer。

### `signal_result_t SignalADCPingPong_Acquire(signal_adc_pingpong_dma_t *module, signal_pingpong_buffer_id_t *id, const uint16_t **samples, size_t *count);`

**它做什么：** 取得一块 DMA 已写完、可由主循环只读处理的缓冲区。

**什么时候调用：** 执行该模块公开的功能；具体数据流以本节参数表和本 README 前面的场景说明为准。

| 参数 | 类型 | 初学者解释 |
|---|---|---|
| `module` | `signal_adc_pingpong_dma_t *` | `module`（`signal_adc_pingpong_dma_t *`）是该 API 的输入/输出参数；按本 README 前面的数据单位和边界条件准备。 |
| `id` | `signal_pingpong_buffer_id_t *` | `id`（`signal_pingpong_buffer_id_t *`）是该 API 的输入/输出参数；按本 README 前面的数据单位和边界条件准备。 |
| `samples` | `const uint16_t **` | 调用者持有的数据或缓冲区。容量、单位和读写方向以函数名及本 README 的输入输出说明为准；异步硬件操作完成前不得改写。 |
| `count` | `size_t *` | 元素数量或容量，单位是“元素个数”而不是字节；必须与实际数组大小一致。 |

**返回：** NO_DATA 表示当前没有完整块；成功后必须以同一 id 调用 Release。

**最小调用形状：** `SignalADCPingPong_Acquire(...);`。可直接从 README_MINIMAL_EXAMPLE.c 复制正常流程；README_FULL_EXAMPLE.c 展示全部公开 API，其中取消类 API 会以 #if 0 隔离。

**注意：** 所有指针和数组都由调用者拥有；先检查返回值。异步采集、DMA 或回调还在使用 buffer 时，不能读取结果或改写该 buffer。

### `signal_result_t SignalADCPingPong_Release(signal_adc_pingpong_dma_t *module, signal_pingpong_buffer_id_t id);`

**它做什么：** 声明一块已处理完，可再次交给 DMA 覆盖。

**什么时候调用：** 执行该模块公开的功能；具体数据流以本节参数表和本 README 前面的场景说明为准。

| 参数 | 类型 | 初学者解释 |
|---|---|---|
| `module` | `signal_adc_pingpong_dma_t *` | `module`（`signal_adc_pingpong_dma_t *`）是该 API 的输入/输出参数；按本 README 前面的数据单位和边界条件准备。 |
| `id` | `signal_pingpong_buffer_id_t` | `id`（`signal_pingpong_buffer_id_t`）是该 API 的输入/输出参数；按本 README 前面的数据单位和边界条件准备。 |

**返回：** 当前实现中出现的返回/成熟度枚举值：`SIGNAL_RESULT_INVALID_ARGUMENT`、`SIGNAL_RESULT_OK`。

**最小调用形状：** `SignalADCPingPong_Release(...);`。可直接从 README_MINIMAL_EXAMPLE.c 复制正常流程；README_FULL_EXAMPLE.c 展示全部公开 API，其中取消类 API 会以 #if 0 隔离。

**注意：** 所有指针和数组都由调用者拥有；先检查返回值。异步采集、DMA 或回调还在使用 buffer 时，不能读取结果或改写该 buffer。

### `signal_module_status_t SignalADCPingPong_GetModuleStatus();`

**它做什么：** 返回构建验证证据等级，不是 DMA 实时状态。

**什么时候调用：** 读取模块当前的验证成熟度或静态状态，不会启动硬件操作。

**参数：** 无。

**返回：** 当前实现中出现的返回/成熟度枚举值：`MODULE_STATUS_BUILD_VERIFIED`。

**最小调用形状：** `SignalADCPingPong_GetModuleStatus(...);`。可直接从 README_MINIMAL_EXAMPLE.c 复制正常流程；README_FULL_EXAMPLE.c 展示全部公开 API，其中取消类 API 会以 #if 0 隔离。

**注意：** 所有指针和数组都由调用者拥有；先检查返回值。异步采集、DMA 或回调还在使用 buffer 时，不能读取结果或改写该 buffer。

### 示例、模块链与验收

- 最小入门：`README_MINIMAL_EXAMPLE.c`，只保留正常入口和结果读取。
- 全功能：`README_FULL_EXAMPLE.c`，以正确顺序展示当前头文件全部公开 API；`Stop` 等非常规路径不会默认执行。
- 模块链：先由上游提供单位、采样率和有效数据，再调用本模块；成功后将输出交给显示、控制、测量或下一步 DSP。硬件资源仍以 SysConfig 合约为唯一来源。
- 文档验收：README/API、两份示例和头文件会由 `tools/validate_beginner_documentation.ps1` 覆盖检查；这只表示文档与源码签名一致，不代替未进行的实板验证。

### 模块链

`上游采集/配置 -> 本模块 -> 检查返回值和结果 -> 测量、显示、控制或下一步 DSP`。只有确认本模块的输出单位和有效状态后，才交给下一模块。

