# Lock-In：只听已知参考频率

> **LEVEL C / REAL ALGORITHM MODULE：** 正交参考、相位、积分归一化、DC 和残差输出需要一致，且只有参考与采样关系明确时才有意义。

**比赛复制清单：** `signal_lock_in.c`、`signal_lock_in.h`、`03_measurement/common/signal_algorithm_status.h`。Application 传真实 `sample_rate_hz`、reference frequency/phase 和整帧样本；无 SysConfig/Pin。

## 1 这个算法是干什么的？

把输入分别与同频 cosine/sine 参考相乘并平均，提取目标频率的 I/Q、幅值和相位，即使其他频率噪声较强也能聚焦目标分量。

## 2 一个最简单的例子

输入 `1.65 + 0.2*cos(2π·1000t+30°)`，参考 1000 Hz/0°，整周期积分得到约 0.2 V、30°和均值 1.65 V。

## 3 原理

同频乘积包含一个 DC 项和一个两倍频项；跨整数周期平均时，两倍频与非同步噪声趋近零，只留下 I/Q。幅值为 `hypot(I,Q)`，相位为 `atan2(Q,I)`。

## 4 比赛里什么时候用？

激励频率由本机 DDS/参考源已知，需要从低 SNR 信号中测响应幅相，例如扫频、传感器或微弱调制分量。

## 5 输入

float V、count、reference_frequency_hz、sample_rate_hz、reference_phase_rad、是否先减均值 DC。

## 6 输出

mean_voltage_v、I/Q（V）、amplitude_peak_v、phase_rad/deg；相位相对给定参考。

## 7 API怎么调用

```c
signal_lock_in_config_t c={1000,100000,0,1U};
signal_lock_in_result_t r;
SignalLockIn_Process(v,N,&c,&r);
```

## 8 参数怎么改

参考频率必须与目标一致；reference phase 用于补偿 DDS/线缆基准；count 尽量覆盖整数周期，低 SNR 时延长积分。

## 9 参数改大会怎样

count 大：等效带宽更窄、噪声更低，但响应更慢、RAM采集/CPU更多；频率或相位参考误差会使输出衰减或旋转。

## 10 这个算法的代价是什么

Benefits：对已知频率选择性强，同时输出幅相。Trade-offs：需要相干参考，有限帧非整数周期会泄漏，不能一次看完整频谱。

## 11 什么时候不要用

频率未知/快速变化、要分析所有谐波、单次瞬态，或参考与采样时钟不同步而未估计漂移时不要直接用。

## 12 怎么和前一个模块接

`ADC_ToVoltage -> [GainCalibration] -> LockIn(reference from DDS config)`

## 13 怎么和后一个模块接

`LockIn I/Q -> Amplitude/Phase -> FrequencyResponse/Control`

## 14 最小Demo

见第 7 节；本实现不保存大型状态，不使用 malloc。

## 15 PC测试

Fs=100 kHz、1000 点、1 kHz、A=0.2 V、phase=30°、DC=1.65 V 的整数周期真值测试 4 项 PASS。

## 16 MCU资源

O(N)，RAM O(1)；每帧两次扫描、递推参考。M0+ 软件浮点建议帧外运行；更高性能版本可预计算参考表但会增加 RAM/Flash。

## 17 验证状态

PC_VERIFIED（整数周期、干净参考）；未 BOARD_VERIFIED。实板排查重点是参考相位定义、时钟同步、非整数周期和前端延迟。

## 17. 统一 API 教程（已按当前头文件核对）

本节由当前公开头文件、实现中实际出现的状态码和正式模块注册表生成。它补充前文的场景教程；函数签名变化时必须重新运行 `tools/upgrade_formal_beginner_docs.ps1`，不要手工保留旧 API。

遵循仓库的 [Beginner README 标准](../../00_docs/BEGINNER_README_STANDARD.md)：先用最小示例完成一次正常数据流，再按需要阅读全功能示例和本节 API 细节。

### 什么时候用 / 什么时候不要用

当题目需要“lock_in”目录对应的公开功能，并且输入数据、单位和硬件资源满足前文约束时使用本模块。若只需要更简单的上游功能、输入尚未准备好，或需要不同的数据模型/外设资源，应先选择相邻模块而不是强行调用本 API。

### 输入 / 输出

输入由各 API 的只读数组、配置、频率/阈值和平台对象组成；输出写入 result/output/buffer 参数或由 Get API 返回。调用者负责数组容量、生命周期和物理单位；失败返回时输出不是有效结果。

### 调用顺序

```text
SignalLockIn_Process
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

### `signal_algorithm_status_t SignalLockIn_Process(const float *voltage_v, uint32_t count, const signal_lock_in_config_t *config, signal_lock_in_result_t *result);`

**它做什么：** 与已知余弦参考同步正交积分，提取目标频率的幅值和相位。

**什么时候调用：** 对调用者提供的数据执行一次同步计算或生成，并在成功后写入输出对象/数组。

| 参数 | 类型 | 初学者解释 |
|---|---|---|
| `voltage_v` | `const float *` | 输入电压，V。 |
| `count` | `uint32_t` | 点数，至少 2；最好覆盖参考的整数周期。 |
| `config` | `const signal_lock_in_config_t *` | 参考频率/Fs/初相和是否去平均 DC。 |
| `result` | `signal_lock_in_result_t *` | 输出均值、I/Q、峰值和相对参考相位。 |

**返回：** 成功返回 SIGNAL_ALGORITHM_OK。

**最小调用形状：** `SignalLockIn_Process(...);`。可直接从 README_MINIMAL_EXAMPLE.c 复制正常流程；README_FULL_EXAMPLE.c 展示全部公开 API，其中取消类 API 会以 #if 0 隔离。

**注意：** 所有指针和数组都由调用者拥有；先检查返回值。异步采集、DMA 或回调还在使用 buffer 时，不能读取结果或改写该 buffer。

### 示例、模块链与验收

- 最小入门：`README_MINIMAL_EXAMPLE.c`，只保留正常入口和结果读取。
- 全功能：`README_FULL_EXAMPLE.c`，以正确顺序展示当前头文件全部公开 API；`Stop` 等非常规路径不会默认执行。
- 模块链：先由上游提供单位、采样率和有效数据，再调用本模块；成功后将输出交给显示、控制、测量或下一步 DSP。硬件资源仍以 SysConfig 合约为唯一来源。
- 文档验收：README/API、两份示例和头文件会由 `tools/validate_beginner_documentation.ps1` 覆盖检查；这只表示文档与源码签名一致，不代替未进行的实板验证。

### 模块链

`上游采集/配置 -> 本模块 -> 检查返回值和结果 -> 测量、显示、控制或下一步 DSP`。只有确认本模块的输出单位和有效状态后，才交给下一模块。

