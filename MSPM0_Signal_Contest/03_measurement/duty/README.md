# Duty：占空比测量

这是一次 `SOURCE_LOST → CLEAN_REIMPLEMENTATION`。它不是旧源码恢复，也不继承旧模块的验证状态。

## 1. 它解决什么问题

- 输入：一帧双状态波形 `float samples[N]` 和真实采样率 `sample_rate_hz`。
- 输出：正占空比、周期、频率、高/低脉宽以及有效周期数。
- 典型链路：`ADC DMA → ADC To Voltage → Duty`。

该模块用中间阈值 crossing 计算时间，并在线性边沿上进行亚采样点插值。它不是简单统计“多少个采样点高于阈值”。

## 2. 需要复制哪些文件

```text
03_measurement/duty/signal_duty.c
03_measurement/duty/signal_duty.h
03_measurement/common/signal_algorithm_status.h
```

工程 Include Path 至少加入 `03_measurement/duty` 和 `03_measurement/common`。这是纯算法，不需要 SysConfig。

## 3. 最小调用

```c
#include "signal_duty.h"

signal_duty_config_t cfg;
signal_duty_result_t measured;

SignalDuty_GetDefaultConfig(&cfg);
if (SignalDuty_Process(voltage_v, N, FS_HZ, &cfg, &measured)
        == SIGNAL_ALGORITHM_OK) {
    printf("Duty = %.3f %%\n", measured.duty_percent);
}
```

可以直接打开同目录的 `README_MINIMAL_EXAMPLE.c` 查看完整小函数。

## 4. 最常修改的参数

| 我要改变什么 | 改哪里 | 说明 |
|---|---|---|
| 采样率 | `SignalDuty_Process` 的 `sample_rate_hz` | 必须传真实 Fs，不是期望值 |
| 自动/显式电平 | `cfg.level_mode` | 默认自动 min/max；强噪声或低占空比改显式 |
| 中间阈值 | `cfg.threshold_ratio` | 默认 0.5，即 Base/Top 的 50% |
| 抗抖滞回 | `cfg.hysteresis_ratio` | 默认 0.05；噪声更大可适当提高 |
| 最小有效幅度 | `cfg.min_amplitude` | 小于该值认为没有可测脉冲 |
| 已知低/高电平 | `cfg.low_level/high_level` | 仅 `EXPLICIT` 模式使用，单位和输入一致 |

阈值与滞回必须满足：

```text
0 < threshold_ratio < 1
0 <= hysteresis_ratio < min(threshold_ratio, 1-threshold_ratio)
```

## 5. 自动电平与显式电平怎么选

默认 `AUTO_MIN_MAX` 适合干净方波，零额外 RAM，也最容易上手。但单个过冲或毛刺会改变 min/max，进而移动阈值。

出现以下情况时改用 `EXPLICIT`：

- 占空比很小，高平台样本很少；
- 有明显过冲、振铃或孤立尖峰；
- 前端已经标定出逻辑低/高电平；
- 希望不同采样帧使用完全一致的判决电平。

如果电平未知且毛刺明显，可以先用 Median/Hampel 清理副本，再估计 Base/Top；不要为了方便修改本模块 `.c`。

## 6. 返回结果怎么看

- `duty_ratio`：0～1 的无量纲比值。
- `duty_percent`：百分比，例如 25% 返回约 `25.0F`。
- `period_s`、`high_width_s`、`low_width_s`：单位秒。
- `frequency_hz`：单位 Hz。
- `valid_cycle_count`：真正参与平均的完整周期数；帧两端残缺周期自动忽略。

建议至少采 3 个完整周期，比赛测量通常采 5～20 个周期。边沿阈值区最好有多个样点；如果一个边沿在相邻采样点间近似线性，插值才能有效提高时间精度。

## 7. 常见失败

| 返回码 | 常见原因 |
|---|---|
| `INVALID_ARGUMENT` | 空指针、非法 level mode |
| `INSUFFICIENT_DATA` | 少于 3 个样本 |
| `OUT_OF_RANGE` | 阈值/滞回非法，或显式 high 不大于 low |
| `NO_FEATURE` | 常量信号、幅度过小、没有完整 rise→fall→rise |
| `NUMERIC_ERROR` | 输入、Fs 或配置存在 NaN/Inf |

失败时结果结构不改变。使用者必须先检查返回码，再显示结果。

## 8. 精度和局限

- 线性插值只对 crossing 附近近似直线的边沿有效。
- 自动 min/max 对毛刺敏感；强噪声时优先显式电平或先做稳健预处理。
- 一个完整周期都没有时不能测 duty。
- 极窄高/低脉冲如果没有跨过滞回 guard，会被认为不是有效状态。
- 当前模块不计算 10/90 或 20/80 rise/fall time；它们仍由 `pulse_timing` Recipe 的多阈值逻辑处理。

## 9. 算法与验证资料

- 规格和旧→新 API 说明：`REIMPLEMENTATION_SPEC.md`
- Python reference：`10_tests/algorithm_reference/duty/duty_reference.py`
- 典型数据：`10_tests/algorithm_reference/duty/test_vectors.json`
- 状态证据：`VERIFICATION.yaml`

