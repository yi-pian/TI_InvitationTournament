# ADC Continuous：把连续采集帧交给处理函数

## 0. 一句话说明

当上游已经拿到一帧完整 ADC 数据时，本模块负责按顺序把这一帧交给你的处理函数，并记录完成帧和丢帧数；它**不启动 ADC 或 DMA**。

## 1. 什么时候用 / 什么时候不要用

适合：连续 ADC Ping-Pong DMA 每完成一帧就送入 RMS、FFT 或显示处理；希望把“采集”和“处理一帧”分开并统计是否丢帧；或先在 PC 上用模拟帧验证处理函数。

不适合：用它代替 `adc_dma`、`adc_pingpong_dma` 配置 ADC/Timer/DMA；只采一帧后直接处理；或想在回调返回后长期保存上游 DMA 缓冲区指针。

## 2. 先看懂原理

```text
ADC/DMA 上游确认一帧完成
            |
            v
SignalADCContinuous_SubmitFrame(raw, N)
            |
            v
你的 callback(context, raw, N, sequence)
            |
            v
RMS / FFT / 显示 / 控制
```

`SubmitFrame()` 在调用它的同一上下文中同步执行 callback，不创建线程、不等待 DMA，也不复制数组。回调返回后上游可以复用该 buffer，需要长期保存的数据必须由你复制。

## 3. 参数分级

### 🟢 小白必须理解

| 参数 | 是什么 | 怎么设 | 设置错误的结果 |
|---|---|---|---|
| `samples` | 已完成帧的首地址 | 传上游已经停止写入的 `uint16_t[]` | DMA 同时改写会让处理结果不一致 |
| `count` | 本帧样本元素数，不是字节数 | 与真实 buffer 元素数一致，且非 0 | 处理函数越界或遗漏样本 |
| `callback` | 你处理一帧数据的函数 | 必须非空；保持短小 | 空指针 Init 失败；耗时太长会拖慢上游 |
| `callback_context` | 传给 callback 的私有状态 | 可为 `NULL` | 类型转换错误会破坏用户状态 |

`sequence` 从 1 开始，每次成功 SubmitFrame 加 1。`completed_frames` 是成功回调次数，`dropped_frames` 是模块未在运行状态时提交的帧数。

### 🟡 出问题时再理解

`state` 只会是 `MODULE_IDLE` 或 `MODULE_RUNNING`。它不是硬件 DMA 状态，硬件是否完成由上游模块判断后再调用 SubmitFrame。

### 🔴 高级参数

本模块没有队列和多线程锁。若 callback 的平均运行时间长于上游产生一帧的时间，应用必须选择更大的 Ping-Pong block、降低采样/刷新率，或把耗时计算放到主循环；不能指望本模块缓存无限帧。

## 4. SysConfig 教程

本模块本身没有 SysConfig 项目。已有 `adc_pingpong_dma` 时按它的 ADC12 + DMA 教程配置，并在 DMA 完成、Acquire 成功后提交完成块。已有定时单帧 `adc_dma` 时等待 `SignalADC_IsFinished()` 后再提交其 buffer。PC 或固定测试数组不需要 SysConfig。

不要因为文件名里有 ADC 就为它额外添加 ADC、Timer、Event 或 DMA，这些资源属于上游采集模块。

## 5. 函数调用顺序

```text
Init（只一次） -> Start（开始接收） -> 上游完成一帧
 -> SubmitFrame（每帧一次） -> callback -> Stop（结束时）
```

## 6. API 教程

### `SignalADCContinuous_Init(module, callback, callback_context)`

保存 callback 和上下文，清零帧计数，状态设为 IDLE。创建 `signal_adc_continuous_t` 后、提交任何帧前调用。`module` 是长期保存的状态对象，`callback` 必须非空，`callback_context` 可为 NULL。成功返回 `SIGNAL_RESULT_OK`；module 或 callback 为空返回 `SIGNAL_RESULT_INVALID_ARGUMENT`。

### `SignalADCContinuous_Start(module)`

