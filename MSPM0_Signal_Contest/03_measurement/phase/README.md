# Phase：统一三种相位差接口

> **LEVEL C / REAL ALGORITHM MODULE：** 三种相位来源、`B-A` 符号和 `[-180,180)` 环绕必须统一，不能用散落公式代替。

## 比赛复制版：先看这里

**适合：** 你已经从双通道得到过零位置、同一 FFT bin 或相关峰 lag，要换算 `B-A` 相位。它不采双通道，也不自己做 FFT/相关。

**复制到 `modules/`：** `signal_phase.c`、`signal_phase.h`，以及 `03_measurement/common/` 下的 `signal_algorithm_status.h`、`signal_complex.h`、`signal_math_backend.h`、`signal_math_backend_config.h`。不需要 SysConfig/Pin。

最简单的过零入口：

```c
#include "signal_phase.h"

signal_phase_result_t phase;
signal_algorithm_status_t status = SignalPhase_FromZeroCross(
    crossing_a_samples, crossing_b_samples, period_samples, &phase);
if (status == SIGNAL_ALGORITHM_OK) {
    float phase_deg = phase.phase_difference_deg;
    // ===== 这里写你自己的逻辑 =====
}
```

**输入 / 输出：** 三种入口分别接受过零 sample、两路 complex spectrum 或 correlation lag；统一输出 `phase_difference_deg/rad`，范围 `[-180,180)`，符号定义为 `B-A`。

| 已有什么 | 调哪个入口 |
|---|---|
| 两路同方向过零点和周期 | `SignalPhase_FromZeroCross` |
| 两路 FFT 和共同峰 bin | `SignalPhase_FromFFTBin` |
| 互相关得到 B 相对 A 的 lag | `SignalPhase_FromCorrelationLag` |

**Build / 最小验证：** 过零位置 A=10、B=12、period=8 可用于检查环绕与符号。隔离复制工程已 `SysConfig / Compile / Full Link PASS`，Flash 1888 B、SRAM（含栈）521 B。完整代码见 `README_MINIMAL_EXAMPLE.c`。

**连接：** `Dual ADC -> 两路同型处理 -> ZeroCross/FFT/Correlation -> Phase`。两路必须用同一 Fs/N/Window；常见错误是混用不同 bin、颠倒 A/B、把 degree/radian 混淆。

> 下文保留三种 API 的详细语义；比赛 COPY 以本节为准。

## 你真的需要这个模块吗？

**已有两路对齐数据，并且要计算 B 相对 A 的相位差时需要。** 这是 C `ALGORITHM_MODULE`，只处理内存数据。

## 你应该已经有什么输入数据

按方法准备两路过零结果、同一 FFT bin 的复数值，或相关峰 lag；两路必须来自一致时基。

## 最短接入步骤

1. **文件：** 复制本节顶部清单到 `modules/`，include `signal_phase.h`；无需另加算法仓库 Include Path。
2. **参数：** 选择 ZeroCross/FFTBin/CorrelationLag API，并提供相应 `Fs`、频率或周期参数。
3. **Workspace / Result：** 准备对应输入值和 `signal_phase_result_t result`；无硬件 workspace。
4. **调用：** `SignalPhase_FromZeroCross(...)` / `SignalPhase_FromFFTBin(...)` / `SignalPhase_FromCorrelationLag(...)`。
5. **输出：** B 相对 A 的相位差及方法相关结果。
6. **连接下一步：** 显示、扫频 gain/phase 表或通道延迟校准。
7. **Build / 最小验证：** 两路相同数组应接近 0°；已知四分之一周期延迟应接近 ±90°。

> 算法边界：不配置 Pin，不修改 SysConfig，不调用 DriverLib，也不需要 Platform Adapter。上游硬件变化时，只把真实 `Fs/N/VREF` 等事实同步到算法参数。

## 1 这个算法是干什么的？

把过零时间差、FFT 复相角差或相关 lag 换成统一的 `phase_B - phase_A`。

## 2 一个最简单的例子

