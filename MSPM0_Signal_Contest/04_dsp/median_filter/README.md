# MedianFilter：用局部中间值去孤立毛刺

> **LEVEL C / REAL ALGORITHM MODULE：** 滑动窗口排序、两端边界和输入输出重叠规则会影响真实瞬态，保留为正式模块。

**比赛复制清单：** `signal_median_filter.c`、`signal_median_filter.h`、`03_measurement/common/signal_algorithm_status.h`。调用者提供至少 `window_size` 个 float workspace，输入输出不能重叠；无 SysConfig/Pin。

## 1 这个算法是干什么的？

它不求平均，而把窗口排序后选中间那个数。一个特别大的孤立毛刺通常排到最边上，不会成为中位数。

## 2 一个最简单的例子

窗口 3：`1,100,1` 排序后为 `1,1,100`，中位数是 1，所以中间毛刺 100 被替换为 1。

## 3 原理

每点取左右各 `W/2` 点，复制到 workspace，排序并取中间值。边缘窗口自动缩短；偶数个边缘样本取中间两数平均。

## 4 比赛里什么时候用？

确认存在少量 ADC 异常跳码，且尖峰不是题目目标时。可作为 RobustVPP 前的可解释积木。

## 5 输入

输入 float 数组、输出数组、`count`、奇数 `window_size`、至少 W 个 float 的 workspace。单位保持不变。

## 6 输出

滤波后的 float 数组。输入输出不能是同一数组，因为中心窗口还要读取未处理的邻点。

## 7 API怎么调用

```c
float workspace[5];
SignalMedianFilter_Process(in_v, out_v, count, 5U, workspace, 5U);
```

## 8 参数怎么改

窗口必须为奇数。先用 3；若毛刺连续 2~3 点才考虑 5/7，并用真实脉冲测试确认没有删掉有效事件。

## 9 参数改大会怎样

3→9：能压制更宽的异常簇，但真实窄峰/边沿被改得更多，CPU 约按 W² 增长，时间分辨率下降。

## 10 这个算法的代价是什么

Benefits：孤立极端值不会拖动结果；不需要噪声方差模型。

Trade-offs：非线性；频谱不再能用普通线性滤波器传递函数解释；需要输出和 workspace；较慢。

## 11 什么时候不要用

脉冲、方波边沿、开关瞬态、故障尖峰本身是目标时绝对不要先滤掉。FFT/THD 前也不应默认加入。

## 12 怎么和前一个模块接

```text
ADC_ToVoltage -> MedianFilter
```

## 13 怎么和后一个模块接

```text
┌── MedianFilter ──┐
│ local sort       │
│ middle value     │
└───────┬──────────┘
        ├──> Vpp / RMS
        └──> ZeroCross（确认边沿未被扭曲）
```

## 14 最小Demo

```c
float in[] = {1,1,100,1,1}, out[5], work[3];
(void)SignalMedianFilter_Process(in, out, 5U, 3U, work, 3U);
```

## 15 PC测试

上例五个输出 Expected 全为 1，Measured 全一致；偶数窗口正确拒绝。PASS。

排查：脉冲消失先判断它是不是有效信号；边缘值与中间不同要看边界缩短规则；BUFFER_TOO_SMALL 检查 workspace 是元素数而非字节数。

## 16 MCU资源

当前实现 O(N·W²) 最坏时间，内部常数 RAM，调用者 workspace `4W`，输出 `4N`。适合 W=3/5/7 小窗口，不适合在 M0+ 上无脑 W=101。

## 17 验证状态

PC_VERIFIED：2026-08-07，严格编译、毛刺与参数测试通过；未 BOARD_VERIFIED。

## 17. 统一 API 教程（已按当前头文件核对）

本节由当前公开头文件、实现中实际出现的状态码和正式模块注册表生成。它补充前文的场景教程；函数签名变化时必须重新运行 `tools/upgrade_formal_beginner_docs.ps1`，不要手工保留旧 API。

遵循仓库的 [Beginner README 标准](../../00_docs/BEGINNER_README_STANDARD.md)：先用最小示例完成一次正常数据流，再按需要阅读全功能示例和本节 API 细节。

### 什么时候用 / 什么时候不要用

当题目需要“median_filter”目录对应的公开功能，并且输入数据、单位和硬件资源满足前文约束时使用本模块。若只需要更简单的上游功能、输入尚未准备好，或需要不同的数据模型/外设资源，应先选择相邻模块而不是强行调用本 API。

### 输入 / 输出

输入由各 API 的只读数组、配置、频率/阈值和平台对象组成；输出写入 result/output/buffer 参数或由 Get API 返回。调用者负责数组容量、生命周期和物理单位；失败返回时输出不是有效结果。

### 调用顺序

```text
SignalMedianFilter_Process
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

### `signal_algorithm_status_t SignalMedianFilter_Process(const float *input_samples, float *output_samples, uint32_t count, uint32_t window_size, float *workspace, uint32_t workspace_count);`

**它做什么：** 用每点附近窗口的中位数替换该点，抑制孤立尖峰。

**什么时候调用：** 对调用者提供的数据执行一次同步计算或生成，并在成功后写入输出对象/数组。

| 参数 | 类型 | 初学者解释 |
|---|---|---|
| `input_samples` | `const float *` | 输入数组，只读；不得与 output_samples 重叠。 |
| `output_samples` | `float *` | 输出数组，单位与输入相同，容量至少 count。 |
| `count` | `uint32_t` | 样本点数，必须大于 0。 |
| `window_size` | `uint32_t` | 奇数窗口长度，范围 1~count；边缘自动缩短窗口。 |
| `workspace` | `float *` | 调用者提供的临时 float 数组，会被排序覆盖。 |
| `workspace_count` | `uint32_t` | 临时数组容量，至少为 window_size。 |

**返回：** 成功返回 SIGNAL_ALGORITHM_OK；参数、容量或数值非法返回错误码。

**最小调用形状：** `SignalMedianFilter_Process(...);`。可直接从 README_MINIMAL_EXAMPLE.c 复制正常流程；README_FULL_EXAMPLE.c 展示全部公开 API，其中取消类 API 会以 #if 0 隔离。

**注意：** 所有指针和数组都由调用者拥有；先检查返回值。异步采集、DMA 或回调还在使用 buffer 时，不能读取结果或改写该 buffer。

### 示例、模块链与验收

- 最小入门：`README_MINIMAL_EXAMPLE.c`，只保留正常入口和结果读取。
- 全功能：`README_FULL_EXAMPLE.c`，以正确顺序展示当前头文件全部公开 API；`Stop` 等非常规路径不会默认执行。
- 模块链：先由上游提供单位、采样率和有效数据，再调用本模块；成功后将输出交给显示、控制、测量或下一步 DSP。硬件资源仍以 SysConfig 合约为唯一来源。
- 文档验收：README/API、两份示例和头文件会由 `tools/validate_beginner_documentation.ps1` 覆盖检查；这只表示文档与源码签名一致，不代替未进行的实板验证。

### 模块链

`上游采集/配置 -> 本模块 -> 检查返回值和结果 -> 测量、显示、控制或下一步 DSP`。只有确认本模块的输出单位和有效状态后，才交给下一模块。

