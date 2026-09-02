# Autocorrelation：从“隔多久又像自己”找周期

> **LEVEL C / REAL ALGORITHM MODULE：** 归一化、lag0 排除、周期搜索范围和 O(N·lag) 资源风险需要统一处理。

**比赛复制清单：** `signal_autocorrelation.c`、`signal_autocorrelation.h`、`03_measurement/common/signal_algorithm_status.h`。无 SysConfig/Pin。先调用 `Process` 得到相关曲线，再调用 `FindPeriod`；两个步骤不能颠倒。

## 1 这个算法是干什么的？

把信号与延迟后的自己比较；若延迟一个周期，波形再次对齐，相关出现峰。

## 2 一个最简单的例子

周期32 sample 的正弦，在 lag32 自相关约1；Fs=32000 Hz，频率=1000 Hz。

## 3 原理

`R(lag)=corr(x[n],x[n+lag])`。lag0永远最大，所以必须用已知频率范围设置 `min_lag>=1`，在合理区间找下一峰。

## 4 比赛里什么时候用？

严重失真但周期稳定、阈值过零不可靠、低 SNR 周期性。

## 5 输入

通常去 DC 的 float 信号、N、max_lag、L+1输出；FindPeriod再传min/max lag和Fs。

## 6 输出

归一化相关曲线、整数 period lag、peak coefficient、frequency_hz。

## 7 API怎么调用

```c
SignalAutocorrelation_Process(x,N,L,r,L+1,&cr);
SignalAutocorrelation_FindPeriod(r,L+1,minL,maxL,Fs,&fr);
```

## 8 参数怎么改

由预计频率范围换算 lag：`min_lag≈Fs/f_max`，`max_lag≈Fs/f_min`。

## 9 参数改大会怎样

max_lag 大可测更低频，但 O(NL) 和 RAM 增；搜索区太宽易选倍周期。

## 10 这个算法的代价是什么

Benefits：不限正弦、抗局部噪声。Trade-offs：重计算、整数周期误差、倍/分频歧义。

## 11 什么时候不要用

单次 burst/瞬态、非平稳扫频、没有任何频率先验却直接全范围取最大。

## 12 怎么和前一个模块接

`ADC_ToVoltage -> RemoveDC -> Autocorrelation`

## 13 怎么和后一个模块接

`period_lag -> frequency result`；未来可对峰做抛物线亚样本插值。

## 14 最小Demo

```c
float r[65];
signal_autocorrelation_result_t a;
signal_autocorrelation_period_result_t p;
(void)SignalAutocorrelation_Process(x,256,64,r,65,&a);
(void)SignalAutocorrelation_FindPeriod(r,65,20,40,32000,&p);
```

## 15 PC测试

周期32正弦：lag0=1、period lag32、frequency1000 Hz，全部 PASS。

排查：选到 min_lag 说明范围太靠近0；选到2倍周期检查谐波/搜索；固定比例错查Fs。

## 16 MCU资源

O(NL)，输出 `4(L+1)`，内部 O(1)。M0+ 大 N/L 需降采样或另做 FFT autocorrelation，不在 ISR。

## 17 验证状态

PC_VERIFIED；未噪声蒙特卡洛和板端实时验证。

## 17. 统一 API 教程（已按当前头文件核对）

本节由当前公开头文件、实现中实际出现的状态码和正式模块注册表生成。它补充前文的场景教程；函数签名变化时必须重新运行 `tools/upgrade_formal_beginner_docs.ps1`，不要手工保留旧 API。

遵循仓库的 [Beginner README 标准](../../00_docs/BEGINNER_README_STANDARD.md)：先用最小示例完成一次正常数据流，再按需要阅读全功能示例和本节 API 细节。

### 什么时候用 / 什么时候不要用

当题目需要“autocorrelation”目录对应的公开功能，并且输入数据、单位和硬件资源满足前文约束时使用本模块。若只需要更简单的上游功能、输入尚未准备好，或需要不同的数据模型/外设资源，应先选择相邻模块而不是强行调用本 API。

### 输入 / 输出

输入由各 API 的只读数组、配置、频率/阈值和平台对象组成；输出写入 result/output/buffer 参数或由 Get API 返回。调用者负责数组容量、生命周期和物理单位；失败返回时输出不是有效结果。