周期100 sample，B 的同方向过零比 A 晚5 sample，B 滞后，所以 `B-A=-360*5/100=-18°`。

## 3 原理

时间延迟对应负相位；FFT 直接取 `atan2(B)-atan2(A)`；相关正 lag 表示 B 晚，也换成负相位。所有结果 wrap 到 [-180,180)。

## 4 比赛里什么时候用？

双通道正弦/周期波形相位差。方法选择见 `PHASE_METHOD_SELECTION.md`。

## 5 输入

ZeroCross：两路对应同方向位置+周期；FFT：同一 bin 的两路 complex；Correlation：B 相对 A 的 lag+周期。

## 6 输出

`phase_difference_deg/rad`，都表示 B-A。

## 7 API怎么调用

```c
SignalPhase_FromZeroCross(a,b,period,&r);
SignalPhase_FromFFTBin(Xa,Xb,N,k,&r);
SignalPhase_FromCorrelationLag(lag,period,&r);
```

## 8 参数怎么改

没有平滑参数；必须把同一周期、同一 FFT bin 或同一相关 lag 交进来。

## 9 参数改大会怎样

period 填大，相同时间差换算角度变小；bin 选错会测到别的频率相位；max lag 由相关模块决定。

## 10 这个算法的代价是什么

Benefits：符号/单位统一、O(1)。Trade-offs：wrap 失去整周延迟信息；通道延迟直接进入相位。

## 11 什么时候不要用

两路频率不一致、非同步 ADC 未校准、目标幅值接近零、过零不是同一事件时。

## 12 怎么和前一个模块接

```text
DualADC -> 两路 Voltage/RemoveDC -> ZeroCross或FFT或Correlation
```

## 13 怎么和后一个模块接

```text
method result -> Phase(B-A) -> ChannelDelayCorrection -> display
```

## 14 最小Demo

```c
signal_phase_result_t r;
(void)SignalPhase_FromZeroCross(10,15,100,&r); /* -18 deg */
```

## 15 PC测试

解析三方法和完整双正弦 FFT 均 PASS；FFT +30° 实测 29.9999847°，相关延迟4/32周期得到 -45°。

排查：符号反先确认 B-A 约定；固定随频率增大的误差通常是通道时间差；跳±360是 wrap/事件配对问题。

## 16 MCU资源

O(1)、O(1) RAM；FFT 法资源主要在上游，ZeroCross 最省，相关最重。

## 17 验证状态

PC_VERIFIED；未做双 ADC 板端通道延迟校准。

## 18. 完整 Public API Reference

### `SignalPhase_FromZeroCross(crossing_a_samples, crossing_b_samples, period_samples, result)`

三个 float 的位置/周期单位均为 sample，period 必须为正；成功输出归一化到 `[-180,180)` 的 B-A deg/rad。B 过零更晚意味着 B 滞后，因此结果为负。

### `SignalPhase_FromFFTBin(spectrum_a, spectrum_b, spectrum_count, bin_index, result)`

两路只读 complex spectrum 等长；bin 必须在范围内且两路该 bin 幅值非零。用同一 bin 的相角计算 B-A；无特征返回 NO_FEATURE，非法范围/数值返回错误。两路 FFT 必须来自同步采样和相同预处理。

### `SignalPhase_FromCorrelationLag(lag_b_relative_to_a_samples, period_samples, result)`

lag/period 单位 sample；正 lag 表示 B 比 A 晚，输出同样按 B-A 规则归一化。period 必须为正。

三个函数均同步、无 Init/workspace，不修改输入。

## 19. Call Sequence / Realistic Example

```text
方法FFT: Dual Voltage -> RemoveDC/Window -> FFT A/B -> 同一 peak bin -> FromFFTBin
方法相关: Dual Voltage -> RemoveDC -> Correlation -> best_lag -> FromCorrelationLag
方法过零: 两路同方向 crossing + period -> FromZeroCross
```

