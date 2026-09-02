# Min / Max 最小值和最大值

**等级：CMSIS RECIPE。** 新比赛工程不需要复制 `signal_minmax.c/.h`。

## 1. 它解决什么问题

输入：`float samples[N]`。输出：最小值和最大值，单位与输入相同。适合查看当前波形范围。

## 2. 最简单实现

### 比赛现场直接复制这一段

<!-- DIRECT_COPY_BEGIN -->
```c
#include "arm_math.h"

float32_t minimum, maximum;
uint32_t min_index, max_index;
arm_min_f32(x, n, &minimum, &min_index);
arm_max_f32(x, n, &maximum, &max_index);
```
<!-- DIRECT_COPY_END -->

前提：所有指针有效，`n > 0`。

## 3. 这段代码放哪里

放在一帧 `voltage_v[]` 已经准备好的处理函数中。它只读数组，可以和 Mean/RMS 并行调用。

## 4. 每一行什么意思

- `arm_min_f32` 返回最小值和它第一次出现的下标。
- `arm_max_f32` 返回最大值和它第一次出现的下标。
- 两个函数只读输入；无需 Init 或 workspace。

## 5. main / processing 实际例子

```c
float minimum_v;
float maximum_v;
uint32_t min_index, max_index;
arm_min_f32(voltage_v, SIGNAL_SAMPLE_COUNT, &minimum_v, &min_index);
arm_max_f32(voltage_v, SIGNAL_SAMPLE_COUNT, &maximum_v, &max_index);
```

## 6. 题目里需要改什么

改 `voltage_v` 和 `SIGNAL_SAMPLE_COUNT`。如果只分析数组中间一段，可传 `&voltage_v[start]` 和该段长度。

## 7. 什么情况下这种方法会不准

一个孤立毛刺就能完全改变结果；采样点没有落在连续波形真实峰顶时，也会略低估最大值或高估最小值。

## 8. 精度不够怎么办

确认毛刺不是有效信号后，再考虑 Hampel 或 Robust Peak-to-Peak。需要峰值位置时使用旧兼容 MinMax 或 Peak Detect Recipe，不要为了位置再增加一层通用结构。

## 9. 完整例子

输入 `{2, -1, 5, 3}`，输出应为 `minimum=-1`、`maximum=5`。未做开发板验证。
