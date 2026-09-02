# Robust RMS：先限住极端点再算 RMS

> **LEVEL C / REAL ALGORITHM MODULE：** 分位数阈值、winsorize、处理计数和 RMS 物理含义必须可追溯；它会主动改变尾部，不能隐藏在普通 RMS 中。

**比赛复制清单：** `signal_robust_rms.c`、`signal_robust_rms.h`、`05_precision/robust_peak_to_peak/signal_robust_peak_to_peak.c`、`signal_robust_peak_to_peak.h`、`03_measurement/common/signal_algorithm_status.h`。需要 N 个 float workspace；无 SysConfig/Pin。

## 1 这个算法是干什么的？

先把低于/高于指定分位数的值夹到边界（Winsorize），再计算总 RMS 或去均值后的交流 RMS。

## 2 一个最简单的例子

`-1,-1,-1,0,1,1,100` 把尾部夹到 [-1,1] 后变成 `-1,-1,-1,0,1,1,1`；去 DC 后 RMS 为 `sqrt(6/7)`。

## 3 原理

RMS 会平方，所以一个 100 值的影响是普通 1 值的一万倍。限幅能控制少量极端值的影响，但它也真实改变了数据。

## 4 比赛里什么时候用？

确定异常点来自 ADC 错码或毛刺，且需要稳定能量/有效值估计时。

## 5 输入

float V、count、上下分位数、`remove_dc` 标志、N-float workspace。

## 6 输出

限幅上下界 V、限幅后的均值 V、RMS V，以及被限幅点数。

## 7 API怎么调用

```c
signal_robust_rms_config_t c={0.01f,0.99f,1U};
float work[N]; signal_robust_rms_result_t r;
SignalRobustRMS_Process(x,N,&c,work,N,&r);
```

## 8 参数怎么改

`remove_dc=0` 测总 RMS，`1` 测稳健交流 RMS；分位数从 1%/99% 起根据已知异常率调整。

## 9 参数改大会怎样

边界向中间收紧：毛刺影响更小，但真实波峰和 RMS 被压低；边界向两端放宽：偏差更小但鲁棒性下降。

## 10 这个算法的代价是什么

Benefits：抑制极少数平方爆炸的异常点，并报告处理数量。Trade-offs：非线性改变波形，RMS 不再是原始信号严格能量，RAM 4N。

## 11 什么时候不要用

burst、冲击、窄脉冲、削顶检测、故障能量或尖峰本身是目标时不要用；正式结果应保留原始 RMS 对照。

## 12 怎么和前一个模块接

`ADC_ToVoltage -> RobustRMS`

## 13 怎么和后一个模块接

`RobustRMS -> Display`，同时记录 `winsorized_count` 作为诊断。

## 14 最小Demo

见第 2/7 节；工作区由调用者静态分配。

## 15 PC测试

七点真值验证上下界、均值、RMS 与处理计数，6 项 PASS。

## 16 MCU资源

平均 O(N) 分位选择 + O(N) RMS；workspace 4N 字节，无 malloc，不在 ISR 中调用。

## 17 验证状态

PC_VERIFIED；未 BOARD_VERIFIED。异常计数很大通常意味着参数过紧或信号本身不是稳态波形。

## 17. 统一 API 教程（已按当前头文件核对）

本节由当前公开头文件、实现中实际出现的状态码和正式模块注册表生成。它补充前文的场景教程；函数签名变化时必须重新运行 `tools/upgrade_formal_beginner_docs.ps1`，不要手工保留旧 API。

遵循仓库的 [Beginner README 标准](../../00_docs/BEGINNER_README_STANDARD.md)：先用最小示例完成一次正常数据流，再按需要阅读全功能示例和本节 API 细节。

### 什么时候用 / 什么时候不要用

当题目需要“robust_rms”目录对应的公开功能，并且输入数据、单位和硬件资源满足前文约束时使用本模块。若只需要更简单的上游功能、输入尚未准备好，或需要不同的数据模型/外设资源，应先选择相邻模块而不是强行调用本 API。

### 输入 / 输出

输入由各 API 的只读数组、配置、频率/阈值和平台对象组成；输出写入 result/output/buffer 参数或由 Get API 返回。调用者负责数组容量、生命周期和物理单位；失败返回时输出不是有效结果。

### 调用顺序

```text
SignalRobustRMS_Process
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

### `signal_algorithm_status_t SignalRobustRMS_Process(const float *voltage_v, uint32_t count, const signal_robust_rms_config_t *config, float *workspace, uint32_t workspace_count, signal_robust_rms_result_t *result);`

**它做什么：** 先按分位数把极端点钳到边界（Winsorize），再计算总/交流 RMS。

**什么时候调用：** 对调用者提供的数据执行一次同步计算或生成，并在成功后写入输出对象/数组。

| 参数 | 类型 | 初学者解释 |
|---|---|---|
| `voltage_v` | `const float *` | 输入电压，V。 |
| `count` | `uint32_t` | 点数。 |
| `config` | `const signal_robust_rms_config_t *` | 分位数和 remove_dc 标志。 |
| `workspace` | `float *` | float 工作区，容量至少 count。 |
| `workspace_count` | `uint32_t` | 工作区容量。 |
| `result` | `signal_robust_rms_result_t *` | 输出边界、winsorized mean、RMS 和钳位点数。 |

**返回：** 成功返回 SIGNAL_ALGORITHM_OK。

**最小调用形状：** `SignalRobustRMS_Process(...);`。可直接从 README_MINIMAL_EXAMPLE.c 复制正常流程；README_FULL_EXAMPLE.c 展示全部公开 API，其中取消类 API 会以 #if 0 隔离。

**注意：** 所有指针和数组都由调用者拥有；先检查返回值。异步采集、DMA 或回调还在使用 buffer 时，不能读取结果或改写该 buffer。

### 示例、模块链与验收

- 最小入门：`README_MINIMAL_EXAMPLE.c`，只保留正常入口和结果读取。
- 全功能：`README_FULL_EXAMPLE.c`，以正确顺序展示当前头文件全部公开 API；`Stop` 等非常规路径不会默认执行。
- 模块链：先由上游提供单位、采样率和有效数据，再调用本模块；成功后将输出交给显示、控制、测量或下一步 DSP。硬件资源仍以 SysConfig 合约为唯一来源。
- 文档验收：README/API、两份示例和头文件会由 `tools/validate_beginner_documentation.ps1` 覆盖检查；这只表示文档与源码签名一致，不代替未进行的实板验证。

### 模块链

`上游采集/配置 -> 本模块 -> 检查返回值和结果 -> 测量、显示、控制或下一步 DSP`。只有确认本模块的输出单位和有效状态后，才交给下一模块。