### 调用顺序

```text
SignalAutocorrelation_Process -> SignalAutocorrelation_FindPeriod
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

### `signal_algorithm_status_t SignalAutocorrelation_Process(const float *samples, uint32_t count, uint32_t max_lag_samples, float *coefficients, uint32_t coefficient_capacity, signal_autocorrelation_result_t *result);`

**它做什么：** 计算 lag=0~max_lag 的归一化自相关。

**什么时候调用：** 对调用者提供的数据执行一次同步计算或生成，并在成功后写入输出对象/数组。

| 参数 | 类型 | 初学者解释 |
|---|---|---|
| `samples` | `const float *` | 输入数组，只读，通常先 RemoveDC。 |
| `count` | `uint32_t` | 样本点数。 |
| `max_lag_samples` | `uint32_t` | 最大 lag，必须小于 count。 |
| `coefficients` | `float *` | 输出数组，容量至少 max_lag+1。 |
| `coefficient_capacity` | `uint32_t` | 输出容量。 |
| `result` | `signal_autocorrelation_result_t *` | 输出 lag 数量。 |

**返回：** 成功返回 SIGNAL_ALGORITHM_OK。

**最小调用形状：** `SignalAutocorrelation_Process(...);`。可直接从 README_MINIMAL_EXAMPLE.c 复制正常流程；README_FULL_EXAMPLE.c 展示全部公开 API，其中取消类 API 会以 #if 0 隔离。

**注意：** 所有指针和数组都由调用者拥有；先检查返回值。异步采集、DMA 或回调还在使用 buffer 时，不能读取结果或改写该 buffer。

### `signal_algorithm_status_t SignalAutocorrelation_FindPeriod(const float *coefficients, uint32_t coefficient_count, uint32_t min_lag_samples, uint32_t max_lag_samples, float sample_rate_hz, signal_autocorrelation_period_result_t *result);`

**它做什么：** 在自相关的非零 lag 区间找最高正峰并换算周期频率。

**什么时候调用：** 执行该模块公开的功能；具体数据流以本节参数表和本 README 前面的场景说明为准。

| 参数 | 类型 | 初学者解释 |
|---|---|---|
| `coefficients` | `const float *` | 自相关数组，索引即 lag。 |
| `coefficient_count` | `uint32_t` | 数组长度。 |
| `min_lag_samples` | `uint32_t` | 搜索最小 lag，必须至少 1。 |
| `max_lag_samples` | `uint32_t` | 搜索最大 lag。 |
| `sample_rate_hz` | `float` | 采样率，Hz。 |
| `result` | `signal_autocorrelation_period_result_t *` | 输出整数周期 lag、峰系数和频率。 |

**返回：** 成功返回 SIGNAL_ALGORITHM_OK。

**最小调用形状：** `SignalAutocorrelation_FindPeriod(...);`。可直接从 README_MINIMAL_EXAMPLE.c 复制正常流程；README_FULL_EXAMPLE.c 展示全部公开 API，其中取消类 API 会以 #if 0 隔离。

**注意：** 所有指针和数组都由调用者拥有；先检查返回值。异步采集、DMA 或回调还在使用 buffer 时，不能读取结果或改写该 buffer。

### 示例、模块链与验收

- 最小入门：`README_MINIMAL_EXAMPLE.c`，只保留正常入口和结果读取。
- 全功能：`README_FULL_EXAMPLE.c`，以正确顺序展示当前头文件全部公开 API；`Stop` 等非常规路径不会默认执行。
- 模块链：先由上游提供单位、采样率和有效数据，再调用本模块；成功后将输出交给显示、控制、测量或下一步 DSP。硬件资源仍以 SysConfig 合约为唯一来源。
- 文档验收：README/API、两份示例和头文件会由 `tools/validate_beginner_documentation.ps1` 覆盖检查；这只表示文档与源码签名一致，不代替未进行的实板验证。

### 模块链

`上游采集/配置 -> 本模块 -> 检查返回值和结果 -> 测量、显示、控制或下一步 DSP`。只有确认本模块的输出单位和有效状态后，才交给下一模块。

