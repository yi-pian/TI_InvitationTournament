# Log Parabolic Interpolation：在对数谱上估计 bin 之间的峰

> **LEVEL C / REAL ALGORITHM MODULE：** 对数域要求 magnitude 全部为正，并有局部峰、退化分母和 offset 边界；不能把线性/对数插值公式混用。

**比赛复制清单：** `signal_log_parabolic_interpolation.c`、`signal_log_parabolic_interpolation.h`、`03_measurement/common/signal_algorithm_status.h`。输入是线性 magnitude，模块内部取对数；不要预先传 dB 数组。无 SysConfig/Pin。

## 1 这个算法是干什么的？

读取 FFT 峰值左右三个正幅值，先取对数，再用抛物线顶点估计真实峰在整数 bin 之间的位置。

## 2 一个最简单的例子

三个幅值来自 `exp(-(x-0.25)^2)` 在 x=-1/0/1 的取值，对数后刚好是一条顶点在 +0.25 的抛物线，算法应输出 offset=0.25。

## 3 原理

令 `L=ln(magnitude)`，三点顶点偏移为 `0.5*(Lleft-Lright)/(Lleft-2Lcenter+Lright)`。频率是 `(peak_bin+offset)*Fs/N`。三点插值能给出 bin 间位置，是因为局部主瓣顶部可近似为平滑二次曲线。

## 4 比赛里什么时候用？

单一突出正弦峰、已完成去 DC/加窗/FFT/Magnitude，且希望比直接最大 bin 更细的频率估计时。需与线性抛物线在你的窗和 SNR 下做 PC 对比。

## 5 输入

正的线性 magnitude 数组、bin_count、内部 peak_index、sample_rate_hz、fft_size。

## 6 输出

`bin_offset`、`fractional_bin`、`frequency_hz` 和插值后的线性 magnitude；频率单位 Hz。

## 7 API怎么调用

```c
signal_log_parabolic_result_t r;
SignalLogParabolicInterpolation_Process(m,bins,k,Fs,N,&r);
```

## 8 参数怎么改

没有可调阈值。应修改前级窗函数、N 和采样率，并用已知 fractional-bin 扫频决定是否采用本方法。

## 9 参数改大会怎样

N 大使 bin 更密但观测更久、RAM/CPU更高；加窗改变主瓣形状，因此也改变插值偏差。低 SNR 时三个点的对数会放大相对误差。

## 10 这个算法的代价是什么

Benefits：O(1) 获得 bin 间频率，常比整数峰精细。Trade-offs：只是假设局部形状，含 log/exp，幅值必须大于零，窗函数与噪声会造成偏差。

## 11 什么时候不要用

峰在 DC/Nyquist 边界、平顶、相邻双音重叠、任一三点≤0、低 SNR 或严重泄漏时不要相信输出。

## 12 怎么和前一个模块接

`RemoveDC -> Window -> FFT -> Magnitude -> PeakDetect -> LogParabolic`

## 13 怎么和后一个模块接

`frequency_hz -> Result / Harmonic center-frequency mapping`

## 14 最小Demo

见第 7 节；必须先检查 PeakDetect 返回值且 peak_index 有左右邻点。

## 15 PC测试

对数域解析抛物线真值 offset=0.25、frequency=22.5 Hz、magnitude=1，严格比较 PASS；这不等于所有 FFT 窗上的零偏差。

## 16 MCU资源

O(1) RAM/CPU 数量级，但 M0+ 上 logf/expf 软件运算较慢；每帧只对一个峰调用通常可接受。

## 17 验证状态

PC_VERIFIED（解析三点真值）；未 BOARD_VERIFIED。实际 Hann/噪声偏差必须按比赛频段扫频再决定。

## 17. 统一 API 教程（已按当前头文件核对）

本节由当前公开头文件、实现中实际出现的状态码和正式模块注册表生成。它补充前文的场景教程；函数签名变化时必须重新运行 `tools/upgrade_formal_beginner_docs.ps1`，不要手工保留旧 API。

遵循仓库的 [Beginner README 标准](../../00_docs/BEGINNER_README_STANDARD.md)：先用最小示例完成一次正常数据流，再按需要阅读全功能示例和本节 API 细节。

### 什么时候用 / 什么时候不要用

当题目需要“log_parabolic_interpolation”目录对应的公开功能，并且输入数据、单位和硬件资源满足前文约束时使用本模块。若只需要更简单的上游功能、输入尚未准备好，或需要不同的数据模型/外设资源，应先选择相邻模块而不是强行调用本 API。

### 输入 / 输出

输入由各 API 的只读数组、配置、频率/阈值和平台对象组成；输出写入 result/output/buffer 参数或由 Get API 返回。调用者负责数组容量、生命周期和物理单位；失败返回时输出不是有效结果。

### 调用顺序

```text
SignalLogParabolicInterpolation_Process
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

### `signal_algorithm_status_t SignalLogParabolicInterpolation_Process(const float *magnitude, uint32_t bin_count, uint32_t peak_index, float sample_rate_hz, uint32_t fft_size, signal_log_parabolic_result_t *result);`

**它做什么：** 对峰值及左右三个正的线性 magnitude 取自然对数后做抛物线插值。

**什么时候调用：** 对调用者提供的数据执行一次同步计算或生成，并在成功后写入输出对象/数组。

| 参数 | 类型 | 初学者解释 |
|---|---|---|
| `magnitude` | `const float *` | 线性幅值谱，三个参与值必须有限且大于 0。 |
| `bin_count` | `uint32_t` | magnitude 数组长度。 |
| `peak_index` | `uint32_t` | 整数峰值索引，必须有左右相邻点且自身为局部最大。 |
| `sample_rate_hz` | `float` | 采样率，Hz。 |
| `fft_size` | `uint32_t` | FFT 点数。 |
| `result` | `signal_log_parabolic_result_t *` | 输出 bin 偏移、分数 bin、频率 Hz 和插值线性幅值。 |

**返回：** 成功返回 SIGNAL_ALGORITHM_OK；非正幅值、平顶或非法参数返回错误码。

**最小调用形状：** `SignalLogParabolicInterpolation_Process(...);`。可直接从 README_MINIMAL_EXAMPLE.c 复制正常流程；README_FULL_EXAMPLE.c 展示全部公开 API，其中取消类 API 会以 #if 0 隔离。

**注意：** 所有指针和数组都由调用者拥有；先检查返回值。异步采集、DMA 或回调还在使用 buffer 时，不能读取结果或改写该 buffer。

### 示例、模块链与验收

- 最小入门：`README_MINIMAL_EXAMPLE.c`，只保留正常入口和结果读取。
- 全功能：`README_FULL_EXAMPLE.c`，以正确顺序展示当前头文件全部公开 API；`Stop` 等非常规路径不会默认执行。
- 模块链：先由上游提供单位、采样率和有效数据，再调用本模块；成功后将输出交给显示、控制、测量或下一步 DSP。硬件资源仍以 SysConfig 合约为唯一来源。
- 文档验收：README/API、两份示例和头文件会由 `tools/validate_beginner_documentation.ps1` 覆盖检查；这只表示文档与源码签名一致，不代替未进行的实板验证。

### 模块链

`上游采集/配置 -> 本模块 -> 检查返回值和结果 -> 测量、显示、控制或下一步 DSP`。只有确认本模块的输出单位和有效状态后，才交给下一模块。