```c
status = SignalPhase_FromFFTBin(fft_a, fft_b, N, peak_bin, &fft_phase);
if (corr_status == SIGNAL_ALGORITHM_OK) {
    status = SignalPhase_FromCorrelationLag(
        (float)corr.best_lag_samples, period_samples, &corr_phase);
}
```

## 20. Parameter / Result / Algorithm Scope

本模块主要依赖 period、bin 或 lag，没有内部阈值。period 填错会让过零/相关相位按比例错；FFT bin 选错会报告另一频率分量相位。双 ADC 同步与通道由上游负责，通道固定延迟属于校准。

输入/结果由调用者拥有；FFT 方法沿用两块 `8N` complex buffer，相关方法需要上游 `2L+1` float coefficients。本模块自身 O(1)。

常见错误：A/B 颠倒、把 B 晚解释为正相位、degree/radian 混用、两路非同步、FFT 使用不同 bin/窗、period 不对应目标频率、忽略通道延迟校准。

## 21. Quick Modify Table

| 我想改什么 | 去哪里 | 改什么 | 影响 | 是否需同步上游 |
|---|---|---|---|---|
| 相位方法 | Application config | ZeroCross/FFT/Correlation API | 鲁棒性/资源 | 否 |
| 目标频率 | Peak/period source | `bin_index`/period | 所测分量 | 否 |
| A/B定义 | call order | spectrum/crossing顺序 | 结果符号 | 否 |
| 通道同步/pin | Dual ADC `.syscfg` | ADC/Event/channel | 硬件偏差 | 是 |
| 固定延迟补偿 | Channel Delay Calibration | delay | 相位零偏 | 否 |

## API Reference

- `SignalPhase_FromZeroCross(crossing_a_samples, crossing_b_samples, period_samples, result)`
- `SignalPhase_FromFFTBin(spectrum_a, spectrum_b, spectrum_count, bin_index, result)`
- `SignalPhase_FromCorrelationLag(lag_b_relative_to_a_samples, period_samples, result)`

## 17. 统一 API 教程（已按当前头文件核对）

本节由当前公开头文件、实现中实际出现的状态码和正式模块注册表生成。它补充前文的场景教程；函数签名变化时必须重新运行 `tools/upgrade_formal_beginner_docs.ps1`，不要手工保留旧 API。

遵循仓库的 [Beginner README 标准](../../00_docs/BEGINNER_README_STANDARD.md)：先用最小示例完成一次正常数据流，再按需要阅读全功能示例和本节 API 细节。

### 什么时候用 / 什么时候不要用

当题目需要“phase”目录对应的公开功能，并且输入数据、单位和硬件资源满足前文约束时使用本模块。若只需要更简单的上游功能、输入尚未准备好，或需要不同的数据模型/外设资源，应先选择相邻模块而不是强行调用本 API。

### 输入 / 输出

输入由各 API 的只读数组、配置、频率/阈值和平台对象组成；输出写入 result/output/buffer 参数或由 Get API 返回。调用者负责数组容量、生命周期和物理单位；失败返回时输出不是有效结果。

### 调用顺序

