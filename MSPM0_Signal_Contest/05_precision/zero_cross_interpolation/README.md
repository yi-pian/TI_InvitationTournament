# ZeroCrossInterpolation：在两个采样点之间找过零位置

> **LEVEL C / REAL ALGORITHM MODULE：** 阈值、左右样本、方向、退化斜率和 fractional sample 边界必须与 Zero Cross 事件完全一致。

## 第一次使用 Zero Cross Interpolation？从这里开始

目标：“把 Zero Cross 给出的整数夹点变成例如 12.25 sample 的小数过零位置”。

### STEP 1：加入工程

链接 `MSPM0_Signal_Contest/05_precision/zero_cross_interpolation/signal_zero_cross_interpolation.c`。Include Path 需要：本目录、`03_measurement/common`、`03_measurement/frequency_zero_cross`。必须同时链接上游正式 `signal_zero_cross.c` 才能完成整条链。

### STEP 2：include

```c
#include "signal_zero_cross_interpolation.h"
```

该头文件会包含 `signal_zero_cross.h`。

### STEP 3：变量

```c
signal_zero_cross_event_t events[E];  /* Zero Cross 输出 */
float crossing_samples[E];            /* 本模块输出，单位 sample */
signal_zero_cross_interpolation_result_t result;
```

### STEP 4：参数

| 参数 | 怎么设 | 错误现象 | SysConfig |
|---|---|---|---|
| `sample_count` | 等于电压数组 N | event 索引越界或漏检 | 否 |
| `threshold_v` | 必须等于 Zero Cross 的 threshold | 插值位置系统性偏移或夹点不成立 | 否 |
| `event_count` | 用 `zero_cross_result.event_count` | 写大读到未初始化 event | 否 |
| `position_capacity` | 至少 event_count | 返回空间不足 | 否 |

线性插值假设阈值附近两个采样点之间近似直线；提高 Fs 通常会使该假设更好，但真实 Fs 是上游硬件参数。

### STEP 5：SysConfig

**【不需要 SysConfig】**。若为了提高时间分辨率改变 ADC Fs，才去修改采集 Profile，并同步后续 `sample_rate_hz`。

### STEP 6：初始化

没有 Init；必须先成功完成 `SignalZeroCross_Process`。

### STEP 7：真正调用

```c
signal_algorithm_status_t status = SignalZeroCrossInterpolation_Process(
    centered_v, N, zc_cfg.threshold_v,
    events, zc_result.event_count,
    crossing_samples, E, &result);
```

### STEP 8：结果

`result.position_count` 是有效位置数；`crossing_samples[i]` 单位是 sample，不是秒或 Hz。要变成时间可除以 Fs，要变成频率继续接 Multi Cycle Average。

### STEP 9：连接

```c
signal_multi_cycle_average_result_t frequency;
if (SignalZeroCrossInterpolation_Process(centered_v, N, 0.0f,
        events, zc.event_count, crossing_samples, E, &interp) ==
    SIGNAL_ALGORITHM_OK) {
    (void)SignalMultiCycleAverage_Process(crossing_samples,
        interp.position_count, actual_fs_hz, &frequency);
}
```

第二种用途：两路对应的 `crossing_samples` 可选同方向事件交给 `SignalPhase_FromZeroCross`。

### STEP 10：Build

`signal_zero_cross.h not found`=缺上游 Include Path；undefined symbol=未链接本模块 `.c`；索引/夹点错误=event 与当前电压数组不匹配；容量错误=扩大 positions。

### STEP 11：验证

样本 `x[0]=-1`、`x[1]=+1`、threshold=0，对应 event `{0,1}`，位置应约为 `0.5 sample`。

### STEP 12：常见修改

1. **Fs 100 k→200 k**：本函数不用改；后续频率计算改为真实 200 kHz，ADC Timer/SysConfig 同步。
2. **阈值不为 0**：Zero Cross 和本函数两处都传同一个值。
3. **只保留上升沿**：在上游 Zero Cross 选 RISING，本模块不再过滤方向。

### STEP 13：完整最小示例

