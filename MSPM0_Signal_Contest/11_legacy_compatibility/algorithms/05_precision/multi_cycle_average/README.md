# MultiCycleAverage：跨多个周期求频率

> **LEVEL A / COMPATIBILITY_API：** 新比赛工程请直接使用详细的 [Multi-Cycle Average Recipe](../../00_docs/recipes/multi_cycle_average.md)。首尾同向过零跨度公式不需要 result struct；旧 API 只为现有工程保留。

## 第一次使用 Multi Cycle Average？从这里开始

目标：“用多个同方向过零位置的首末差，得到平均周期和频率”。

### STEP 1：加入工程

链接 `MSPM0_Signal_Contest/05_precision/multi_cycle_average/signal_multi_cycle_average.c`；Include Path 加本目录和 `MSPM0_Signal_Contest/03_measurement/common`。

### STEP 2：include

```c
#include "signal_multi_cycle_average.h"
```

### STEP 3：变量

```c
float crossing_samples[E]; /* 严格递增、同方向，单位 sample */
signal_multi_cycle_average_result_t result;
```

### STEP 4：参数

| 参数 | 怎么设 | 调大/调小与错误现象 | SysConfig |
|---|---|---|---|
| `crossing_count` | 用 interpolation 的有效数量，至少 2 | 更多周期通常平均更稳，但响应更慢 | 否 |
| `sample_rate_hz` | 必须用真实/配置 ADC Fs，Hz | 写大则频率同比偏大，写小则偏小 | 改真实 Fs 时要改 ADC Timer/SysConfig |

最重要规则：数组只能放同方向过零。把上升和下降混在一起会把半周期当一周期，结果约翻倍。

### STEP 5：SysConfig

**【不直接需要 SysConfig】**。只有实际采样率改变时，修改上游 ADC Timer/Event 配置，并把同一个 Fs 传入这里。

### STEP 6：初始化

没有 Init；过零位置 ready 后直接调用。

### STEP 7：真正调用

```c
signal_algorithm_status_t status = SignalMultiCycleAverage_Process(
    crossing_samples, crossing_count, actual_fs_hz, &result);
```

### STEP 8：结果

- `result.frequency_hz`：频率 Hz。
- `average_period_samples`：平均周期，sample。
- `observation_time_s`：首末过零之间的时间，s。
- `cycle_count`：实际跨越周期数=`crossing_count-1`。

### STEP 9：连接

```text
Zero Cross events -> Linear Interpolation positions -> Multi Cycle Average frequency_hz
```

```c
signal_phase_result_t phase;
(void)SignalPhase_FromZeroCross(cross_a, cross_b,
    result.average_period_samples, &phase);
```

频率结果还能给 DDS 校准、显示或相位周期换算使用。

### STEP 10：Build

undefined symbol=未链接 `.c`；DATA_TOO_SHORT/参数错误=位置少于 2；频率翻倍=混入 BOTH 方向；频率固定比例错=Fs 不是实际采样率。

### STEP 11：验证

Fs=1000 Hz，位置 `{10,110,210}` sample，应得到平均周期 100 sample、2 个周期、频率 10 Hz。

### STEP 12：常见修改

1. **提高稳定性**：增加记录长度，让 crossing_count 增加；响应时间同步变长。
2. **提高响应速度**：减少周期数，但单次抖动占比会增大。
3. **Fs 变化**：只改传入值还不够，必须保证 ADC Timer 真实配置同步。
4. **要下降沿测频**：上游统一选 FALLING，本模块无需改变。

### STEP 13：完整最小示例

```c
#include "signal_multi_cycle_average.h"
void MeasureFrequency(void)
{
    const float p[3] = {10.0f, 110.0f, 210.0f};
    signal_multi_cycle_average_result_t r;
    if (SignalMultiCycleAverage_Process(p, 3U, 1000.0f, &r) ==
        SIGNAL_ALGORITHM_OK) {
        /* r.frequency_hz=10 Hz */
    }
}
```

下面是原理、验证证据和完整 API Reference。

## 1 这个算法是干什么的？

不用只拿相邻两个过零点算一个周期，而是拿第一个和最后一个同方向过零点的总间隔，除以中间跨过的周期数。单次过零位置的小误差因此被分摊。

## 2 一个最简单的例子

```text
上升过零位置: 0.25, 100.25, 200.25, 300.25 sample
跨越: 300 sample，3 个周期
平均周期: 100 sample
Fs=100000 Hz -> f=100000/100=1000 Hz
```

## 3 原理

`average_period_samples = (last-first)/(crossing_count-1)`，`frequency_hz = sample_rate_hz/average_period_samples`。首末各有一点时间误差，但除以很多周期后，频率相对误差通常减小。

## 4 比赛里什么时候用？