将分发器设为 RUNNING，允许提交帧。在 Init 后、上游开始产生帧前调用。成功为 `OK`；对象/回调无效为 `NOT_INITIALIZED`；已经运行是 `BUSY`。

### `SignalADCContinuous_SubmitFrame(module, samples, count)`

立即调用 callback，并把 `completed_frames + 1` 作为 sequence 传入。只在 `samples[0..count-1]` 已完整且 DMA 不再写入时调用。`samples` 必须非空，`count` 是非零**元素数**。`OK` 表示 callback 已执行；`BUSY` 表示未 Start，本帧被放弃且 dropped_frames 增加；`INVALID_ARGUMENT` 表示指针或 count 非法。

### `SignalADCContinuous_Stop(module)`

状态置为 IDLE，不会停止 ADC/DMA。结束测量或需要拒收帧时调用。返回 `OK`；module 为 NULL 返回 `INVALID_ARGUMENT`。

### `SignalADCContinuous_GetModuleStatus()`

返回证据等级 `MODULE_STATUS_BUILD_VERIFIED`，不是 `module->state`，也不能证明接线已实板验证。

## 7. 示例与信号链

- [最小示例](README_MINIMAL_EXAMPLE.c)：一次 Init、Start 和 Submit。
- [全功能示例](README_FULL_EXAMPLE.c)：所有公开 API，包括安全的 Stop 演示。

```text
adc_pingpong_dma -> Acquire 完整块 -> SubmitFrame -> callback -> DSP/测量/显示
```

## 8. 常见错误

- SubmitFrame 返回 BUSY：先 Start 或检查是否 Stop；这一帧已丢弃。
- 回调数据偶发跳变：DMA 仍在写 buffer；在完成/Acquire 后再提交。
- completed_frames 增长而波形错乱：保存了会被 DMA 复用的指针；改为回调内复制或立即处理。
- CPU 来不及：缩短 callback 或降低上游帧率；本模块不含缓存队列。

## 9. 验证边界

本模块为 `BUILD_VERIFIED`。示例验证 API 顺序与计数；ADC 时序、DMA 连续性和引脚由选择的上游硬件模块单独验证。

## Hardware / Platform Binding

`signal_adc_frame_callback_t` 是应用层的帧处理回调，不是 DriverLib ISR。需要真实 MSPM0 采集时，先使用上游 ADC/DMA 平台层取得稳定帧，再在主循环或已确认安全的完成回调中调用 SubmitFrame。现有最小平台闭环工程为 `09_examples/platform_closure/adc_continuous_minimum`；它用于验证该软件分发器能与工程链接，不替代 ADC/DMA 的独立实板验证。

## 17. 统一 API 教程（已按当前头文件核对）

本节由当前公开头文件、实现中实际出现的状态码和正式模块注册表生成。它补充前文的场景教程；函数签名变化时必须重新运行 `tools/upgrade_formal_beginner_docs.ps1`，不要手工保留旧 API。

遵循仓库的 [Beginner README 标准](../../00_docs/BEGINNER_README_STANDARD.md)：先用最小示例完成一次正常数据流，再按需要阅读全功能示例和本节 API 细节。

### 什么时候用 / 什么时候不要用

当题目需要“adc_continuous”目录对应的公开功能，并且输入数据、单位和硬件资源满足前文约束时使用本模块。若只需要更简单的上游功能、输入尚未准备好，或需要不同的数据模型/外设资源，应先选择相邻模块而不是强行调用本 API。

### 输入 / 输出

输入由各 API 的只读数组、配置、频率/阈值和平台对象组成；输出写入 result/output/buffer 参数或由 Get API 返回。调用者负责数组容量、生命周期和物理单位；失败返回时输出不是有效结果。

### 调用顺序

```text
SignalADCContinuous_Init -> SignalADCContinuous_Start -> SignalADCContinuous_SubmitFrame -> SignalADCContinuous_GetModuleStatus -> SignalADCContinuous_Stop
```

`Init` 一般仅一次；`Set/Configure` 仅在参数变化且模块空闲时执行；`Start/Process/Generate` 是每帧或每次任务入口；`Get/Is` 用于读取已完成的结果；`Stop` 只在需要取消时调用。若本模块没有其中某类 API，以实际列出的函数为准。

