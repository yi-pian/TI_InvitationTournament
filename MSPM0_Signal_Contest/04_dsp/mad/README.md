# MAD：对离群点更稳的波动尺度

> **LEVEL C / REAL ALGORITHM MODULE：** MAD 需要排序 workspace、中位数的奇偶长度规则和非破坏输入语义，并被 Hampel/Robust 算法复用。

**比赛复制清单：** `signal_mad.c`、`signal_mad.h`、`03_measurement/common/signal_algorithm_status.h`。调用者提供至少 N 个 float workspace；无 SysConfig/Pin。

## 1 这个算法是干什么的？

MAD 先找数据中位数，再看每个点离中位数多远，最后对这些距离再取中位数。少量特别大的毛刺通常不会控制它。

## 2 一个最简单的例子

`1,1,2,2,100` 的中位数是 2；绝对偏差为 `1,1,0,0,98`，其中位数是 1，所以 MAD=1，而 100 没把结果拉得很大。

## 3 原理

`MAD=median(|x-median(x)|)`。对近似高斯噪声，`1.4826*MAD` 可估计通常意义的 sigma。1.4826 不是万能常数，只是高斯分布下的尺度换算。

## 4 比赛里什么时候用？

估计带少量毛刺的数据尺度、Hampel 阈值、比较异常值处理前后。标准差会被极端点平方放大时 MAD 很有价值。

## 5 输入

只读 float 数组、`count>0`、容量至少 count 的 float workspace；单位可为 V。

## 6 输出

`median_value`、`mad_value`、`robust_sigma_estimate`，均与输入相同单位。workspace 内容不保留。

## 7 API怎么调用

```c
float work[N];
signal_mad_result_t r;
SignalMAD_Process(samples, N, work, N, &r);
```

## 8 参数怎么改

没有阈值参数。主要选择数据段和 count；Hampel 会在每个局部窗口调用它。

## 9 参数改大会怎样

count 增大通常让稳定分布估计更稳，但 RAM/时间增加，并可能混合不同工作状态。不是越大永远越好。

## 10 这个算法的代价是什么

Benefits：少量离群点影响小；物理单位清楚；不用预先知道均值。

Trade-offs：需要 `4N` workspace；选择算法平均快但最坏 O(N²)；只给尺度不保留时间位置。

## 11 什么时候不要用

需要频率结构、噪声谱密度、瞬态位置或分布随时间变化时。MAD=0 不等于绝对无噪声，可能只是量化后多数点相同。

## 12 怎么和前一个模块接

```text
ADC_ToVoltage / 任意 float samples -> MAD
```

## 13 怎么和后一个模块接

```text
┌──── MAD ────┐
│ median      │
│ |x-median| │
│ median again│
└──────┬──────┘
       ├──> robust noise report
       └──> Hampel threshold
```

## 14 最小Demo

```c
float x[] = {1,1,2,2,100}, work[5];
signal_mad_result_t r;
(void)SignalMAD_Process(x, 5U, work, 5U, &r);
/* median=2, MAD=1 */
```

## 15 PC测试

上例 Expected median=2、MAD=1、sigma=1.4826，Measured 完全一致，PASS。

排查：BUFFER_TOO_SMALL 检查 workspace 元素数；sigma 与标准差差异大时先看是否有毛刺、非高斯或信号本身在变化。

## 16 MCU资源

workspace `4N` 字节；Quickselect 平均 O(N)，最坏 O(N²)；两次选择加一次绝对偏差遍历。N 很大时注意 MSPM0 32 KB RAM。

## 17 验证状态

PC_VERIFIED：2026-08-07，严格编译与已知鲁棒统计测试通过；未 BOARD_VERIFIED。

## 17. 统一 API 教程（已按当前头文件核对）

本节由当前公开头文件、实现中实际出现的状态码和正式模块注册表生成。它补充前文的场景教程；函数签名变化时必须重新运行 `tools/upgrade_formal_beginner_docs.ps1`，不要手工保留旧 API。

遵循仓库的 [Beginner README 标准](../../00_docs/BEGINNER_README_STANDARD.md)：先用最小示例完成一次正常数据流，再按需要阅读全功能示例和本节 API 细节。

### 什么时候用 / 什么时候不要用

当题目需要“mad”目录对应的公开功能，并且输入数据、单位和硬件资源满足前文约束时使用本模块。若只需要更简单的上游功能、输入尚未准备好，或需要不同的数据模型/外设资源，应先选择相邻模块而不是强行调用本 API。

### 输入 / 输出

输入由各 API 的只读数组、配置、频率/阈值和平台对象组成；输出写入 result/output/buffer 参数或由 Get API 返回。调用者负责数组容量、生命周期和物理单位；失败返回时输出不是有效结果。

### 调用顺序

```text
SignalMAD_Process
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

### `signal_algorithm_status_t SignalMAD_Process(const float *samples, uint32_t count, float *workspace, uint32_t workspace_count, signal_mad_result_t *result);`

**它做什么：** 计算中位数、绝对中位差 MAD 和高斯噪声等效 sigma 估计。

**什么时候调用：** 对调用者提供的数据执行一次同步计算或生成，并在成功后写入输出对象/数组。

| 参数 | 类型 | 初学者解释 |
|---|---|---|
| `samples` | `const float *` | 输入样本，只读，单位任意但必须一致。 |
| `count` | `uint32_t` | 样本数，必须大于 0 且不超过 INT32_MAX。 |
| `workspace` | `float *` | 调用者提供的临时 float 数组，会被重排覆盖。 |
| `workspace_count` | `uint32_t` | 临时数组容量，至少为 count。 |
| `result` | `signal_mad_result_t *` | 输出中位数、MAD、1.4826MAD，单位与输入相同。 |

**返回：** 成功返回 SIGNAL_ALGORITHM_OK；参数、空间或数值非法返回错误码。

**最小调用形状：** `SignalMAD_Process(...);`。可直接从 README_MINIMAL_EXAMPLE.c 复制正常流程；README_FULL_EXAMPLE.c 展示全部公开 API，其中取消类 API 会以 #if 0 隔离。

**注意：** 所有指针和数组都由调用者拥有；先检查返回值。异步采集、DMA 或回调还在使用 buffer 时，不能读取结果或改写该 buffer。

### 示例、模块链与验收

- 最小入门：`README_MINIMAL_EXAMPLE.c`，只保留正常入口和结果读取。
- 全功能：`README_FULL_EXAMPLE.c`，以正确顺序展示当前头文件全部公开 API；`Stop` 等非常规路径不会默认执行。
- 模块链：先由上游提供单位、采样率和有效数据，再调用本模块；成功后将输出交给显示、控制、测量或下一步 DSP。硬件资源仍以 SysConfig 合约为唯一来源。
- 文档验收：README/API、两份示例和头文件会由 `tools/validate_beginner_documentation.ps1` 覆盖检查；这只表示文档与源码签名一致，不代替未进行的实板验证。

### 模块链

`上游采集/配置 -> 本模块 -> 检查返回值和结果 -> 测量、显示、控制或下一步 DSP`。只有确认本模块的输出单位和有效状态后，才交给下一模块。