稳定正弦、三角波或周期边沿的高精度平均频率。需要快速跟踪频率跳变时应缩短观测窗口。

## 5 输入

严格递增的同方向过零位置，单位 sample；至少两个；`sample_rate_hz>0`，单位 Hz。

## 6 输出

`frequency_hz`（Hz）、`average_period_samples`（sample）、`observation_time_s`（s）、`cycle_count`（周期）。

## 7 API怎么调用

```c
signal_multi_cycle_average_result_t result;
SignalMultiCycleAverage_Process(
    positions_samples, position_count,
    sample_rate_hz, &result);
```

## 8 参数怎么改

模块没有滤波参数。通过传入多少个过零位置控制平均周期数。只传 Rising 或只传 Falling，不能混合 BOTH 输出。

## 9 参数改大会怎样

周期数增大：稳定频率精度/重复性通常更好；代价是延迟更长，频率变化被平均。周期数减小：响应快，但更容易受单次插值和噪声影响。

## 10 这个算法的代价是什么

Benefits：只需首末差值，RAM/CPU 很低；自然给出观测时间。

Trade-offs：只给平均频率；采样时钟偏差会等比例进入结果；漏过零会把频率算低，假过零会算高。

## 11 什么时候不要用

- RISING/FALLING 交替混在同一数组：结果约翻倍；
- 信号扫频/调频且要瞬时频率；
- 只有一个事件；
- 上游事件漏检或误检未排查。

## 12 怎么和前一个模块接

```text
ZeroCross -> LinearInterpolation -> crossing_positions_samples[]
                                      ↓
                              MultiCycleAverage
```

## 13 怎么和后一个模块接

```text
┌──── MultiCycleAverage ────┐
│ first/last crossing       │
│ span / cycle_count        │
│ Fs / period              │
└────────────┬──────────────┘
             ↓
 frequency_hz / quality display / control
```

## 14 最小Demo

```c
const float p[] = {0.25f,100.25f,200.25f,300.25f};
signal_multi_cycle_average_result_t r;
(void)SignalMultiCycleAverage_Process(p, 4U, 100000.0f, &r);
/* r.frequency_hz=1000, r.cycle_count=3 */
```

## 15 PC测试

已知位置得到 1000 Hz；完整链对带 1.65 V DC 的 1234.5 Hz 正弦及 RemoveDC 后 1000 Hz 正弦，绝对误差均小于 0.01 Hz；逆序位置正确报错。全部 PASS。

排查：频率约 2 倍检查 BOTH 事件；约 1/2 检查漏检；固定比例误差检查 `sample_rate_hz` 不是目标配置值而应为真实/校准值。

## 16 MCU资源

时间 O(K)（检查递增）且内部 RAM O(1)。只含少量软件浮点除法，不分配数组。

## 17 验证状态

PC_VERIFIED：2026-08-07，严格编译、已知位置及两条完整测频链通过；未 BOARD_VERIFIED。

## 18. 完整 API、调用顺序与 Buffer 规则

唯一公开函数 `SignalMultiCycleAverage_Process(crossing_positions_samples, crossing_count, sample_rate_hz, result)`：positions 为严格递增、同方向的只读 float 过零位置，单位 sample；`crossing_count>=2`；Fs>0 Hz；result 非空。成功返回 OK，并写 `frequency_hz`、`average_period_samples`、`observation_time_s`、`cycle_count=crossing_count-1`；数据不足、顺序错误或数值非法返回对应状态。

它不需要 workspace、不修改 positions、没有 Init。调用顺序固定为：

```text
ZeroCross(RISING only) -> Interpolation -> MultiCycleAverage -> frequency_hz
```

```c
signal_multi_cycle_average_result_t f;
status = SignalMultiCycleAverage_Process(
    positions, interp.position_count, actual_fs_hz, &f);
```

## 19. Parameter / Result Meaning

crossing_count 增大意味着跨更多周期平均，随机抖动通常下降但响应变慢；输入 RAM 已由 positions 提供，本模块 O(1)。`sample_rate_hz` 必须是每通道实际配置/校准率；填错会使 frequency 按比例错。BOTH 方向事件相邻为半周期，会让结果约翻倍。

全部 CONFIG ONLY；只有改变上游真实 Fs 才需采集 SysConfig。

## 20. Quick Modify Table

| 我想改什么 | 去哪里 | 改什么 | 影响 | SysConfig? |
|---|---|---|---|---|
| 平均周期数 | 上游事件容量/帧长 | `crossing_count` | 抖动/响应 | 否 |
| Fs | 采集结果 + call | `sample_rate_hz` | 频率比例 | 改硬件Fs时是 |
| 上升/下降沿 | ZeroCross config | 只保留一种方向 | 周期含义 | 否 |
| 观测时间 | N/Fs | 帧长/采样率 | 可用周期数 | 改硬件Fs时可能是 |
