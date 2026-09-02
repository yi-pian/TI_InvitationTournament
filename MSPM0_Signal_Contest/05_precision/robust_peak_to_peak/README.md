# Robust Peak-to-Peak：用分位数抵抗毛刺

> **LEVEL C / REAL ALGORITHM MODULE：** 分位数排序、插值、workspace 和“真实尖峰不能被当异常值”的边界使它必须保留为显式高级模块。

**比赛复制清单：** `signal_robust_peak_to_peak.c`、`signal_robust_peak_to_peak.h`、`03_measurement/common/signal_algorithm_status.h`。调用者提供 N 个 float workspace；原输入只读，workspace 会被重排。无 SysConfig/Pin。

## 1 这个算法是干什么的？

不用最小值和最大值，而用上下分位数之差估计稳健 Vpp，避免一个异常尖峰把结果拉得很大。

## 2 一个最简单的例子

`0,1,2,3,100` 的普通 Vpp 是 100；25%/75% 分位数为 1/3，稳健 Vpp 是 2。

## 3 原理

排序位置为 `q×(N-1)`，落在两点之间时线性插值。少量尾部样本不会决定结果，所以能抵抗极端异常值。

## 4 比赛里什么时候用？

已确认有偶发 ADC 错码/毛刺，目标是周期波形的稳定幅度估计时。

## 5 输入

`float voltage_v[]`（V）、count、`0<=lower<upper<=1`，以及调用者提供的 N 个 float workspace。

## 6 输出

上下分位电压和 `robust_vpp_v`，均为 V；workspace 会被重排，原输入只读。

## 7 API怎么调用

```c
float work[N];
signal_robust_peak_to_peak_config_t c={0.01f,0.99f};
signal_robust_peak_to_peak_result_t r;
SignalRobustPeakToPeak_Process(x,N,&c,work,N,&r);
```

## 8 参数怎么改

从 1%/99% 或 0.5%/99.5% 开始，按点数和允许丢弃的尾部比例修改。N 很小时不要用极端小百分比假装“稳健”。

## 9 参数改大会怎样

lower 增大、upper 减小会更抗毛刺，但会更多削掉真实峰谷，使 Vpp 偏小；反向修改更接近 max-min，也更怕异常值。

## 10 这个算法的代价是什么

Benefits：少量极端点不再支配结果。Trade-offs：主动舍弃尾部，不是物理最大峰峰值，需要 4N 字节 workspace。

## 11 什么时候不要用

尖峰、窄脉冲、过冲本身就是被测目标时绝对不要用；样本未覆盖足够周期也可能漏峰。

## 12 怎么和前一个模块接

`ADC_ToVoltage -> [可选 Hampel] -> RobustVPP`

## 13 怎么和后一个模块接

`robust_vpp_v -> Display / Range Decision`

## 14 最小Demo

上例设 lower=0.25、upper=0.75，输出 2 V。

## 15 PC测试

乱序 `{100,0,3,1,2}` 的上下分位与差值共 4 项 PASS。

## 16 MCU资源

平均 O(N) 选择算法；workspace 4N 字节，无 malloc。最坏情况与数据排列有关，勿在 ISR 运行。

## 17 验证状态

PC_VERIFIED；未 BOARD_VERIFIED。结果偏小先检查分位数是否过于激进和采样是否覆盖峰谷。

## 17. 统一 API 教程（已按当前头文件核对）

本节由当前公开头文件、实现中实际出现的状态码和正式模块注册表生成。它补充前文的场景教程；函数签名变化时必须重新运行 `tools/upgrade_formal_beginner_docs.ps1`，不要手工保留旧 API。

遵循仓库的 [Beginner README 标准](../../00_docs/BEGINNER_README_STANDARD.md)：先用最小示例完成一次正常数据流，再按需要阅读全功能示例和本节 API 细节。

### 什么时候用 / 什么时候不要用

当题目需要“robust_peak_to_peak”目录对应的公开功能，并且输入数据、单位和硬件资源满足前文约束时使用本模块。若只需要更简单的上游功能、输入尚未准备好，或需要不同的数据模型/外设资源，应先选择相邻模块而不是强行调用本 API。

### 输入 / 输出

输入由各 API 的只读数组、配置、频率/阈值和平台对象组成；输出写入 result/output/buffer 参数或由 Get API 返回。调用者负责数组容量、生命周期和物理单位；失败返回时输出不是有效结果。

### 调用顺序

```text
SignalRobustPeakToPeak_Process
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

### `signal_algorithm_status_t SignalRobustPeakToPeak_Process(const float *voltage_v, uint32_t count, const signal_robust_peak_to_peak_config_t *config, float *workspace, uint32_t workspace_count, signal_robust_peak_to_peak_result_t *result);`

**它做什么：** 用上下分位数差代替 max-min，降低少量极端毛刺影响。

**什么时候调用：** 对调用者提供的数据执行一次同步计算或生成，并在成功后写入输出对象/数组。

| 参数 | 类型 | 初学者解释 |
|---|---|---|
| `voltage_v` | `const float *` | 输入电压，V，只读。 |
| `count` | `uint32_t` | 点数，必须大于 0 且不超过 INT32_MAX。 |
| `config` | `const signal_robust_peak_to_peak_config_t *` | 0~1 内 lower<upper 的线性插值分位数。 |
| `workspace` | `float *` | 调用者 float 工作区，容量至少 count，会被重排。 |
| `workspace_count` | `uint32_t` | 工作区容量。 |
| `result` | `signal_robust_peak_to_peak_result_t *` | 输出上下分位电压与 robust Vpp，V。 |

**返回：** 成功返回 SIGNAL_ALGORITHM_OK。

**最小调用形状：** `SignalRobustPeakToPeak_Process(...);`。可直接从 README_MINIMAL_EXAMPLE.c 复制正常流程；README_FULL_EXAMPLE.c 展示全部公开 API，其中取消类 API 会以 #if 0 隔离。

**注意：** 所有指针和数组都由调用者拥有；先检查返回值。异步采集、DMA 或回调还在使用 buffer 时，不能读取结果或改写该 buffer。

### 示例、模块链与验收

- 最小入门：`README_MINIMAL_EXAMPLE.c`，只保留正常入口和结果读取。
- 全功能：`README_FULL_EXAMPLE.c`，以正确顺序展示当前头文件全部公开 API；`Stop` 等非常规路径不会默认执行。
- 模块链：先由上游提供单位、采样率和有效数据，再调用本模块；成功后将输出交给显示、控制、测量或下一步 DSP。硬件资源仍以 SysConfig 合约为唯一来源。
- 文档验收：README/API、两份示例和头文件会由 `tools/validate_beginner_documentation.ps1` 覆盖检查；这只表示文档与源码签名一致，不代替未进行的实板验证。

### 模块链

`上游采集/配置 -> 本模块 -> 检查返回值和结果 -> 测量、显示、控制或下一步 DSP`。只有确认本模块的输出单位和有效状态后，才交给下一模块。