### SysConfig 边界

本模块的 SysConfig 需求取决于所选后端。使用调用者提供帧的纯软件路径时不需要 SysConfig；连接真实硬件采集后端时，按本 README 前文及注册表合约配置对应资源。不要编辑生成的 `ti_msp_dl_config.c/.h`。

### 参数分级

- 【比赛必须会】输入/输出数组、`count/length/capacity`、采样率/频率、阈值/增益以及本 README 前文标出的 pin。它们直接影响题目范围、RAM、时间轴或物理单位。
- 【出问题再理解】Timer 时钟、DMA 通道、Event 路由、参考源和 IRQ。它们属于硬件链路，必须与 SysConfig 生成结果一致。
- 【以后进阶】多缓冲、运行时重配置、回调调度和 ISR 优化。先用最小示例完成一帧闭环，再处理吞吐或延迟。

### 常见错误 FAQ

- 参数错误：先检查指针非空、count/capacity 的单位是元素数、频率/阈值单位与上游一致。
- 硬件无结果：不要修改生成文件；回到 SysConfig 核对 pin、instance、时钟、Timer、DMA 和 Event 的完整链路。
- 结果异常：确认上一轮异步采集已经完成，真实 Fs/N/参考电压已传到算法，且没有在 DMA 使用期间改写 buffer。

### `signal_result_t SignalADCContinuous_Init(signal_adc_continuous_t *module, signal_adc_frame_callback_t callback, void *callback_context);`

**它做什么：** 绑定业务回调并清零完成帧、丢帧计数。

**什么时候调用：** 根据调用者提供的配置或对象完成一次初始化；通常在 `SYSCFG_DL_init()` 之后且仅调用一次。

| 参数 | 类型 | 初学者解释 |
|---|---|---|
| `module` | `signal_adc_continuous_t *` | 调用者分配的模块状态，不能为空。 |
| `callback` | `signal_adc_frame_callback_t` | 每一帧的处理函数，不能为空。 |
| `callback_context` | `void *` | 原样传回 callback 的用户指针，可为 NULL。 |

**返回：** OK 成功；INVALID_ARGUMENT 表示 module 或 callback 为空。

**最小调用形状：** `SignalADCContinuous_Init(...);`。可直接从 README_MINIMAL_EXAMPLE.c 复制正常流程；README_FULL_EXAMPLE.c 展示全部公开 API，其中取消类 API 会以 #if 0 隔离。

**注意：** 所有指针和数组都由调用者拥有；先检查返回值。异步采集、DMA 或回调还在使用 buffer 时，不能读取结果或改写该 buffer。

### `signal_result_t SignalADCContinuous_Start(signal_adc_continuous_t *module);`

**它做什么：** 允许接收帧。重复 Start 返回 BUSY；未正确 Init 返回 NOT_INITIALIZED。

**什么时候调用：** 启动一轮新的硬件操作或异步传输；成功后按对应的完成查询 API 等待结果。

| 参数 | 类型 | 初学者解释 |
|---|---|---|
| `module` | `signal_adc_continuous_t *` | `module`（`signal_adc_continuous_t *`）是该 API 的输入/输出参数；按本 README 前面的数据单位和边界条件准备。 |

**返回：** 当前实现中出现的返回/成熟度枚举值：`SIGNAL_RESULT_NOT_INITIALIZED`、`SIGNAL_RESULT_BUSY`、`SIGNAL_RESULT_OK`。

**最小调用形状：** `SignalADCContinuous_Start(...);`。可直接从 README_MINIMAL_EXAMPLE.c 复制正常流程；README_FULL_EXAMPLE.c 展示全部公开 API，其中取消类 API 会以 #if 0 隔离。

**注意：** 所有指针和数组都由调用者拥有；先检查返回值。异步采集、DMA 或回调还在使用 buffer 时，不能读取结果或改写该 buffer。

### `signal_result_t SignalADCContinuous_Stop(signal_adc_continuous_t *module);`

