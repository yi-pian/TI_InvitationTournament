# ADC Gain/Offset Calibration：修正电压刻度

> **LEVEL C / REAL ALGORITHM MODULE：** “只加一个固定 offset”属于 Recipe；从两个可靠参考点求 gain/offset 并应用、检查退化跨度属于正式校准模块。

**比赛复制清单：** `signal_adc_gain_offset_calibration.c`、`signal_adc_gain_offset_calibration.h`、`03_measurement/common/signal_algorithm_status.h`。先 `Compute` 一次得到校准参数，再对每帧 `Apply`；无 SysConfig/Pin。

## 1 这个算法是干什么的？

用两个已知标准电压，求出 `校准值 = gain × 测量值 + offset`，修正稳定的比例误差和零点误差。

## 2 一个最简单的例子

仪器把真实 0 V 测成 0.1 V，把真实 3.3 V 测成 3.2 V。两点校准后，输入 0.1/3.2 V 会输出 0/3.3 V。

## 3 原理

在“测量值—真值”平面上用两个点确定一条直线。gain 修正斜率，offset 修正整体平移。

## 4 比赛里什么时候用？

电压基准、分压电阻、运放增益或 ADC 偏置造成稳定系统误差时；应先预热并用可信万用表/基准源取得真值。

## 5 输入

两个低/高测量电压和对应真值，全部为 V；应用阶段输入 `float voltage_v[]`。

## 6 输出

无量纲 `gain`、单位 V 的 `offset_v`，以及校准后的 V 数组。

## 7 API怎么调用

```c
signal_adc_gain_offset_calibration_t cal;
SignalADCGainOffsetCalibration_Compute(0.1f,0.0f,3.2f,3.3f,&cal);
SignalADCGainOffsetCalibration_Apply(in_v,out_v,count,&cal);
```

## 8 参数怎么改

没有经验参数。改变的是两组校准点；应覆盖比赛实际测量范围，且两点不要太接近。

## 9 参数改大会怎样

高低点间隔越大，参考源小误差对 gain 的放大越小；超出两点范围属于外推，误差可能增加。

## 10 这个算法的代价是什么

Benefits：用 O(N) 很低代价修正一阶系统误差。Trade-offs：依赖参考源准确度，不能修正非线性、温漂和频率响应。

## 11 什么时候不要用

校准点不可信、前端正在削顶、误差随频率/量程变化，或把某次噪声误当固定 offset 时不要盲用。

## 12 怎么和前一个模块接

`ADC_ToVoltage -> ADC Gain/Offset Calibration`

## 13 怎么和后一个模块接

`Calibration -> Mean / Vpp / RMS / FFT`

## 14 最小Demo

同第 7 节；`in_v` 与 `out_v` 可为同一数组。

## 15 PC测试

已用 0.1→0、3.2→3.3 两点真值测试，比较 gain、offset 和端点输出，6 项 PASS。

## 16 MCU资源

计算参数 O(1)；应用 O(N)，RAM O(1)，允许原地处理，无 malloc。

## 17 验证状态

PC_VERIFIED（2026-08-07）；未 BOARD_VERIFIED。排查先检查单位是否全为 V、测量/真值参数有没有写反。

## 17. 统一 API 教程（已按当前头文件核对）

本节由当前公开头文件、实现中实际出现的状态码和正式模块注册表生成。它补充前文的场景教程；函数签名变化时必须重新运行 `tools/upgrade_formal_beginner_docs.ps1`，不要手工保留旧 API。

遵循仓库的 [Beginner README 标准](../../00_docs/BEGINNER_README_STANDARD.md)：先用最小示例完成一次正常数据流，再按需要阅读全功能示例和本节 API 细节。

### 什么时候用 / 什么时候不要用

当题目需要“adc_gain_offset_calibration”目录对应的公开功能，并且输入数据、单位和硬件资源满足前文约束时使用本模块。若只需要更简单的上游功能、输入尚未准备好，或需要不同的数据模型/外设资源，应先选择相邻模块而不是强行调用本 API。

