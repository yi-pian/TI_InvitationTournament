# Correlation：找两路最像时要错开多少点

> 普通全相关默认 `arm_correlate_*`。本模块只在需要“限定 lag + 归一化 + 峰值/延时语义”的竞赛链中保留，不再作为普通相关核心的重复实现。

> **LEVEL C / REAL ALGORITHM MODULE：** lag 正负约定、重叠区归一化、workspace 容量和搜索范围容易写错，继续保留正式模块。

## 第一次使用 Correlation？从这里开始

目标：“在两路等长波形中搜索 B 相对 A 的最佳 lag 和归一化相关系数”。

### STEP 1：加入工程

链接 `MSPM0_Signal_Contest/04_dsp/correlation/signal_correlation.c`；Include Path 加本目录和 `03_measurement/common`。

### STEP 2：include

```c
#include "signal_correlation.h"
```

### STEP 3：变量 / Workspace

```c
float channel_a_v[N], channel_b_v[N];
float coefficients[2U * MAX_LAG + 1U];
signal_correlation_result_t result;
```

`coefficients[index]` 对应 `lag=index-MAX_LAG`。Workspace 由调用者创建。

### STEP 4：参数

| 参数 | 怎么选 | 调大/调小与错误现象 | SysConfig |
|---|---|---|---|
| `count=N` | 两路必须等长 | 大：计算更慢但统计更稳 | 否 |
| `max_lag_samples` | 覆盖预期最大延迟且 `<N` | 大：workspace/CPU 线性增；小：真实峰可能在边界外 | 否 |
| `capacity` | 至少 `2*max_lag+1` | 太小返回容量错误 | 否 |

通常先对两路分别 Remove DC。正 lag 表示 B 相对 A 更晚。

### STEP 5：SysConfig

算法：**【不需要 SysConfig】**。双通道同步采集对照 P02；采集 skew 和前端延迟仍需板测/校准。

### STEP 6：初始化

没有 Init；两路帧 ready 后调用。

### STEP 7：调用

```c
signal_algorithm_status_t status = SignalCorrelation_Process(
    channel_a_v, channel_b_v, N, MAX_LAG,
    coefficients, 2U * MAX_LAG + 1U, &result);
```

### STEP 8：结果

- `best_lag_samples/best_coefficient`：最大正相关。
- `best_absolute_lag_samples/best_absolute_coefficient`：绝对值最大，反相信号也可能被选中。
- 系数接近 ±1 表示形状高度相似，接近 0 表示弱相关。

### STEP 9：连接

```c
signal_phase_result_t phase;
if (SignalCorrelation_Process(a, b, N, MAX_LAG, coeff,
        2U * MAX_LAG + 1U, &corr) == SIGNAL_ALGORITHM_OK) {
    (void)SignalPhase_FromCorrelationLag((float)corr.best_lag_samples,
        period_samples, &phase);
}
```

第二种用途：直接把 `best_lag_samples/Fs` 变成时间延迟；此时无需 Phase。

### STEP 10：Build

undefined symbol=未链接 `.c`；容量错误=必须 `2L+1`；能量为零=输入为常量/全零；峰总在边界=MAX_LAG 太小；符号反=通道顺序接反。

### STEP 11：验证

把 B 设置为 A 延迟 3 sample，搜索范围至少 3，应得到正 lag≈3。再交换 A/B，应得到相反符号。

### STEP 12：常见修改

1. **MAX_LAG 64→128**：workspace 从 129 增到 257 个 float，多 512 B，计算量也增加。
2. **去 DC**：分别原地 Remove DC 后再相关，避免常量偏置主导系数。
3. **反相信号**：按题意决定使用正相关字段还是 absolute 字段。
4. **要亚采样 lag**：当前 API 输出整数 lag；不要在 Application 复制改算法，先记录 Integration Issue/选择正式精度模块。

### STEP 13：完整最小示例

```c
#include "signal_correlation.h"
static float coeff[17];
void FindLag(const float *a, const float *b)
{
    signal_correlation_result_t r;
    (void)SignalCorrelation_Process(a, b, 128U, 8U, coeff, 17U, &r);
}
```

下面是归一化、lag 符号、复杂度、验证证据和完整 API Reference。

## 1 这个算法是干什么的？

把 B 左右移动，逐个 lag 计算与 A 的相似度；最高点给出延迟。

## 2 一个最简单的例子

B 是 A 晚4点的复制，相关峰在 lag=+4，表示 B 更晚。

## 3 原理

`R_ab(lag)=sum a[n]b[n+lag]/sqrt(EaEb)`。归一化让整体增益不同仍可比较。周期波形会每隔一周期重复出现峰。

## 4 比赛里什么时候用？

两路波形相似但非纯正弦、有谐波、需要对齐/相位。

## 5 输入

等长 float A/B、count、max_lag<count、容量2L+1输出。

## 6 输出

[-L,+L]系数；best positive lag/coefficient；best absolute 用于可能反相的诊断。

## 7 API怎么调用

`SignalCorrelation_Process(a,b,N,L,corr,2*L+1,&r);`

## 8 参数怎么改

