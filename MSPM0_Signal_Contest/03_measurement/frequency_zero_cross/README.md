# ZeroCross：先找到阈值夹在哪两个样本之间

> **LEVEL C / REAL ALGORITHM MODULE：** 滞回、方向、事件容量和边界状态会直接影响假过零/漏过零，因此继续作为正式模块；只做一次阈值判定时使用 Threshold Recipe。

## 第一次使用 Zero Cross？从这里开始

目标：“在一帧 float 波形中找出每次上升/下降穿过阈值的相邻样本索引”。它只找夹点；要精细频率还要接 Interpolation 和 Multi Cycle Average。

### STEP 1：加入工程

链接 `MSPM0_Signal_Contest/03_measurement/frequency_zero_cross/signal_zero_cross.c`；Include Path 加该目录和 `MSPM0_Signal_Contest/03_measurement/common`。

### STEP 2：include

```c
#include "signal_zero_cross.h"
```

### STEP 3：变量 / Buffer / Result

```c
float centered_v[N];
signal_zero_cross_event_t events[N];
signal_zero_cross_config_t cfg;
signal_zero_cross_result_t result;
```

`events` 由调用者创建；每项保存 `left_index/right_index/direction`。若只预计少量过零，也可用更小容量，但容量不足会返回错误。

### STEP 4：第一次要改的参数

| 参数 | 常用初值 | 含义 | 调大/调小与错误现象 | 同步 / SysConfig |
|---|---|---|---|---|
| `threshold_v` | Remove DC 后用 `0.0f` | 过零阈值，V | 偏离中心会改变过零时刻，甚至找不到事件 | 插值必须使用同一阈值；不改 SysConfig |
| `hysteresis_v` | `0.005f` 起试 | 抗阈值附近抖动的滞回，V | 大：假过零少但弱信号可能丢失；小：噪声可能重复触发 | 与噪声和幅值匹配；不改 SysConfig |
| `direction` | 测频优先 `RISING` | 上升/下降/两者 | `BOTH` 会把半周期也列为相邻事件 | Multi Cycle Average 只接同方向事件 |
| `event_capacity` | 最坏可先用 N | 输出事件容量 | 太小返回空间不足 | 数组容量同步；不改 SysConfig |

### STEP 5：SysConfig

**【不需要 SysConfig】**。它处理已采好的 float 数组。ADC Fs/通道在上游配置。

### STEP 6：初始化

没有 Init；每帧直接设置 `cfg` 后调用。

### STEP 7：真正调用

```c
cfg.threshold_v = 0.0f;
cfg.hysteresis_v = 0.005f;
cfg.direction = SIGNAL_ZERO_CROSS_RISING;

signal_algorithm_status_t status = SignalZeroCross_Process(
    centered_v, N, &cfg, events, N, &result);
```

### STEP 8：结果

- `result.event_count`：写入 events 的总数。
- `rising_count/falling_count`：各方向数量。
- `events[i].left_index/right_index`：阈值夹在这两个相邻采样点之间。

### STEP 9：接下一个模块

```c
signal_zero_cross_interpolation_result_t interp;
float crossing_samples[N];
if (SignalZeroCross_Process(centered_v, N, &cfg, events, N, &result) ==
    SIGNAL_ALGORITHM_OK) {
    (void)SignalZeroCrossInterpolation_Process(centered_v, N,
        cfg.threshold_v, events, result.event_count,
        crossing_samples, N, &interp);
}
```

```text
ADC To Voltage -> Remove DC -> Zero Cross -> Interpolation -> Multi Cycle Average
```

### STEP 10：Build

缺头文件/undefined symbol=Include 或 linked source；`NO_FEATURE`=没有找到过零；空间不足=扩大 events；事件异常多=滞回太小、方向选 BOTH 或没有先去 DC/选对阈值。

### STEP 11：验证

输入单调片段 `{-1,-0.2,+0.3,+1}`、threshold=0、RISING，应只得到一个事件，left=1、right=2。

### STEP 12：常见修改

1. **滞回 5 mV→20 mV**：改 `cfg.hysteresis_v=0.020f`；若弱信号不再触发，说明设得过大。
2. **只测频率**：用 `RISING` 或 `FALLING`，不要混用 BOTH 后直接平均。
3. **信号带 1.65 V 偏置**：先 Remove DC 后 threshold=0，或把 threshold 设为真实中点并在插值中保持一致。
4. **events 太占 RAM**：按最低频率和观察时长估算最大同方向事件数，再留余量。

### STEP 13：完整最小示例