**它做什么：** 停止接收帧。之后提交的帧会计入 dropped_frames。

**什么时候调用：** 主动终止当前操作并释放模块占用的运行状态；只在需要取消本轮任务时调用。

| 参数 | 类型 | 初学者解释 |
|---|---|---|
| `module` | `signal_adc_continuous_t *` | `module`（`signal_adc_continuous_t *`）是该 API 的输入/输出参数；按本 README 前面的数据单位和边界条件准备。 |

**返回：** 当前实现中出现的返回/成熟度枚举值：`SIGNAL_RESULT_INVALID_ARGUMENT`、`SIGNAL_RESULT_OK`。

**最小调用形状：** `SignalADCContinuous_Stop(...);`。可直接从 README_MINIMAL_EXAMPLE.c 复制正常流程；README_FULL_EXAMPLE.c 展示全部公开 API，其中取消类 API 会以 #if 0 隔离。

**注意：** 所有指针和数组都由调用者拥有；先检查返回值。异步采集、DMA 或回调还在使用 buffer 时，不能读取结果或改写该 buffer。

### `signal_result_t SignalADCContinuous_SubmitFrame(signal_adc_continuous_t *module, const uint16_t *samples, size_t count);`

**它做什么：** 把上游已完成的一帧数据交给 callback。

**什么时候调用：** 把一帧由调用者持有的数据提交给模块；提交后按 README 的缓冲区生命周期规则处理。

| 参数 | 类型 | 初学者解释 |
|---|---|---|
| `module` | `signal_adc_continuous_t *` | 已 Init 且已 Start 的模块状态。 |
| `samples` | `const uint16_t *` | 上游拥有的只读样本数组，不能为空。 |
| `count` | `size_t` | 样本元素数，必须非零。 |

**返回：** OK 已调用 callback；BUSY 表示未 Start 且本帧已丢弃；

**最小调用形状：** `SignalADCContinuous_SubmitFrame(...);`。可直接从 README_MINIMAL_EXAMPLE.c 复制正常流程；README_FULL_EXAMPLE.c 展示全部公开 API，其中取消类 API 会以 #if 0 隔离。

**注意：** 所有指针和数组都由调用者拥有；先检查返回值。异步采集、DMA 或回调还在使用 buffer 时，不能读取结果或改写该 buffer。

### `signal_module_status_t SignalADCContinuous_GetModuleStatus();`

**它做什么：** 返回本模块当前构建证据等级，不是运行时状态。

**什么时候调用：** 读取模块当前的验证成熟度或静态状态，不会启动硬件操作。

**参数：** 无。

**返回：** 当前实现中出现的返回/成熟度枚举值：`MODULE_STATUS_BUILD_VERIFIED`。

**最小调用形状：** `SignalADCContinuous_GetModuleStatus(...);`。可直接从 README_MINIMAL_EXAMPLE.c 复制正常流程；README_FULL_EXAMPLE.c 展示全部公开 API，其中取消类 API 会以 #if 0 隔离。

**注意：** 所有指针和数组都由调用者拥有；先检查返回值。异步采集、DMA 或回调还在使用 buffer 时，不能读取结果或改写该 buffer。

### 示例、模块链与验收

- 最小入门：`README_MINIMAL_EXAMPLE.c`，只保留正常入口和结果读取。
- 全功能：`README_FULL_EXAMPLE.c`，以正确顺序展示当前头文件全部公开 API；`Stop` 等非常规路径不会默认执行。
- 模块链：先由上游提供单位、采样率和有效数据，再调用本模块；成功后将输出交给显示、控制、测量或下一步 DSP。硬件资源仍以 SysConfig 合约为唯一来源。
- 文档验收：README/API、两份示例和头文件会由 `tools/validate_beginner_documentation.ps1` 覆盖检查；这只表示文档与源码签名一致，不代替未进行的实板验证。

### 模块链

`上游采集/配置 -> 本模块 -> 检查返回值和结果 -> 测量、显示、控制或下一步 DSP`。只有确认本模块的输出单位和有效状态后，才交给下一模块。