当前验证状态以 `VERIFICATION.yaml` 为准。没有开发板输入实测时，`BOARD` 必须保持 `NOT_RUN`。

## 17. 统一 API 教程（已按当前头文件核对）

本节由当前公开头文件、实现中实际出现的状态码和正式模块注册表生成。它补充前文的场景教程；函数签名变化时必须重新运行 `tools/upgrade_formal_beginner_docs.ps1`，不要手工保留旧 API。

遵循仓库的 [Beginner README 标准](../../00_docs/BEGINNER_README_STANDARD.md)：先用最小示例完成一次正常数据流，再按需要阅读全功能示例和本节 API 细节。

### 什么时候用 / 什么时候不要用

当题目需要“duty”目录对应的公开功能，并且输入数据、单位和硬件资源满足前文约束时使用本模块。若只需要更简单的上游功能、输入尚未准备好，或需要不同的数据模型/外设资源，应先选择相邻模块而不是强行调用本 API。

### 输入 / 输出

输入由各 API 的只读数组、配置、频率/阈值和平台对象组成；输出写入 result/output/buffer 参数或由 Get API 返回。调用者负责数组容量、生命周期和物理单位；失败返回时输出不是有效结果。

### 调用顺序

```text
SignalDuty_Process -> SignalDuty_GetDefaultConfig
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

### `signal_algorithm_status_t SignalDuty_GetDefaultConfig(signal_duty_config_t *config);`

**它做什么：** 读取最近一次操作保存的状态、结果或配置；先确认前置操作已经成功。

**什么时候调用：** 读取最近一次操作保存的状态、结果或配置；先确认前置操作已经成功。

| 参数 | 类型 | 初学者解释 |
|---|---|---|
| `config` | `signal_duty_config_t *` | 调用者填写的配置对象。先阅读该类型的成员；它控制本次初始化或处理方式。 |

**返回：** 当前实现中出现的返回/成熟度枚举值：`SIGNAL_ALGORITHM_INVALID_ARGUMENT`、`SIGNAL_ALGORITHM_OK`。

**最小调用形状：** `SignalDuty_GetDefaultConfig(...);`。可直接从 README_MINIMAL_EXAMPLE.c 复制正常流程；README_FULL_EXAMPLE.c 展示全部公开 API，其中取消类 API 会以 #if 0 隔离。

**注意：** 所有指针和数组都由调用者拥有；先检查返回值。异步采集、DMA 或回调还在使用 buffer 时，不能读取结果或改写该 buffer。

### `signal_algorithm_status_t SignalDuty_Process(const float *samples, uint32_t count, float sample_rate_hz, const signal_duty_config_t *config, signal_duty_result_t *result);`

**它做什么：** 对调用者提供的数据执行一次同步计算或生成，并在成功后写入输出对象/数组。

**什么时候调用：** 对调用者提供的数据执行一次同步计算或生成，并在成功后写入输出对象/数组。

| 参数 | 类型 | 初学者解释 |
|---|---|---|
| `samples` | `const float *` | Read-only finite samples in any linear amplitude unit. |
| `count` | `uint32_t` | Number of samples; at least three and enough for one full cycle. |
| `sample_rate_hz` | `float` | Physical sample rate in Hz, finite and greater than zero. |
| `config` | `const signal_duty_config_t *` | Threshold, hysteresis and state-level configuration. |
| `result` | `signal_duty_result_t *` | Output in ratio, percent, seconds and Hz. Unchanged on failure. |

**返回：** 当前实现中出现的返回/成熟度枚举值：`SIGNAL_ALGORITHM_INVALID_ARGUMENT`、`SIGNAL_ALGORITHM_INSUFFICIENT_DATA`、`SIGNAL_ALGORITHM_NUMERIC_ERROR`、`SIGNAL_ALGORITHM_OUT_OF_RANGE`、`SIGNAL_ALGORITHM_NO_FEATURE`、`SIGNAL_ALGORITHM_OK`。

**最小调用形状：** `SignalDuty_Process(...);`。可直接从 README_MINIMAL_EXAMPLE.c 复制正常流程；README_FULL_EXAMPLE.c 展示全部公开 API，其中取消类 API 会以 #if 0 隔离。

**注意：** 所有指针和数组都由调用者拥有；先检查返回值。异步采集、DMA 或回调还在使用 buffer 时，不能读取结果或改写该 buffer。

### 示例、模块链与验收

- 最小入门：`README_MINIMAL_EXAMPLE.c`，只保留正常入口和结果读取。
- 全功能：`README_FULL_EXAMPLE.c`，以正确顺序展示当前头文件全部公开 API；`Stop` 等非常规路径不会默认执行。
- 模块链：先由上游提供单位、采样率和有效数据，再调用本模块；成功后将输出交给显示、控制、测量或下一步 DSP。硬件资源仍以 SysConfig 合约为唯一来源。
- 文档验收：README/API、两份示例和头文件会由 `tools/validate_beginner_documentation.ps1` 覆盖检查；这只表示文档与源码签名一致，不代替未进行的实板验证。

### 模块链

`上游采集/配置 -> 本模块 -> 检查返回值和结果 -> 测量、显示、控制或下一步 DSP`。只有确认本模块的输出单位和有效状态后，才交给下一模块。

