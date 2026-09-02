# SNR：频谱信噪比

> **LEVEL C / REAL ALGORITHM MODULE：** signal/noise bin 区域、能量平方和和 dB 解释容易被幅值/功率混用，继续保留正式模块。

**比赛复制清单：** `signal_snr.c`、`signal_snr.h`、`03_measurement/common/signal_algorithm_status.h`。输入为同一 FFT/窗/标度的 magnitude；无 SysConfig/Pin。本模块不是模拟前端全带宽仪器 SNR 的自动替代。

## 1 这个算法是干什么的？

把目标频带的平方能量与其余噪声 bin 的平方能量相比。

## 2 一个最简单的例子

signal energy=100、noise energy=6，SNR=`10log10(100/6)=12.2185 dB`。

## 3 原理

功率比用 `10log10`。目标 band 从噪声中自动排除；用户还可排 DC/谐波。若不排谐波，结果更像 SINAD。

## 4 比赛里什么时候用？

单音输出噪声质量比较，并且能明确分析带宽和排除项。

## 5 输入

magnitude、signal range、analysis range、excluded range 表。

## 6 输出

signal/noise energy、功率比、snr_db、noise_bin_count。

## 7 API怎么调用

```c
signal_snr_config_t c={s0,s1,a0,a1,exclude,exclude_count};
SignalSNR_Process(m,bins,&c,&r);
```

## 8 参数怎么改

signal band 覆盖主瓣；analysis band 对应报告带宽；excluded 显式列 DC、H2~等。

## 9 参数改大会怎样

分析带宽扩大收更多噪声，SNR通常下降；signal band 太宽会把噪声算进信号；排除太多会虚高。

## 10 这个算法的代价是什么

Benefits：定义透明。Trade-offs：强依赖窗/带宽/mask，不自动生成严格 IEEE 测量流程。

## 11 什么时候不要用

未说明带宽、窗和排除项；宽带噪声要换算 PSD 却直接用 bin 和；谐波未排却称 SNR。

## 12 怎么和前一个模块接

`FFT -> Magnitude -> SNR(config)`

## 13 怎么和后一个模块接

`snr_db -> report/threshold`。

## 14 最小Demo

```c
signal_snr_result_t r;
(void)SignalSNR_Process(m,bins,&cfg,&r);
```

## 15 PC测试

signal100、noise6、排除一条谐波，Expected/Measured 12.2184868 dB，PASS。

## 16 MCU资源

扫描分析 bin；每 bin 平方，最后一次 log10f。excluded range 多时比较增加。

## 17 验证状态

PC_VERIFIED（执行与范围逻辑）；尚未建立标准仪器对比。

## 17. 统一 API 教程（已按当前头文件核对）

本节由当前公开头文件、实现中实际出现的状态码和正式模块注册表生成。它补充前文的场景教程；函数签名变化时必须重新运行 `tools/upgrade_formal_beginner_docs.ps1`，不要手工保留旧 API。

遵循仓库的 [Beginner README 标准](../../00_docs/BEGINNER_README_STANDARD.md)：先用最小示例完成一次正常数据流，再按需要阅读全功能示例和本节 API 细节。

### 什么时候用 / 什么时候不要用

当题目需要“snr”目录对应的公开功能，并且输入数据、单位和硬件资源满足前文约束时使用本模块。若只需要更简单的上游功能、输入尚未准备好，或需要不同的数据模型/外设资源，应先选择相邻模块而不是强行调用本 API。

### 输入 / 输出

输入由各 API 的只读数组、配置、频率/阈值和平台对象组成；输出写入 result/output/buffer 参数或由 Get API 返回。调用者负责数组容量、生命周期和物理单位；失败返回时输出不是有效结果。

### 调用顺序

```text
SignalSNR_Process
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

### `signal_algorithm_status_t SignalSNR_Process(const float *magnitude, uint32_t bin_count, const signal_snr_config_t *config, signal_snr_result_t *result);`

**它做什么：** 在指定谱区间内，用目标 band 能量与其余未排除 bin 能量计算 SNR。

**什么时候调用：** 对调用者提供的数据执行一次同步计算或生成，并在成功后写入输出对象/数组。

| 参数 | 类型 | 初学者解释 |
|---|---|---|
| `magnitude` | `const float *` | 非负线性 magnitude 数组。 |
| `bin_count` | `uint32_t` | 数组长度。 |
| `config` | `const signal_snr_config_t *` | 目标范围、分析范围和可选排除范围（如 DC/谐波）。 |
| `result` | `signal_snr_result_t *` | 输出 signal/noise 能量、功率比、dB 和噪声 bin 数。 |

**返回：** 成功返回 SIGNAL_ALGORITHM_OK；无信号/噪声或范围非法返回错误。

**最小调用形状：** `SignalSNR_Process(...);`。可直接从 README_MINIMAL_EXAMPLE.c 复制正常流程；README_FULL_EXAMPLE.c 展示全部公开 API，其中取消类 API 会以 #if 0 隔离。

**注意：** 所有指针和数组都由调用者拥有；先检查返回值。异步采集、DMA 或回调还在使用 buffer 时，不能读取结果或改写该 buffer。

### 示例、模块链与验收

- 最小入门：`README_MINIMAL_EXAMPLE.c`，只保留正常入口和结果读取。
- 全功能：`README_FULL_EXAMPLE.c`，以正确顺序展示当前头文件全部公开 API；`Stop` 等非常规路径不会默认执行。
- 模块链：先由上游提供单位、采样率和有效数据，再调用本模块；成功后将输出交给显示、控制、测量或下一步 DSP。硬件资源仍以 SysConfig 合约为唯一来源。
- 文档验收：README/API、两份示例和头文件会由 `tools/validate_beginner_documentation.ps1` 覆盖检查；这只表示文档与源码签名一致，不代替未进行的实板验证。

### 模块链

`上游采集/配置 -> 本模块 -> 检查返回值和结果 -> 测量、显示、控制或下一步 DSP`。只有确认本模块的输出单位和有效状态后，才交给下一模块。

