# Sine Fit 3-Param：已知频率时拟合幅值、相位和 DC

> **LEVEL C / REAL ALGORITHM MODULE：** 最小二乘方程、矩阵退化、相位约定和 residual 判断不适合比赛现场重写。

**比赛复制清单：** `signal_sine_fit_3param.c`、`signal_sine_fit_3param.h`、`03_measurement/common/signal_algorithm_status.h`。必须传可靠的已知/粗估 frequency 和真实 Fs；无 SysConfig/Pin。

## 1 这个算法是干什么的？

已知正弦频率后，一次拟合出峰值幅度、相位、直流偏置和残差。

## 2 一个最简单的例子

若数据来自 `1.2 + 0.4*cos(2πfn/Fs-20°)`，输入正确 f/Fs 后应得到幅值 0.4 V、相位 -20°、DC 1.2 V。

## 3 原理

把正弦写成线性模型 `C*cos(wn)+S*sin(wn)+DC`，用最小二乘解 3×3 方程；`A=hypot(C,S)`，cos 模型相位 `atan2(-S,C)`。平方误差最小，所以能利用整帧数据抵抗随机噪声。

## 4 比赛里什么时候用？

频率已由捕获/FFT/过零准确获得，而且被测信号主要是单一正弦，需要精确幅相/DC 时。

## 5 输入

`float voltage_v[]`（V）、count≥3、`frequency_hz`、`sample_rate_hz`。

## 6 输出

C/S/DC/峰值幅度（V）、phase_rad、phase_deg、residual_rms_v。

## 7 API怎么调用

```c
signal_sine_fit_3param_config_t c={1000.0f,100000.0f};
signal_sine_fit_3param_result_t r;
SignalSineFit3Param_Process(v,N,&c,&r);
```

## 8 参数怎么改

只改真实采样率和已知频率。频率估计误差会直接变成幅相偏差和较大 residual。

## 9 参数改大会怎样

count 增大通常降低随机噪声，但 CPU 增加、观测时间变长，信号在帧内漂移时模型反而变差。

## 10 这个算法的代价是什么

Benefits：非相干采样也能直接估幅相/DC，残差可诊断模型质量。Trade-offs：需要准确频率，含 sin/cos 和浮点 3×3 求解；不是通用失真波形模型。

## 11 什么时候不要用

方波、严重谐波、多音、频率快速漂移、削顶或 frequency_hz 不可靠时不要把结果当真；先看 residual。

## 12 怎么和前一个模块接

`ADC_ToVoltage -> Frequency Estimate -> SineFit3`

## 13 怎么和后一个模块接

`Amplitude/Phase/DC/Residual -> Calibration / Result`

## 14 最小Demo

见第 7 节；不需要工作区，不修改输入。

## 15 PC测试

1000 点、Fs=100 kHz、f=1234.5 Hz、A=0.4 V、phase=-20°、DC=1.2 V 的真值测试全部 PASS。

## 16 MCU资源

O(N)，RAM O(1)，两遍扫描；M0+ 软件浮点上比均值/RMS 慢，放在帧处理任务而非 ISR。

## 17 验证状态

PC_VERIFIED；仅合成正弦真值，未 BOARD_VERIFIED。排查首先比较 residual 和输入频率误差。

## 17. 统一 API 教程（已按当前头文件核对）

本节由当前公开头文件、实现中实际出现的状态码和正式模块注册表生成。它补充前文的场景教程；函数签名变化时必须重新运行 `tools/upgrade_formal_beginner_docs.ps1`，不要手工保留旧 API。

遵循仓库的 [Beginner README 标准](../../00_docs/BEGINNER_README_STANDARD.md)：先用最小示例完成一次正常数据流，再按需要阅读全功能示例和本节 API 细节。

### 什么时候用 / 什么时候不要用

当题目需要“sine_fit_3param”目录对应的公开功能，并且输入数据、单位和硬件资源满足前文约束时使用本模块。若只需要更简单的上游功能、输入尚未准备好，或需要不同的数据模型/外设资源，应先选择相邻模块而不是强行调用本 API。

### 输入 / 输出

输入由各 API 的只读数组、配置、频率/阈值和平台对象组成；输出写入 result/output/buffer 参数或由 Get API 返回。调用者负责数组容量、生命周期和物理单位；失败返回时输出不是有效结果。

### 调用顺序

```text
SignalSineFit3Param_Process
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

### `signal_algorithm_status_t SignalSineFit3Param_Process(const float *voltage_v, uint32_t count, const signal_sine_fit_3param_config_t *config, signal_sine_fit_3param_result_t *result);`

**它做什么：** 在已知频率下最小二乘拟合 `x=Ccos(w n)+Ssin(w n)+DC`。

**什么时候调用：** 对调用者提供的数据执行一次同步计算或生成，并在成功后写入输出对象/数组。

| 参数 | 类型 | 初学者解释 |
|---|---|---|
| `voltage_v` | `const float *` | 输入电压，V。 |
| `count` | `uint32_t` | 点数，至少 3。 |
| `config` | `const signal_sine_fit_3param_config_t *` | 已知频率和采样率，Hz。 |
| `result` | `signal_sine_fit_3param_result_t *` | 输出系数、DC、峰值、cos 模型相位和残差 RMS。 |

**返回：** 成功返回 SIGNAL_ALGORITHM_OK；矩阵奇异或参数非法返回错误。

**最小调用形状：** `SignalSineFit3Param_Process(...);`。可直接从 README_MINIMAL_EXAMPLE.c 复制正常流程；README_FULL_EXAMPLE.c 展示全部公开 API，其中取消类 API 会以 #if 0 隔离。

**注意：** 所有指针和数组都由调用者拥有；先检查返回值。异步采集、DMA 或回调还在使用 buffer 时，不能读取结果或改写该 buffer。

### 示例、模块链与验收

- 最小入门：`README_MINIMAL_EXAMPLE.c`，只保留正常入口和结果读取。
- 全功能：`README_FULL_EXAMPLE.c`，以正确顺序展示当前头文件全部公开 API；`Stop` 等非常规路径不会默认执行。
- 模块链：先由上游提供单位、采样率和有效数据，再调用本模块；成功后将输出交给显示、控制、测量或下一步 DSP。硬件资源仍以 SysConfig 合约为唯一来源。
- 文档验收：README/API、两份示例和头文件会由 `tools/validate_beginner_documentation.ps1` 覆盖检查；这只表示文档与源码签名一致，不代替未进行的实板验证。

### 模块链

`上游采集/配置 -> 本模块 -> 检查返回值和结果 -> 测量、显示、控制或下一步 DSP`。只有确认本模块的输出单位和有效状态后，才交给下一模块。

