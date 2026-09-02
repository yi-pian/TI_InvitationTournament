# SFDR：最大杂散离主信号有多远

> **LEVEL C / REAL ALGORITHM MODULE：** 主峰排除区、最大杂散搜索和 dB 比值必须在同一种 magnitude/幅值语义下执行。

**比赛复制清单：** `signal_sfdr.c`、`signal_sfdr.h`、`03_measurement/common/signal_algorithm_status.h`。输入是同一频谱标度下的非负 magnitude；无 SysConfig/Pin。

## 1 这个算法是干什么的？

找主信号 band 里的最大值和 band 外最大 spur，计算幅值比 dB。

## 2 一个最简单的例子

主峰10、spur2，SFDR=`20log10(10/2)=13.9794 dB`。

## 3 原理

幅值比用 20log10。主瓣占多个 bin 时必须把完整 main band 排除，否则自己的旁瓣会被当 spur。

## 4 比赛里什么时候用？

DDS/DAC/放大器频谱纯净度、找最大非目标谱线。

## 5 输入

非负 magnitude、主 band、分析 band。

## 6 输出

main/spur bin、magnitude、ratio、sfdr_db。

## 7 API怎么调用

`SignalSFDR_Process(m,bins,&cfg,&r);`

## 8 参数怎么改

main band 覆盖窗主瓣；analysis start 常排除 DC，end 按有效带宽。

## 9 参数改大会怎样

main band 太窄把泄漏当 spur，SFDR虚低；太宽可能吞掉真实近端 spur，SFDR虚高。

## 10 这个算法的代价是什么

Benefits：简单、报告最大 spur。Trade-offs：不积分 spur 能量，依赖窗和范围。

## 11 什么时候不要用

需要总噪声 SNR、多个 spur 总和或主频未知时直接套用。

## 12 怎么和前一个模块接

`Window -> FFT -> Magnitude -> SFDR`

## 13 怎么和后一个模块接

`sfdr_db + spur_bin*Fs/N -> report`。

## 14 最小Demo

```c
signal_sfdr_result_t r;
(void)SignalSFDR_Process(m,bins,&cfg,&r);
```

## 15 PC测试

主峰10、最大spur2，Expected/Measured 13.9794006 dB，PASS。

## 16 MCU资源

O(analysis bins)，O(1)，一次 log10f。

## 17 验证状态

PC_VERIFIED；未实板仪器对比。

## 17. 统一 API 教程（已按当前头文件核对）

本节由当前公开头文件、实现中实际出现的状态码和正式模块注册表生成。它补充前文的场景教程；函数签名变化时必须重新运行 `tools/upgrade_formal_beginner_docs.ps1`，不要手工保留旧 API。

遵循仓库的 [Beginner README 标准](../../00_docs/BEGINNER_README_STANDARD.md)：先用最小示例完成一次正常数据流，再按需要阅读全功能示例和本节 API 细节。

### 什么时候用 / 什么时候不要用

当题目需要“sfdr”目录对应的公开功能，并且输入数据、单位和硬件资源满足前文约束时使用本模块。若只需要更简单的上游功能、输入尚未准备好，或需要不同的数据模型/外设资源，应先选择相邻模块而不是强行调用本 API。

### 输入 / 输出

输入由各 API 的只读数组、配置、频率/阈值和平台对象组成；输出写入 result/output/buffer 参数或由 Get API 返回。调用者负责数组容量、生命周期和物理单位；失败返回时输出不是有效结果。

### 调用顺序

```text
SignalSFDR_Process
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

### `signal_algorithm_status_t SignalSFDR_Process(const float *magnitude, uint32_t bin_count, const signal_sfdr_config_t *config, signal_sfdr_result_t *result);`

**它做什么：** 计算主信号峰与分析范围内最大非主 band 杂散峰之比。

**什么时候调用：** 对调用者提供的数据执行一次同步计算或生成，并在成功后写入输出对象/数组。

| 参数 | 类型 | 初学者解释 |
|---|---|---|
| `magnitude` | `const float *` | 非负线性 magnitude。 |
| `bin_count` | `uint32_t` | 数组长度。 |
| `config` | `const signal_sfdr_config_t *` | 主 band 和分析 band。 |
| `result` | `signal_sfdr_result_t *` | 输出主峰/杂散索引、幅值、比值与 dB。 |

**返回：** 成功返回 SIGNAL_ALGORITHM_OK；范围或零峰返回错误。

**最小调用形状：** `SignalSFDR_Process(...);`。可直接从 README_MINIMAL_EXAMPLE.c 复制正常流程；README_FULL_EXAMPLE.c 展示全部公开 API，其中取消类 API 会以 #if 0 隔离。

**注意：** 所有指针和数组都由调用者拥有；先检查返回值。异步采集、DMA 或回调还在使用 buffer 时，不能读取结果或改写该 buffer。

### 示例、模块链与验收

- 最小入门：`README_MINIMAL_EXAMPLE.c`，只保留正常入口和结果读取。
- 全功能：`README_FULL_EXAMPLE.c`，以正确顺序展示当前头文件全部公开 API；`Stop` 等非常规路径不会默认执行。
- 模块链：先由上游提供单位、采样率和有效数据，再调用本模块；成功后将输出交给显示、控制、测量或下一步 DSP。硬件资源仍以 SysConfig 合约为唯一来源。
- 文档验收：README/API、两份示例和头文件会由 `tools/validate_beginner_documentation.ps1` 覆盖检查；这只表示文档与源码签名一致，不代替未进行的实板验证。

### 模块链

`上游采集/配置 -> 本模块 -> 检查返回值和结果 -> 测量、显示、控制或下一步 DSP`。只有确认本模块的输出单位和有效状态后，才交给下一模块。

