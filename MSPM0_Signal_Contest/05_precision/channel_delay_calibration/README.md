# Channel Delay Calibration：补偿双通道固定延迟

> **LEVEL C / REAL ALGORITHM MODULE：** sample delay、秒、弧度、角度、频率和环绕符号同时存在，必须统一约定。

**比赛复制清单：** `signal_channel_delay_calibration.c`、`signal_channel_delay_calibration.h`、`03_measurement/common/signal_algorithm_status.h`。先用已知同相信号 `Compute` 固定通道 delay，再把补偿用于目标频率相位；无 SysConfig/Pin。

## 1 这个算法是干什么的？

把双通道前端、采样时刻不同造成的固定时间差换算成相位并补偿。

## 2 一个最简单的例子

10 kHz 同相信号测得 B-A=-36°，说明 B 晚约 10 us；在 20 kHz 时同一延迟会造成 -72°，补偿后回到 0°。

## 3 原理

延迟的相位为 `phase_deg = -360 × frequency_hz × delay_s`。算法先由已知相位真值反求 delay，再按当前频率加回相位。

## 4 比赛里什么时候用？

双 ADC/双模拟通道测相位，且延迟近似固定、已经用同相信号完成校准时。

## 5 输入

实测与期望 `phase_b_minus_a_deg`、`frequency_hz`；约定正 delay 表示 B 更晚。

## 6 输出

`delay_b_relative_to_a_s`（s）和补偿后的 B-A 相位（deg，[-180,180)）。

## 7 API怎么调用

```c
signal_channel_delay_calibration_t cal;
float phase_deg;
SignalChannelDelayCalibration_Compute(-36,0,10000,&cal);
SignalChannelDelayCalibration_Apply(-72,20000,&cal,&phase_deg);
```

## 8 参数怎么改

改变校准频率和已知期望相位；应选信噪比高、相位测量稳定且接近比赛频段的校准点。

## 9 参数改大会怎样

频率越高，同一时间误差对应更大相位，灵敏度提高，但相位 wrap 歧义也更容易出现。

## 10 这个算法的代价是什么

Benefits：一个 delay 参数可随频率换算。Trade-offs：假设延迟固定，无法描述模拟滤波器随频率变化的相位响应。

## 11 什么时候不要用

两个通道不同步漂移、相位误差超过半周期而整周数未知，或前端群时延明显随频率变化时。

## 12 怎么和前一个模块接

`DualADC -> Phase Method -> Channel Delay Calibration`

## 13 怎么和后一个模块接

`Corrected phase_deg -> Display / Control / Result`

## 14 最小Demo

见第 7 节；注意传入当前真实频率，不是固定采样率。

## 15 PC测试

10 kHz/-36°反求 10 us，再对 20 kHz/-72°补偿，4 项 PASS。

## 16 MCU资源

O(1) 时间/RAM；包含浮点除法，建议不在 ISR 中调用。

## 17 验证状态

PC_VERIFIED；未 BOARD_VERIFIED。错误优先检查 B-A 符号、Hz/s 单位与相位 wrap。

## 17. 统一 API 教程（已按当前头文件核对）

本节由当前公开头文件、实现中实际出现的状态码和正式模块注册表生成。它补充前文的场景教程；函数签名变化时必须重新运行 `tools/upgrade_formal_beginner_docs.ps1`，不要手工保留旧 API。

遵循仓库的 [Beginner README 标准](../../00_docs/BEGINNER_README_STANDARD.md)：先用最小示例完成一次正常数据流，再按需要阅读全功能示例和本节 API 细节。

### 什么时候用 / 什么时候不要用

当题目需要“channel_delay_calibration”目录对应的公开功能，并且输入数据、单位和硬件资源满足前文约束时使用本模块。若只需要更简单的上游功能、输入尚未准备好，或需要不同的数据模型/外设资源，应先选择相邻模块而不是强行调用本 API。

### 输入 / 输出

