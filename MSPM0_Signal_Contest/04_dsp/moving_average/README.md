# MovingAverage：最简单的滑动平滑

> **LEVEL B / SIMPLE HELPER：** 保留原因是运行和与起始边界语义值得统一。只有一个同步入口 `SignalMovingAverage_Process(input, output, count, window_size)`；没有 Init、context、result struct、workspace 或 SysConfig。输入输出不能是同一数组。

## 比赛现场先复制和调用

复制 `signal_moving_average.c`、`signal_moving_average.h` 和 `03_measurement/common/signal_algorithm_status.h`，然后：

```c
#include "signal_moving_average.h"

static float filtered_v[SIGNAL_SAMPLE_COUNT];

signal_algorithm_status_t status = SignalMovingAverage_Process(
    voltage_v, filtered_v, SIGNAL_SAMPLE_COUNT, 5U);
if (status == SIGNAL_ALGORITHM_OK) {
    /* filtered_v[] 已可交给下一步。 */
}
```

`window_size=5` 表示当前点和最多前 4 点平均；前四个输出使用当前已有的 1/2/3/4 点，不会丢弃帧头。窗口越大，噪声更平滑，但带宽更低、变化更慢。完整原理和边界继续见下文。

## 1 这个算法是干什么的？

它把每个点替换为最近若干点的平均值，让随机抖动变小。当前实现是因果窗口，只用当前和过去样本。

## 2 一个最简单的例子

窗口 3，输入 `1,2,3,4,5`，输出 `1,1.5,2,3,4`。前两点还没有完整 3 点历史，所以使用已有的 1 点、2 点平均。

## 3 原理

完整窗口时 `y[n]=(x[n]+x[n-1]+...)/W`。实现维护滚动和：加入新点、减去离开窗口的旧点，因此不是每点重新相加 W 次。

## 4 比赛里什么时候用？

稳定 DC 读数、缓慢包络或低频量显示抖动时。频谱、THD、边沿时间测量前不能因为“看起来平滑”就默认加入。

## 5 输入

只读 `float input_samples[]`、`count>0`、`1<=window_size<=count`。单位可为 V 或其他明确单位。

## 6 输出

`float output_samples[count]`，单位不变。输入输出不得重叠。

## 7 API怎么调用

```c
SignalMovingAverage_Process(input_v, output_v, count, 5U);
```

## 8 参数怎么改

改 `window_size`。先从 3、5、7 这种小窗口开始，用已知频率正弦检查幅值和相位，而不是只看噪声标准差。

## 9 参数改大会怎样

3→21：随机噪声通常更小，但有效带宽更低、快速变化更钝、延迟更明显，高频正弦幅值衰减更大。

## 10 这个算法的代价是什么

Benefits：O(N)、无系数设计、易理解。

Trade-offs：本质是 FIR 低通；有通带衰减和延迟；块首状态不跨帧；额外输出 `4*N` 字节。

## 11 什么时候不要用

测上升时间、脉宽、高频幅值、THD、单次脉冲或需要相位精度时，除非已经计算并接受其频率响应。

## 12 怎么和前一个模块接

```text
ADC_ToVoltage -> MovingAverage
```

## 13 怎么和后一个模块接

```text
┌── MovingAverage ──┐
│ rolling sum / W   │
└───────┬───────────┘
        ├──> Mean / display
        └──> ZeroCross（确认延迟可接受）
```

## 14 最小Demo

```c
float in[] = {1,2,3,4,5}, out[5];
(void)SignalMovingAverage_Process(in, out, 5U, 3U);
```

## 15 PC测试

手算序列 Expected=`1,1.5,2,3,4`，五点全部一致；原地调用正确报错。PASS。

排查：开头与稳态不同是暖机边界；相位/幅值变化不是程序 bug，而是滤波器代价；跨帧跳变说明本块版本不保留上一帧历史。

## 16 MCU资源

O(N) 时间、内部 O(1)，输出 `4*N` 字节；每点约加、减、除。除数恒定但暖机期变化，编译器未必都优化成乘法。

## 17 验证状态

PC_VERIFIED：2026-08-07，严格编译与手算测试通过；未 BOARD_VERIFIED。

## 17. 统一 API 教程（已按当前头文件核对）

本节由当前公开头文件、实现中实际出现的状态码和正式模块注册表生成。它补充前文的场景教程；函数签名变化时必须重新运行 `tools/upgrade_formal_beginner_docs.ps1`，不要手工保留旧 API。

遵循仓库的 [Beginner README 标准](../../00_docs/BEGINNER_README_STANDARD.md)：先用最小示例完成一次正常数据流，再按需要阅读全功能示例和本节 API 细节。

### 什么时候用 / 什么时候不要用

当题目需要“moving_average”目录对应的公开功能，并且输入数据、单位和硬件资源满足前文约束时使用本模块。若只需要更简单的上游功能、输入尚未准备好，或需要不同的数据模型/外设资源，应先选择相邻模块而不是强行调用本 API。

### 输入 / 输出

输入由各 API 的只读数组、配置、频率/阈值和平台对象组成；输出写入 result/output/buffer 参数或由 Get API 返回。调用者负责数组容量、生命周期和物理单位；失败返回时输出不是有效结果。

### 调用顺序

```text
SignalMovingAverage_Process
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

### `signal_algorithm_status_t SignalMovingAverage_Process(const float *input_samples, float *output_samples, uint32_t count, uint32_t window_size);`

**它做什么：** 对每个样本计算“当前点及其之前若干点”的因果滑动平均。

**什么时候调用：** 对调用者提供的数据执行一次同步计算或生成，并在成功后写入输出对象/数组。

| 参数 | 类型 | 初学者解释 |
|---|---|---|
| `input_samples` | `const float *` | 输入数组，只读，单位由调用者决定。 |
| `output_samples` | `float *` | 输出数组，单位与输入相同，容量至少为 count；不得与输入重叠。 |
| `count` | `uint32_t` | 样本点数，必须大于 0。 |
| `window_size` | `uint32_t` | 平均窗口点数，范围 1~count；帧起始处使用已有的较短窗口。 |

**返回：** 成功返回 SIGNAL_ALGORITHM_OK；参数或数值非法返回错误码。

**最小调用形状：** `SignalMovingAverage_Process(...);`。可直接从 README_MINIMAL_EXAMPLE.c 复制正常流程；README_FULL_EXAMPLE.c 展示全部公开 API，其中取消类 API 会以 #if 0 隔离。

**注意：** 所有指针和数组都由调用者拥有；先检查返回值。异步采集、DMA 或回调还在使用 buffer 时，不能读取结果或改写该 buffer。

### 示例、模块链与验收

- 最小入门：`README_MINIMAL_EXAMPLE.c`，只保留正常入口和结果读取。
- 全功能：`README_FULL_EXAMPLE.c`，以正确顺序展示当前头文件全部公开 API；`Stop` 等非常规路径不会默认执行。
- 模块链：先由上游提供单位、采样率和有效数据，再调用本模块；成功后将输出交给显示、控制、测量或下一步 DSP。硬件资源仍以 SysConfig 合约为唯一来源。
- 文档验收：README/API、两份示例和头文件会由 `tools/validate_beginner_documentation.ps1` 覆盖检查；这只表示文档与源码签名一致，不代替未进行的实板验证。

### 模块链

`上游采集/配置 -> 本模块 -> 检查返回值和结果 -> 测量、显示、控制或下一步 DSP`。只有确认本模块的输出单位和有效状态后，才交给下一模块。