```c
#include "signal_zero_cross_interpolation.h"
void Interpolate(void)
{
    const float x[2] = {-1.0f, 1.0f};
    const signal_zero_cross_event_t e[1] = {
        {.left_index=0U, .right_index=1U, .direction=SIGNAL_ZERO_CROSS_RISING}
    };
    float p[1];
    signal_zero_cross_interpolation_result_t r;
    (void)SignalZeroCrossInterpolation_Process(x, 2U, 0.0f,
        e, 1U, p, 1U, &r); /* p[0]≈0.5 sample */
}
```

下面是原理、限制、验证证据和完整 API Reference。

## 1 这个算法是干什么的？

ADC 只在离散时刻采样，但真正过零通常发生在两点之间。线性插值把两点连成直线，估计阈值交点位于一个采样间隔的多少比例处。

## 2 一个最简单的例子

```text
index 0: -1 V
index 1: +3 V
threshold: 0 V
从 -1 走到 +3 共 4 V，到 0 走了 1 V
过零位置 = 0 + 1/4 = 0.25 sample
```

## 3 原理

`position = left + (threshold-x_left)/(x_right-x_left)`。它为什么提高精度：不用把过零时间硬量化成整数采样点，而是利用两点幅值推断点间位置。

## 4 比赛里什么时候用？

正弦、三角波或边沿附近可近似直线的信号做过零频率/相位。它是廉价精度增强，不需要提高数组长度。

## 5 输入

原电压数组（V）、样本总数、与检测完全相同的 `threshold_v`、ZeroCross 事件数组和事件数。

## 6 输出

`float crossing_positions_samples[]`，单位 sample。乘以 `1/sample_rate_hz` 可转为秒，但通常直接交给 MultiCycleAverage。

## 7 API怎么调用

```c
float positions[32];
signal_zero_cross_interpolation_result_t r;
SignalZeroCrossInterpolation_Process(
    centered_v, count, 0.0f,
    events, event_count, positions, 32U, &r);
```

## 8 参数怎么改

只有 `threshold_v`，必须与 ZeroCross 使用同一个值。输出容量至少等于事件数。

## 9 参数改大会怎样

阈值改变会改变交点位置。纯正弦两路频率测量中固定阈值偏差通常不改变长期周期，但有失真/噪声时会产生系统偏差。阈值不能为了“结果好看”随意调整。

## 10 这个算法的代价是什么

Benefits：O(K)、每事件一次除法、显著减小整数样本量化误差。

Trade-offs：除法在 M0+ 上有成本；噪声同时改变两点电压；高曲率/快速边沿时直线近似有误差。

## 11 什么时候不要用

- 事件索引不是相邻点；
- 两点相等或阈值不在两点之间；
- 采样严重混叠；
- 期望靠插值恢复采样带宽之外的信息。

## 12 怎么和前一个模块接

```text
ZeroCross event {left,right}[] -> LinearInterpolation
同一 voltage_v[] --------------------↑
```

## 13 怎么和后一个模块接

```text
┌──── LinearInterpolation ────┐
│ x_left, x_right, threshold  │
│ fractional sample position │
└────────────┬────────────────┘
             ├──> MultiCycleAverage -> Hz
             └──> Phase timing -> deg
```

## 14 最小Demo

```c
const float x[] = {-1.0f, 3.0f};
signal_zero_cross_event_t e = {0U, 1U, SIGNAL_ZERO_CROSS_RISING};
float p;
signal_zero_cross_interpolation_result_t r;
(void)SignalZeroCrossInterpolation_Process(x, 2U, 0.0f,
    &e, 1U, &p, 1U, &r); /* p=0.25 sample */
```

## 15 PC测试

解析例 `-1→+3` 得 0.25 sample；合成 1234.5 Hz 带 DC 正弦和 1000 Hz 去 DC 正弦均通过频率链真值测试。

排查：返回 OUT_OF_RANGE 检查事件是否属于同一数组、阈值是否一致；频率仍抖动看 SNR、滞回和观测周期数。

## 16 MCU资源

O(K) 时间、内部 O(1) RAM、输出 `4*K` 字节。每事件含软件浮点除法；通常 K 远小于样本 N。

## 17 验证状态