```c
#include "signal_zero_cross.h"
void FindCross(void)
{
    const float x[4] = {-1.0f, -0.2f, 0.3f, 1.0f};
    signal_zero_cross_event_t e[2];
    signal_zero_cross_result_t r;
    const signal_zero_cross_config_t c = {
        .threshold_v = 0.0f, .hysteresis_v = 0.0f,
        .direction = SIGNAL_ZERO_CROSS_RISING
    };
    (void)SignalZeroCross_Process(x, 4U, &c, e, 2U, &r);
}
```

下面是原理、验证证据、结构体成员和完整 API Reference。

## 1 这个算法是干什么的？

它不直接输出“精确频率”，而是找出波形每次从阈值下方走到上方，或从上方走到下方时，阈值被哪两个相邻样本夹住。这样后续插值模块才能估计更精确的过零时刻。

## 2 一个最简单的例子

```text
threshold = 0 V
samples   = -0.4, -0.1, +0.2, +0.5 V
                         ↑
上升过零被 index 1 和 2 夹住
event = {left_index=1, right_index=2, RISING}
```

## 3 原理

上升过零要求 `x[n-1] < threshold` 且 `x[n] >= threshold`；下降相反。滞回不是把阈值改成两个过零点，而是要求信号先到达 `threshold-hysteresis` 才重新允许下一次上升检测，避免阈值附近的小抖动生成许多事件。

## 4 比赛里什么时候用？

纯正弦或边沿明确、SNR 较高的频率和相位测量。方波边沿非常干净时优先用硬件 Timer Capture；低 SNR 正弦可考虑 FFT。

## 5 输入

- `voltage_v[]`：单位 V，可含 DC。
- `count>=2`。
- `threshold_v`：实际比较中心，V。
- `hysteresis_v>=0`：重新武装宽度，V。
- `direction`：RISING/FALLING/BOTH。
- `events[]/event_capacity`：调用者工作区。

## 6 输出

事件数组，每项包含相邻 `left_index/right_index` 和方向；result 提供总数、上升数、下降数。没有事件返回 `SIGNAL_ALGORITHM_NO_FEATURE`。

## 7 API怎么调用

```c
signal_zero_cross_event_t events[32];
signal_zero_cross_config_t cfg = {
    .threshold_v = 0.0f,
    .hysteresis_v = 0.01f,
    .direction = SIGNAL_ZERO_CROSS_RISING
};
signal_zero_cross_result_t result;

SignalZeroCross_Process(centered_v, count, &cfg,
                        events, 32U, &result);
```

## 8 参数怎么改

未 RemoveDC 时，把 `threshold_v` 设为信号中心（例如约 1.65 V）；已 RemoveDC 时通常设 0 V。`hysteresis_v` 可从峰峰值的 1%~5% 小范围试起，再用测试数据检查漏检和假检。

## 9 参数改大会怎样

- 滞回变大：抗阈值附近噪声更强，但小幅信号或快速异常周期可能无法重新武装。
- 滞回变小：更灵敏，但噪声可制造多次事件。
- 只选 RISING：事件更少且都是整周期间隔；BOTH 会得到半周期交替事件，不能直接交给当前 MultiCycleAverage。

## 10 这个算法的代价是什么

Benefits：O(N)、逻辑透明、阈值不写死、可保存每个事件供诊断。

Trade-offs：需要事件 buffer；过零点会被噪声、失真和阈值误差移动；没有自动滤波。

## 11 什么时候不要用

- 低 SNR 导致大量假过零；
- 严重谐波每周期多次穿阈值；
- 单次非周期瞬态；
- 方波可直接 Timer Capture 却仍高成本 ADC。

## 12 怎么和前一个模块接

```text
含 DC: ADC_ToVoltage -> ZeroCross(threshold=信号中心)
去 DC: ADC_ToVoltage -> RemoveDC -> ZeroCross(threshold=0)
```

## 13 怎么和后一个模块接

```text
┌──────── ZeroCross ────────┐
│ voltage[] + threshold     │
│ hysteresis re-arm         │
│ event {left,right,dir}[]  │
└────────────┬──────────────┘
             ↓
 ZeroCrossInterpolation -> MultiCycleAverage -> frequency_hz
```

## 14 最小Demo

```c
const float x[] = {-0.2f, -0.1f, 0.1f, 0.2f};
signal_zero_cross_event_t e[2];
signal_zero_cross_config_t c = {0.0f, 0.0f, SIGNAL_ZERO_CROSS_RISING};
signal_zero_cross_result_t r;
(void)SignalZeroCross_Process(x, 4U, &c, e, 2U, &r);
/* e[0] 为 left=1,right=2 */
```

## 15 PC测试