输入由各 API 的只读数组、配置、频率/阈值和平台对象组成；输出写入 result/output/buffer 参数或由 Get API 返回。调用者负责数组容量、生命周期和物理单位；失败返回时输出不是有效结果。

### 调用顺序

```text
SignalChannelDelayCalibration_Apply -> SignalChannelDelayCalibration_Compute
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

### `signal_algorithm_status_t SignalChannelDelayCalibration_Compute(float measured_phase_b_minus_a_deg, float expected_phase_b_minus_a_deg, float frequency_hz, signal_channel_delay_calibration_t *calibration);`

**它做什么：** 用已知同频相位真值估计 B 相对 A 的固定时间延迟。

**什么时候调用：** 执行该模块公开的功能；具体数据流以本节参数表和本 README 前面的场景说明为准。

| 参数 | 类型 | 初学者解释 |
|---|---|---|
| `measured_phase_b_minus_a_deg` | `float` | 实测 B-A 相位，deg。 |
| `expected_phase_b_minus_a_deg` | `float` | 参考真值，deg。 |
| `frequency_hz` | `float` | 校准频率，Hz。 |
| `calibration` | `signal_channel_delay_calibration_t *` | 输出 B 相对 A 延迟，正值表示 B 更晚，单位 s。 |

**返回：** 成功返回 SIGNAL_ALGORITHM_OK。

**最小调用形状：** `SignalChannelDelayCalibration_Compute(...);`。可直接从 README_MINIMAL_EXAMPLE.c 复制正常流程；README_FULL_EXAMPLE.c 展示全部公开 API，其中取消类 API 会以 #if 0 隔离。

**注意：** 所有指针和数组都由调用者拥有；先检查返回值。异步采集、DMA 或回调还在使用 buffer 时，不能读取结果或改写该 buffer。

### `signal_algorithm_status_t SignalChannelDelayCalibration_Apply(float measured_phase_b_minus_a_deg, float frequency_hz, const signal_channel_delay_calibration_t *calibration, float *corrected_phase_b_minus_a_deg);`

**它做什么：** 从实测 B-A 相位中补偿固定通道延迟。

**什么时候调用：** 对调用者提供的数据执行一次同步计算或生成，并在成功后写入输出对象/数组。

| 参数 | 类型 | 初学者解释 |
|---|---|---|
| `measured_phase_b_minus_a_deg` | `float` | 实测相位，deg。 |
| `frequency_hz` | `float` | 当前频率，Hz。 |
| `calibration` | `const signal_channel_delay_calibration_t *` | 固定 delay，s。 |
| `corrected_phase_b_minus_a_deg` | `float *` | 输出补偿相位，范围 [-180,180)。 |

**返回：** 成功返回 SIGNAL_ALGORITHM_OK。

**最小调用形状：** `SignalChannelDelayCalibration_Apply(...);`。可直接从 README_MINIMAL_EXAMPLE.c 复制正常流程；README_FULL_EXAMPLE.c 展示全部公开 API，其中取消类 API 会以 #if 0 隔离。

**注意：** 所有指针和数组都由调用者拥有；先检查返回值。异步采集、DMA 或回调还在使用 buffer 时，不能读取结果或改写该 buffer。

### 示例、模块链与验收

- 最小入门：`README_MINIMAL_EXAMPLE.c`，只保留正常入口和结果读取。
- 全功能：`README_FULL_EXAMPLE.c`，以正确顺序展示当前头文件全部公开 API；`Stop` 等非常规路径不会默认执行。
- 模块链：先由上游提供单位、采样率和有效数据，再调用本模块；成功后将输出交给显示、控制、测量或下一步 DSP。硬件资源仍以 SysConfig 合约为唯一来源。
- 文档验收：README/API、两份示例和头文件会由 `tools/validate_beginner_documentation.ps1` 覆盖检查；这只表示文档与源码签名一致，不代替未进行的实板验证。

### 模块链

`上游采集/配置 -> 本模块 -> 检查返回值和结果 -> 测量、显示、控制或下一步 DSP`。只有确认本模块的输出单位和有效状态后，才交给下一模块。