PC_VERIFIED：2026-08-07，严格编译、解析插值和完整测频链测试通过；未实板验证。

## 17. 统一 API 教程（已按当前头文件核对）

本节由当前公开头文件、实现中实际出现的状态码和正式模块注册表生成。它补充前文的场景教程；函数签名变化时必须重新运行 `tools/upgrade_formal_beginner_docs.ps1`，不要手工保留旧 API。

遵循仓库的 [Beginner README 标准](../../00_docs/BEGINNER_README_STANDARD.md)：先用最小示例完成一次正常数据流，再按需要阅读全功能示例和本节 API 细节。

### 什么时候用 / 什么时候不要用

当题目需要“zero_cross_interpolation”目录对应的公开功能，并且输入数据、单位和硬件资源满足前文约束时使用本模块。若只需要更简单的上游功能、输入尚未准备好，或需要不同的数据模型/外设资源，应先选择相邻模块而不是强行调用本 API。

### 输入 / 输出

输入由各 API 的只读数组、配置、频率/阈值和平台对象组成；输出写入 result/output/buffer 参数或由 Get API 返回。调用者负责数组容量、生命周期和物理单位；失败返回时输出不是有效结果。

### 调用顺序

```text
SignalZeroCrossInterpolation_Process
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

### `signal_algorithm_status_t SignalZeroCrossInterpolation_Process(const float *voltage_v, uint32_t sample_count, float threshold_v, const signal_zero_cross_event_t *events, uint32_t event_count, float *crossing_positions_samples, uint32_t position_capacity, signal_zero_cross_interpolation_result_t *result);`

**它做什么：** 对过零事件两侧样本做直线插值，得到带小数的样本位置。

**什么时候调用：** 对调用者提供的数据执行一次同步计算或生成，并在成功后写入输出对象/数组。

| 参数 | 类型 | 初学者解释 |
|---|---|---|
| `voltage_v` | `const float *` | 原始或去直流电压数组，单位 V，只读。 |
| `sample_count` | `uint32_t` | voltage_v 的总点数。 |
| `threshold_v` | `float` | 过零检测使用的同一阈值，单位 V。 |
| `events` | `const signal_zero_cross_event_t *` | SignalZeroCross_Process 输出的事件数组。 |
| `event_count` | `uint32_t` | 事件数，必须大于 0。 |
| `crossing_positions_samples` | `float *` | 输出位置，单位 sample，例如 12.25 表示第 12 与 13 点之间。 |
| `position_capacity` | `uint32_t` | 输出容量，至少为 event_count。 |
| `result` | `signal_zero_cross_interpolation_result_t *` | 输出有效位置数量。 |

**返回：** 成功返回 SIGNAL_ALGORITHM_OK；索引、夹点、空间或数值非法返回错误码。

**最小调用形状：** `SignalZeroCrossInterpolation_Process(...);`。可直接从 README_MINIMAL_EXAMPLE.c 复制正常流程；README_FULL_EXAMPLE.c 展示全部公开 API，其中取消类 API 会以 #if 0 隔离。

**注意：** 所有指针和数组都由调用者拥有；先检查返回值。异步采集、DMA 或回调还在使用 buffer 时，不能读取结果或改写该 buffer。

### 示例、模块链与验收

- 最小入门：`README_MINIMAL_EXAMPLE.c`，只保留正常入口和结果读取。
- 全功能：`README_FULL_EXAMPLE.c`，以正确顺序展示当前头文件全部公开 API；`Stop` 等非常规路径不会默认执行。
- 模块链：先由上游提供单位、采样率和有效数据，再调用本模块；成功后将输出交给显示、控制、测量或下一步 DSP。硬件资源仍以 SysConfig 合约为唯一来源。
- 文档验收：README/API、两份示例和头文件会由 `tools/validate_beginner_documentation.ps1` 覆盖检查；这只表示文档与源码签名一致，不代替未进行的实板验证。

### 模块链

`上游采集/配置 -> 本模块 -> 检查返回值和结果 -> 测量、显示、控制或下一步 DSP`。只有确认本模块的输出单位和有效状态后，才交给下一模块。