### 输入 / 输出

输入由各 API 的只读数组、配置、频率/阈值和平台对象组成；输出写入 result/output/buffer 参数或由 Get API 返回。调用者负责数组容量、生命周期和物理单位；失败返回时输出不是有效结果。

### 调用顺序

```text
SignalADCGainOffsetCalibration_Apply -> SignalADCGainOffsetCalibration_Compute
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

### `signal_algorithm_status_t SignalADCGainOffsetCalibration_Compute(float measured_low_v, float true_low_v, float measured_high_v, float true_high_v, signal_adc_gain_offset_calibration_t *calibration);`

**它做什么：** 从两个已知真值点计算 `corrected = gainmeasured + offset`。

**什么时候调用：** 执行该模块公开的功能；具体数据流以本节参数表和本 README 前面的场景说明为准。

| 参数 | 类型 | 初学者解释 |
|---|---|---|
| `measured_low_v` | `float` | 低点测量值，V。 |
| `true_low_v` | `float` | 低点参考真值，V。 |
| `measured_high_v` | `float` | 高点测量值，V，必须与低点不同。 |
| `true_high_v` | `float` | 高点参考真值，V。 |
| `calibration` | `signal_adc_gain_offset_calibration_t *` | 输出无量纲 gain 和 V offset。 |

**返回：** 成功返回 SIGNAL_ALGORITHM_OK。

**最小调用形状：** `SignalADCGainOffsetCalibration_Compute(...);`。可直接从 README_MINIMAL_EXAMPLE.c 复制正常流程；README_FULL_EXAMPLE.c 展示全部公开 API，其中取消类 API 会以 #if 0 隔离。

**注意：** 所有指针和数组都由调用者拥有；先检查返回值。异步采集、DMA 或回调还在使用 buffer 时，不能读取结果或改写该 buffer。

### `signal_algorithm_status_t SignalADCGainOffsetCalibration_Apply(const float *input_voltage_v, float *output_voltage_v, uint32_t count, const signal_adc_gain_offset_calibration_t *calibration);`

**它做什么：** 对电压数组应用增益/零偏校准，允许原地处理。

**什么时候调用：** 对调用者提供的数据执行一次同步计算或生成，并在成功后写入输出对象/数组。

| 参数 | 类型 | 初学者解释 |
|---|---|---|
| `input_voltage_v` | `const float *` | 输入测量电压，V。 |
| `output_voltage_v` | `float *` | 输出校准电压，V。 |
| `count` | `uint32_t` | 点数。 |
| `calibration` | `const signal_adc_gain_offset_calibration_t *` | 已计算校准参数。 |

**返回：** 成功返回 SIGNAL_ALGORITHM_OK。

**最小调用形状：** `SignalADCGainOffsetCalibration_Apply(...);`。可直接从 README_MINIMAL_EXAMPLE.c 复制正常流程；README_FULL_EXAMPLE.c 展示全部公开 API，其中取消类 API 会以 #if 0 隔离。

**注意：** 所有指针和数组都由调用者拥有；先检查返回值。异步采集、DMA 或回调还在使用 buffer 时，不能读取结果或改写该 buffer。

### 示例、模块链与验收

- 最小入门：`README_MINIMAL_EXAMPLE.c`，只保留正常入口和结果读取。
- 全功能：`README_FULL_EXAMPLE.c`，以正确顺序展示当前头文件全部公开 API；`Stop` 等非常规路径不会默认执行。
- 模块链：先由上游提供单位、采样率和有效数据，再调用本模块；成功后将输出交给显示、控制、测量或下一步 DSP。硬件资源仍以 SysConfig 合约为唯一来源。
- 文档验收：README/API、两份示例和头文件会由 `tools/validate_beginner_documentation.ps1` 覆盖检查；这只表示文档与源码签名一致，不代替未进行的实板验证。

### 模块链

`上游采集/配置 -> 本模块 -> 检查返回值和结果 -> 测量、显示、控制或下一步 DSP`。只有确认本模块的输出单位和有效状态后，才交给下一模块。

