# MultiBinEnergy：不要只盯一个 FFT bin

> **LEVEL C / REAL ALGORITHM MODULE：** 它定义闭区间 bin、平方能量和 root-sum-square，并作为 Harmonic 的正式依赖；不能把 magnitude 求和冒充能量。

**比赛复制清单：** `signal_multi_bin_energy.c`、`signal_multi_bin_energy.h`、`03_measurement/common/signal_algorithm_status.h`。输入必须是非负 magnitude，`first_bin..last_bin` 为闭区间；无 SysConfig/Pin。

## 1 这个算法是干什么的？

把峰附近多个 bin 的 magnitude 平方相加，收集加窗/非整 bin 时散在主瓣中的能量。

## 2 一个最简单的例子

bin 值 3 和 4 的能量为 `3²+4²=25`，RSS 为 5。

## 3 原理

频率不落在 bin 中心时，能量分散；Hann 也主动把峰变宽。平方和比只读中心 bin 更接近整条主瓣能量。

## 4 比赛里什么时候用？

Hann FFT 的谐波/THD、弱频率偏离 bin 中心时；24_C 以 `radius_bins=2` 为起点做 H1~H5 邻近 bin 能量积分，输入必须是线性 magnitude，窗口不可重叠。

## 5 输入

非负线性 magnitude、bin_count、center、radius。

## 6 输出

实际 start/end、`energy=sum(M²)`、`root_sum_square=sqrt(energy)`，标度仍继承 raw spectrum。

## 7 API怎么调用

`SignalMultiBinEnergy_Process(m,bins,k,2U,&r);`

## 8 参数怎么改

BASIC 用 radius=0；Hann COMPETITION 可从 2 开始，用已知信号扫 fractional bin 验证。

## 9 参数改大会怎样

半径大收集更多主瓣，也引入更多噪声和邻近谱线；相邻谐波窗口可能重叠。

## 10 这个算法的代价是什么

Benefits：降低 scalloping/泄漏造成的能量漏计。Trade-offs：噪声带宽增大、分离近邻变差。

## 11 什么时候不要用

窗口重叠、多音非常接近、或需要单 bin complex phase。

## 12 怎么和前一个模块接

`FFT -> Magnitude -> MultiBinEnergy`

## 13 怎么和后一个模块接

`MultiBinEnergy -> Harmonic -> THD`

## 14 最小Demo

```c
signal_multi_bin_energy_result_t r;
(void)SignalMultiBinEnergy_Process(m,bins,k,2U,&r);
```

## 15 PC测试

`{0,3,4,0}` 中心1半径1，Expected energy25/RSS5，PASS。

## 16 MCU资源

O(2R+1)，O(1)，每 bin 一次平方累加。

## 17 验证状态

PC_VERIFIED；未实板。

## 17. 统一 API 教程（已按当前头文件核对）

本节由当前公开头文件、实现中实际出现的状态码和正式模块注册表生成。它补充前文的场景教程；函数签名变化时必须重新运行 `tools/upgrade_formal_beginner_docs.ps1`，不要手工保留旧 API。

遵循仓库的 [Beginner README 标准](../../00_docs/BEGINNER_README_STANDARD.md)：先用最小示例完成一次正常数据流，再按需要阅读全功能示例和本节 API 细节。

### 什么时候用 / 什么时候不要用

当题目需要“multi_bin_energy”目录对应的公开功能，并且输入数据、单位和硬件资源满足前文约束时使用本模块。若只需要更简单的上游功能、输入尚未准备好，或需要不同的数据模型/外设资源，应先选择相邻模块而不是强行调用本 API。

### 输入 / 输出

输入由各 API 的只读数组、配置、频率/阈值和平台对象组成；输出写入 result/output/buffer 参数或由 Get API 返回。调用者负责数组容量、生命周期和物理单位；失败返回时输出不是有效结果。

### 调用顺序

```text
SignalMultiBinEnergy_Process
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

### `signal_algorithm_status_t SignalMultiBinEnergy_Process(const float *magnitude, uint32_t bin_count, uint32_t center_bin, uint32_t radius_bins, signal_multi_bin_energy_result_t *result);`

**它做什么：** 把中心 bin 左右指定半径内的 magnitude 平方后求和。

**什么时候调用：** 对调用者提供的数据执行一次同步计算或生成，并在成功后写入输出对象/数组。

| 参数 | 类型 | 初学者解释 |
|---|---|---|
| `magnitude` | `const float *` | 非负线性 magnitude 数组。 |
| `bin_count` | `uint32_t` | 数组长度。 |
| `center_bin` | `uint32_t` | 中心索引。 |
| `radius_bins` | `uint32_t` | 左右半径；0 为 BASIC 单 bin。 |
| `result` | `signal_multi_bin_energy_result_t *` | 输出实际起止 bin、平方和与平方根。 |

**返回：** 成功返回 SIGNAL_ALGORITHM_OK；范围或数值非法返回错误码。

**最小调用形状：** `SignalMultiBinEnergy_Process(...);`。可直接从 README_MINIMAL_EXAMPLE.c 复制正常流程；README_FULL_EXAMPLE.c 展示全部公开 API，其中取消类 API 会以 #if 0 隔离。

**注意：** 所有指针和数组都由调用者拥有；先检查返回值。异步采集、DMA 或回调还在使用 buffer 时，不能读取结果或改写该 buffer。

### 示例、模块链与验收

- 最小入门：`README_MINIMAL_EXAMPLE.c`，只保留正常入口和结果读取。
- 全功能：`README_FULL_EXAMPLE.c`，以正确顺序展示当前头文件全部公开 API；`Stop` 等非常规路径不会默认执行。
- 模块链：先由上游提供单位、采样率和有效数据，再调用本模块；成功后将输出交给显示、控制、测量或下一步 DSP。硬件资源仍以 SysConfig 合约为唯一来源。
- 文档验收：README/API、两份示例和头文件会由 `tools/validate_beginner_documentation.ps1` 覆盖检查；这只表示文档与源码签名一致，不代替未进行的实板验证。

### 模块链

`上游采集/配置 -> 本模块 -> 检查返回值和结果 -> 测量、显示、控制或下一步 DSP`。只有确认本模块的输出单位和有效状态后，才交给下一模块。