已验证：DC=1.65 V 的 1234.5 Hz 正弦用 threshold=1.65 V；RemoveDC 后的 1000 Hz 正弦用 threshold=0；阈值附近抖动在 0.1 V 滞回下不会重复计数。第二批 21 项全部 PASS。

排查：频率翻倍先看是否把 BOTH 的半周期事件当整周期；事件过多增加滞回或检查噪声；无事件检查阈值是否落在波形范围内。

## 16 MCU资源

O(N) 时间，模块内部 O(1)。事件工作区由调用者决定；RISING-only 通常每周期一个事件。不要把最大可能 `N-1` 事件数组盲目放栈上，可在应用层静态分配合理上限并检查 BUFFER_TOO_SMALL。

## 17 验证状态

PC_VERIFIED：2026-08-07，GCC C11 严格编译，有 DC/去 DC/滞回测试通过；未 BOARD_VERIFIED。

## 17. 统一 API 教程（已按当前头文件核对）

本节由当前公开头文件、实现中实际出现的状态码和正式模块注册表生成。它补充前文的场景教程；函数签名变化时必须重新运行 `tools/upgrade_formal_beginner_docs.ps1`，不要手工保留旧 API。

遵循仓库的 [Beginner README 标准](../../00_docs/BEGINNER_README_STANDARD.md)：先用最小示例完成一次正常数据流，再按需要阅读全功能示例和本节 API 细节。

### 什么时候用 / 什么时候不要用

当题目需要“frequency_zero_cross”目录对应的公开功能，并且输入数据、单位和硬件资源满足前文约束时使用本模块。若只需要更简单的上游功能、输入尚未准备好，或需要不同的数据模型/外设资源，应先选择相邻模块而不是强行调用本 API。

### 输入 / 输出

输入由各 API 的只读数组、配置、频率/阈值和平台对象组成；输出写入 result/output/buffer 参数或由 Get API 返回。调用者负责数组容量、生命周期和物理单位；失败返回时输出不是有效结果。

### 调用顺序

```text
SignalZeroCross_Process
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

### `signal_algorithm_status_t SignalZeroCross_Process(const float *voltage_v, uint32_t count, const signal_zero_cross_config_t *config, signal_zero_cross_event_t *events, uint32_t event_capacity, signal_zero_cross_result_t *result);`

**它做什么：** 在电压样本中查找跨越指定阈值的相邻样本对。

**什么时候调用：** 对调用者提供的数据执行一次同步计算或生成，并在成功后写入输出对象/数组。

| 参数 | 类型 | 初学者解释 |
|---|---|---|
| `voltage_v` | `const float *` | 输入电压数组，单位 V，只读。 |
| `count` | `uint32_t` | 样本点数，至少为 2。 |
| `config` | `const signal_zero_cross_config_t *` | 阈值、非负滞回和方向配置，单位 V。 |
| `events` | `signal_zero_cross_event_t *` | 调用者提供的事件数组；每个事件保存阈值左右的相邻索引。 |
| `event_capacity` | `uint32_t` | events 可容纳的元素数，必须大于 0。 |
| `result` | `signal_zero_cross_result_t *` | 输出事件数量及上升/下降数量。 |

**返回：** 找到事件返回 SIGNAL_ALGORITHM_OK；未找到、空间不足或参数非法返回对应状态。

**最小调用形状：** `SignalZeroCross_Process(...);`。可直接从 README_MINIMAL_EXAMPLE.c 复制正常流程；README_FULL_EXAMPLE.c 展示全部公开 API，其中取消类 API 会以 #if 0 隔离。

**注意：** 所有指针和数组都由调用者拥有；先检查返回值。异步采集、DMA 或回调还在使用 buffer 时，不能读取结果或改写该 buffer。

### 示例、模块链与验收

- 最小入门：`README_MINIMAL_EXAMPLE.c`，只保留正常入口和结果读取。
- 全功能：`README_FULL_EXAMPLE.c`，以正确顺序展示当前头文件全部公开 API；`Stop` 等非常规路径不会默认执行。
- 模块链：先由上游提供单位、采样率和有效数据，再调用本模块；成功后将输出交给显示、控制、测量或下一步 DSP。硬件资源仍以 SysConfig 合约为唯一来源。
- 文档验收：README/API、两份示例和头文件会由 `tools/validate_beginner_documentation.ps1` 覆盖检查；这只表示文档与源码签名一致，不代替未进行的实板验证。

### 模块链

`上游采集/配置 -> 本模块 -> 检查返回值和结果 -> 测量、显示、控制或下一步 DSP`。只有确认本模块的输出单位和有效状态后，才交给下一模块。