max_lag 应略大于预期延迟但小于半周期，避免选到等价周期峰。

## 9 参数改大会怎样

L 大能找更长延迟，但 CPU/输出 RAM增加，周期歧义增多。

## 10 这个算法的代价是什么

Benefits：不限正弦、对幅度比例鲁棒。Trade-offs：O(NL)、整数 lag、DC会制造高相关背景。

## 11 什么时候不要用

两路形状不同、频率不同、未 RemoveDC、实时预算不足或搜索范围跨多个周期。

## 12 怎么和前一个模块接

`DualADC -> 两路 RemoveDC -> Correlation`

## 13 怎么和后一个模块接

`best_lag -> Phase_FromCorrelationLag`；亚样本 lag 后续可加峰插值。

## 14 最小Demo

```c
float c[17]; signal_correlation_result_t r;
(void)SignalCorrelation_Process(a,b,N,8,c,17,&r);
```

## 15 PC测试

周期32、B延迟4，Expected lag+4/coefficient1，全部一致；换相位得到 -45°，PASS。

排查：峰在边界扩大/重新限定 L；多峰检查周期歧义；负峰最强看 best_absolute 和是否反相。

## 16 MCU资源

O(N(2L+1))，输出 `4(2L+1)`，内部 O(1)，每 lag 一次 sqrtf。大规模可未来用 FFT correlation，但不能假装当前已实现。

## 17 验证状态

PC_VERIFIED；未板端实时预算。

## 17. 统一 API 教程（已按当前头文件核对）

本节由当前公开头文件、实现中实际出现的状态码和正式模块注册表生成。它补充前文的场景教程；函数签名变化时必须重新运行 `tools/upgrade_formal_beginner_docs.ps1`，不要手工保留旧 API。

遵循仓库的 [Beginner README 标准](../../00_docs/BEGINNER_README_STANDARD.md)：先用最小示例完成一次正常数据流，再按需要阅读全功能示例和本节 API 细节。

### 什么时候用 / 什么时候不要用

当题目需要“correlation”目录对应的公开功能，并且输入数据、单位和硬件资源满足前文约束时使用本模块。若只需要更简单的上游功能、输入尚未准备好，或需要不同的数据模型/外设资源，应先选择相邻模块而不是强行调用本 API。

### 输入 / 输出

输入由各 API 的只读数组、配置、频率/阈值和平台对象组成；输出写入 result/output/buffer 参数或由 Get API 返回。调用者负责数组容量、生命周期和物理单位；失败返回时输出不是有效结果。

### 调用顺序

```text
SignalCorrelation_Process
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

### `signal_algorithm_status_t SignalCorrelation_Process(const float *samples_a, const float *samples_b, uint32_t count, uint32_t max_lag_samples, float *coefficients, uint32_t coefficient_capacity, signal_correlation_result_t *result);`

**它做什么：** 计算归一化互相关 R_ab[lag]=corr(a[n],b[n+lag])。

**什么时候调用：** 对调用者提供的数据执行一次同步计算或生成，并在成功后写入输出对象/数组。

| 参数 | 类型 | 初学者解释 |
|---|---|---|
| `samples_a` | `const float *` | A 路输入，只读。 |
| `samples_b` | `const float *` | B 路输入，只读，与 A 等长。 |
| `count` | `uint32_t` | 每路点数。 |
| `max_lag_samples` | `uint32_t` | 搜索 -max_lag~+max_lag，必须小于 count。 |
| `coefficients` | `float *` | 输出长度 2max_lag+1；索引 lag+max_lag。 |
| `coefficient_capacity` | `uint32_t` | 输出容量。 |
| `result` | `signal_correlation_result_t *` | 输出最大正相关和最大绝对相关的 lag/系数。 |

**返回：** 成功返回 SIGNAL_ALGORITHM_OK；能量为零或参数非法返回错误。

**最小调用形状：** `SignalCorrelation_Process(...);`。可直接从 README_MINIMAL_EXAMPLE.c 复制正常流程；README_FULL_EXAMPLE.c 展示全部公开 API，其中取消类 API 会以 #if 0 隔离。

**注意：** 所有指针和数组都由调用者拥有；先检查返回值。异步采集、DMA 或回调还在使用 buffer 时，不能读取结果或改写该 buffer。

### 示例、模块链与验收

- 最小入门：`README_MINIMAL_EXAMPLE.c`，只保留正常入口和结果读取。
- 全功能：`README_FULL_EXAMPLE.c`，以正确顺序展示当前头文件全部公开 API；`Stop` 等非常规路径不会默认执行。
- 模块链：先由上游提供单位、采样率和有效数据，再调用本模块；成功后将输出交给显示、控制、测量或下一步 DSP。硬件资源仍以 SysConfig 合约为唯一来源。
- 文档验收：README/API、两份示例和头文件会由 `tools/validate_beginner_documentation.ps1` 覆盖检查；这只表示文档与源码签名一致，不代替未进行的实板验证。

### 模块链

`上游采集/配置 -> 本模块 -> 检查返回值和结果 -> 测量、显示、控制或下一步 DSP`。只有确认本模块的输出单位和有效状态后，才交给下一模块。