```text
SignalPhase_FromZeroCross -> SignalPhase_FromFFTBin -> SignalPhase_FromCorrelationLag
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

### `signal_algorithm_status_t SignalPhase_FromZeroCross(float crossing_a_samples, float crossing_b_samples, float period_samples, signal_phase_result_t *result);`

**它做什么：** 用两路同方向过零位置与周期计算 B-A 相位差。

**什么时候调用：** 执行该模块公开的功能；具体数据流以本节参数表和本 README 前面的场景说明为准。

| 参数 | 类型 | 初学者解释 |
|---|---|---|
| `crossing_a_samples` | `float` | A 路过零位置，sample。 |
| `crossing_b_samples` | `float` | B 路对应过零位置，sample。 |
| `period_samples` | `float` | 周期，sample，必须为正。 |
| `result` | `signal_phase_result_t *` | 输出 B-A 相位，范围 [-180,180)，单位 deg/rad。 |

**返回：** 成功返回 SIGNAL_ALGORITHM_OK。

**最小调用形状：** `SignalPhase_FromZeroCross(...);`。可直接从 README_MINIMAL_EXAMPLE.c 复制正常流程；README_FULL_EXAMPLE.c 展示全部公开 API，其中取消类 API 会以 #if 0 隔离。

**注意：** 所有指针和数组都由调用者拥有；先检查返回值。异步采集、DMA 或回调还在使用 buffer 时，不能读取结果或改写该 buffer。

### `signal_algorithm_status_t SignalPhase_FromFFTBin(const signal_complex_f32_t *spectrum_a, const signal_complex_f32_t *spectrum_b, uint32_t spectrum_count, uint32_t bin_index, signal_phase_result_t *result);`

**它做什么：** 用同一 FFT bin 的两路复数相角计算 B-A 相位差。

**什么时候调用：** 执行该模块公开的功能；具体数据流以本节参数表和本 README 前面的场景说明为准。

| 参数 | 类型 | 初学者解释 |
|---|---|---|
| `spectrum_a` | `const signal_complex_f32_t *` | A 路 N 点复频谱。 |
| `spectrum_b` | `const signal_complex_f32_t *` | B 路 N 点复频谱。 |
| `spectrum_count` | `uint32_t` | 两数组长度。 |
| `bin_index` | `uint32_t` | 目标频率 bin。 |
| `result` | `signal_phase_result_t *` | 输出 B-A 相位 deg/rad。 |

**返回：** 成功返回 SIGNAL_ALGORITHM_OK；目标 bin 幅值为零返回 NO_FEATURE。

**最小调用形状：** `SignalPhase_FromFFTBin(...);`。可直接从 README_MINIMAL_EXAMPLE.c 复制正常流程；README_FULL_EXAMPLE.c 展示全部公开 API，其中取消类 API 会以 #if 0 隔离。

**注意：** 所有指针和数组都由调用者拥有；先检查返回值。异步采集、DMA 或回调还在使用 buffer 时，不能读取结果或改写该 buffer。

### `signal_algorithm_status_t SignalPhase_FromCorrelationLag(float lag_b_relative_to_a_samples, float period_samples, signal_phase_result_t *result);`

**它做什么：** 把互相关峰值 lag 换算为 B-A 相位差。

**什么时候调用：** 执行该模块公开的功能；具体数据流以本节参数表和本 README 前面的场景说明为准。

| 参数 | 类型 | 初学者解释 |
|---|---|---|
| `lag_b_relative_to_a_samples` | `float` | 正值表示 B 比 A 晚，单位 sample。 |
| `period_samples` | `float` | 周期，sample。 |
| `result` | `signal_phase_result_t *` | 输出 B-A 相位。 |

**返回：** 成功返回 SIGNAL_ALGORITHM_OK。

**最小调用形状：** `SignalPhase_FromCorrelationLag(...);`。可直接从 README_MINIMAL_EXAMPLE.c 复制正常流程；README_FULL_EXAMPLE.c 展示全部公开 API，其中取消类 API 会以 #if 0 隔离。

**注意：** 所有指针和数组都由调用者拥有；先检查返回值。异步采集、DMA 或回调还在使用 buffer 时，不能读取结果或改写该 buffer。

### 示例、模块链与验收

- 最小入门：`README_MINIMAL_EXAMPLE.c`，只保留正常入口和结果读取。
- 全功能：`README_FULL_EXAMPLE.c`，以正确顺序展示当前头文件全部公开 API；`Stop` 等非常规路径不会默认执行。
- 模块链：先由上游提供单位、采样率和有效数据，再调用本模块；成功后将输出交给显示、控制、测量或下一步 DSP。硬件资源仍以 SysConfig 合约为唯一来源。
- 文档验收：README/API、两份示例和头文件会由 `tools/validate_beginner_documentation.ps1` 覆盖检查；这只表示文档与源码签名一致，不代替未进行的实板验证。

### 模块链

`上游采集/配置 -> 本模块 -> 检查返回值和结果 -> 测量、显示、控制或下一步 DSP`。只有确认本模块的输出单位和有效状态后，才交给下一模块。

