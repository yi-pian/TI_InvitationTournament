# CZT：指定窄频带的 Unit-Circle 直接计算

这是 clean reimplementation 的正确性基线，不是旧 `SignalCZT_Execute` 恢复，也不是已优化 Bluestein backend。

## 它做什么

普通 FFT 的频点固定为 `k·Fs/N`。本模块可以指定起始频率、频率步长和 M 个输出点，直接计算这些点的复频谱：

```text
float samples[N] → CZT(start, step, M) → complex spectrum[M]
```

## 加入和调用

链接 `signal_czt.c/.h`；Include Path 加本目录和 `03_measurement/common`；无需 SysConfig。

```c
signal_complex_f32_t zoom[41];
SignalCZT_UnitCircleRealDirect(
    samples, N, FS_HZ, 900.0f, 5.0f, zoom, 41U, 41U);
```

输出未除以 N，DFT 符号与正式 FFT 一致。最常改的是 `start_frequency_hz`、`frequency_step_hz`、`output_count`。

当前复杂度 O(NM)：N=1024、M=200 会非常慢，只适合小 M 窄带确认。大范围频谱用 FFT；只修主频用 Peak Interpolation；不要把“频点更密”误解为增加了原始记录的信息量。首末频点必须在 ±Fs/2，容量不足会返回错误。详细数学和边界见 [REIMPLEMENTATION_SPEC](REIMPLEMENTATION_SPEC.md)。

## 17. 统一 API 教程（已按当前头文件核对）

本节由当前公开头文件、实现中实际出现的状态码和正式模块注册表生成。它补充前文的场景教程；函数签名变化时必须重新运行 `tools/upgrade_formal_beginner_docs.ps1`，不要手工保留旧 API。

遵循仓库的 [Beginner README 标准](../../00_docs/BEGINNER_README_STANDARD.md)：先用最小示例完成一次正常数据流，再按需要阅读全功能示例和本节 API 细节。

### 什么时候用 / 什么时候不要用

当题目需要“czt”目录对应的公开功能，并且输入数据、单位和硬件资源满足前文约束时使用本模块。若只需要更简单的上游功能、输入尚未准备好，或需要不同的数据模型/外设资源，应先选择相邻模块而不是强行调用本 API。

### 输入 / 输出

输入由各 API 的只读数组、配置、频率/阈值和平台对象组成；输出写入 result/output/buffer 参数或由 Get API 返回。调用者负责数组容量、生命周期和物理单位；失败返回时输出不是有效结果。

### 调用顺序

```text
SignalCZT_UnitCircleRealDirect
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

### `signal_algorithm_status_t SignalCZT_UnitCircleRealDirect(const float *samples, uint32_t sample_count, float sample_rate_hz, float start_frequency_hz, float frequency_step_hz, signal_complex_f32_t *output, uint32_t output_count, uint32_t output_capacity);`

**它做什么：** 执行该模块公开的功能；具体数据流以本节参数表和本 README 前面的场景说明为准。

**什么时候调用：** 执行该模块公开的功能；具体数据流以本节参数表和本 README 前面的场景说明为准。

| 参数 | 类型 | 初学者解释 |
|---|---|---|
| `samples` | `const float *` | 调用者持有的数据或缓冲区。容量、单位和读写方向以函数名及本 README 的输入输出说明为准；异步硬件操作完成前不得改写。 |
| `sample_count` | `uint32_t` | 元素数量或容量，单位是“元素个数”而不是字节；必须与实际数组大小一致。 |
| `sample_rate_hz` | `float` | 频率/速率参数，单位 Hz。必须传入实际配置或测得的上游数值，不能把 ADC 时钟名称直接当采样率。 |
| `start_frequency_hz` | `float` | 频率/速率参数，单位 Hz。必须传入实际配置或测得的上游数值，不能把 ADC 时钟名称直接当采样率。 |
| `frequency_step_hz` | `float` | `frequency_step_hz`（`float`）是该 API 的输入/输出参数；按本 README 前面的数据单位和边界条件准备。 |
| `output` | `signal_complex_f32_t *` | 由调用者分配的输出对象/数组。成功返回后才读取其中内容。 |
| `output_count` | `uint32_t` | 由调用者分配的输出对象/数组。成功返回后才读取其中内容。 |
| `output_capacity` | `uint32_t` | 由调用者分配的输出对象/数组。成功返回后才读取其中内容。 |

**返回：** 返回 signal_algorithm_status_t 类型结果；调用者应检查该值。

**最小调用形状：** `SignalCZT_UnitCircleRealDirect(...);`。可直接从 README_MINIMAL_EXAMPLE.c 复制正常流程；README_FULL_EXAMPLE.c 展示全部公开 API，其中取消类 API 会以 #if 0 隔离。

**注意：** 所有指针和数组都由调用者拥有；先检查返回值。异步采集、DMA 或回调还在使用 buffer 时，不能读取结果或改写该 buffer。

### 示例、模块链与验收

- 最小入门：`README_MINIMAL_EXAMPLE.c`，只保留正常入口和结果读取。
- 全功能：`README_FULL_EXAMPLE.c`，以正确顺序展示当前头文件全部公开 API；`Stop` 等非常规路径不会默认执行。
- 模块链：先由上游提供单位、采样率和有效数据，再调用本模块；成功后将输出交给显示、控制、测量或下一步 DSP。硬件资源仍以 SysConfig 合约为唯一来源。
- 文档验收：README/API、两份示例和头文件会由 `tools/validate_beginner_documentation.ps1` 覆盖检查；这只表示文档与源码签名一致，不代替未进行的实板验证。

### 模块链

`上游采集/配置 -> 本模块 -> 检查返回值和结果 -> 测量、显示、控制或下一步 DSP`。只有确认本模块的输出单位和有效状态后，才交给下一模块。

