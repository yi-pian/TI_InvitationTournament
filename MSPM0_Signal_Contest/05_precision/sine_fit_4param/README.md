# Sine Fit 4-Param：窄频带搜索频率再拟合正弦

> **LEVEL C / REAL ALGORITHM MODULE：** 它是带初值和搜索宽度的局部优化，不是全频搜索；迭代、退化和 residual 风险必须保留在模块内。

**比赛复制清单：** `signal_sine_fit_4param.c`、`signal_sine_fit_4param.h`、`05_precision/sine_fit_3param/signal_sine_fit_3param.c`、`signal_sine_fit_3param.h`、`03_measurement/common/signal_algorithm_status.h`。无 SysConfig/Pin；先用 FFT/过零获得窄初值。

## 1 这个算法是干什么的？

在用户给定的小频率范围内寻找残差最小的频率，同时给出幅值、相位和 DC。

## 2 一个最简单的例子

真值 1234.5 Hz，先验 1220±80 Hz；搜索后得到约 1234.5 Hz，再用 3 参数拟合获得 0.4 V、-20°、1.2 V。

## 3 原理

每个候选频率调用 3 参数最小二乘，得到 residual RMS；用黄金分割缩小残差最低的窄区间。这是“有先验的局部搜索”，不是全频谱搜索。

## 4 比赛里什么时候用？

已知是单一正弦，粗频率已经由 FFT/过零得到，且希望进一步联合优化频率和幅相时。

## 5 输入

float V、count≥4、`initial_frequency_hz`、`search_half_width_hz`、Fs、6~40 次迭代。

## 6 输出

frequency_hz、实际迭代次数，以及 3 参数拟合的幅值/相位/DC/residual。

## 7 API怎么调用

```c
signal_sine_fit_4param_config_t c={1220,80,100000,28};
signal_sine_fit_4param_result_t r;
SignalSineFit4Param_Process(v,N,&c,&r);
```

## 8 参数怎么改

先用可靠粗测设 initial；half width 只覆盖粗测不确定度；20~30 次通常足够，必须用 PC 真值和现场数据确认。

## 9 参数改大会怎样

half width 大：不容易漏真值，但可能跨多个局部极小/频谱主瓣而选错；迭代多：频率网格更细但 CPU 线性增加，浮点精度最终会限制收益。

## 10 这个算法的代价是什么

Benefits：无需 FFT bin 限制，能联合得到频率与幅相/DC。Trade-offs：约 O(iterations×N)，M0+ 较慢，依赖单音和单峰先验。

## 11 什么时候不要用

没有可靠初值、范围内有多音/多个极小值、信号短且严重失真，或需要实时每采样点更新时不要现场冒险使用。

## 12 怎么和前一个模块接

`FFT/ZeroCross coarse frequency -> SineFit4 narrow search`

## 13 怎么和后一个模块接

`SineFit4 -> Frequency/Amplitude/Phase Result`

## 14 最小Demo

见第 7 节。先检查返回值和 `waveform.residual_rms_v`。

## 15 PC测试

1000 点无噪声合成正弦，1220±80 Hz/28 次搜索恢复 1234.5 Hz，频率及幅相/DC 共 5 项 PASS。

## 16 MCU资源

O(I×N)，RAM O(1)；每次候选含三角函数初始化、两遍浮点累加。建议低速结果任务中调用并限制迭代次数。

## 17 验证状态

PC_VERIFIED（仅窄带干净单音场景）；未 BOARD_VERIFIED。多音、噪声和粗初值边界仍需按赛题验证，属于较高风险模块。

## 17. 统一 API 教程（已按当前头文件核对）

本节由当前公开头文件、实现中实际出现的状态码和正式模块注册表生成。它补充前文的场景教程；函数签名变化时必须重新运行 `tools/upgrade_formal_beginner_docs.ps1`，不要手工保留旧 API。

遵循仓库的 [Beginner README 标准](../../00_docs/BEGINNER_README_STANDARD.md)：先用最小示例完成一次正常数据流，再按需要阅读全功能示例和本节 API 细节。

### 什么时候用 / 什么时候不要用

当题目需要“sine_fit_4param”目录对应的公开功能，并且输入数据、单位和硬件资源满足前文约束时使用本模块。若只需要更简单的上游功能、输入尚未准备好，或需要不同的数据模型/外设资源，应先选择相邻模块而不是强行调用本 API。

### 输入 / 输出

输入由各 API 的只读数组、配置、频率/阈值和平台对象组成；输出写入 result/output/buffer 参数或由 Get API 返回。调用者负责数组容量、生命周期和物理单位；失败返回时输出不是有效结果。

### 调用顺序

```text
SignalSineFit4Param_Process
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

### `signal_algorithm_status_t SignalSineFit4Param_Process(const float *voltage_v, uint32_t count, const signal_sine_fit_4param_config_t *config, signal_sine_fit_4param_result_t *result);`

**它做什么：** 在用户给定窄频带内用黄金分割搜索频率，每个候选做 3 参数最小二乘。

**什么时候调用：** 对调用者提供的数据执行一次同步计算或生成，并在成功后写入输出对象/数组。

| 参数 | 类型 | 初学者解释 |
|---|---|---|
| `voltage_v` | `const float *` | 输入电压，V。 |
| `count` | `uint32_t` | 点数，至少 4。 |
| `config` | `const signal_sine_fit_4param_config_t *` | 初值、搜索半宽、Fs 和 6~40 次迭代。 |
| `result` | `signal_sine_fit_4param_result_t *` | 输出频率及对应幅值/相位/DC/残差。 |

**返回：** 成功返回 SIGNAL_ALGORITHM_OK。

**最小调用形状：** `SignalSineFit4Param_Process(...);`。可直接从 README_MINIMAL_EXAMPLE.c 复制正常流程；README_FULL_EXAMPLE.c 展示全部公开 API，其中取消类 API 会以 #if 0 隔离。

**注意：** 所有指针和数组都由调用者拥有；先检查返回值。异步采集、DMA 或回调还在使用 buffer 时，不能读取结果或改写该 buffer。

### 示例、模块链与验收

- 最小入门：`README_MINIMAL_EXAMPLE.c`，只保留正常入口和结果读取。
- 全功能：`README_FULL_EXAMPLE.c`，以正确顺序展示当前头文件全部公开 API；`Stop` 等非常规路径不会默认执行。
- 模块链：先由上游提供单位、采样率和有效数据，再调用本模块；成功后将输出交给显示、控制、测量或下一步 DSP。硬件资源仍以 SysConfig 合约为唯一来源。
- 文档验收：README/API、两份示例和头文件会由 `tools/validate_beginner_documentation.ps1` 覆盖检查；这只表示文档与源码签名一致，不代替未进行的实板验证。

### 模块链

`上游采集/配置 -> 本模块 -> 检查返回值和结果 -> 测量、显示、控制或下一步 DSP`。只有确认本模块的输出单位和有效状态后，才交给下一模块。

