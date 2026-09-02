# Multi-Cycle Average 多周期平均测频

**等级：LEVEL A — DIRECT RECIPE。** 已有同方向过零位置后，频率只需首尾跨度公式。

## 1. 它解决什么问题

输入：按时间递增的同方向过零位置 `crossings[]`，单位 sample；采样率 `Fs`，单位 Hz。输出：跨多个周期平均后的频率 Hz。

## 2. 最简单实现

### 比赛现场直接复制这一段

<!-- DIRECT_COPY_BEGIN -->
```c
#include <stdint.h>

static float recipe_multicycle_frequency(const float *crossings,
                                         uint32_t crossing_count,
                                         float sample_rate_hz)
{
    float cycle_count = (float)(crossing_count - 1U);
    float span_samples = crossings[crossing_count - 1U] - crossings[0];

    return sample_rate_hz * cycle_count / span_samples;
}
```
<!-- DIRECT_COPY_END -->

前提：`crossing_count >= 2`，位置严格递增，全部是同一方向的过零，`sample_rate_hz > 0`。

## 3. 这段代码放哪里

放在 Zero Cross 和 Zero-Cross Interpolation 已得到一组亚采样位置之后。

```text
Zero Cross -> Interpolation -> crossings[] -> Multi-Cycle Average
```

## 4. 每一行什么意思

- M 个同方向过零位置之间包含 M-1 个完整周期。
- 首尾差是这些周期总共跨过的 sample 数。
- `Fs * 周期数 / 总 sample 数` 就是平均频率。

## 5. main / processing 实际例子

```c
float frequency_hz = recipe_multicycle_frequency(
    rising_crossings, rising_count, SIGNAL_SAMPLE_RATE_HZ);
```

## 6. 题目里需要改什么

传真实 Fs 和当前帧有效 crossing 数量。若 events 同时含上升、下降沿，必须先只保留一种方向，否则结果会接近两倍频率。

## 7. 什么情况下这种方法会不准

漏过零、假过零、首尾方向不一致、频率在观察期间变化或 Fs 标称值不准都会出错。只用首尾能平均随机抖动，但不会发现中间漏掉一个周期。

## 8. 精度不够怎么办

使用 Zero Cross 的滞回和插值；同时检查相邻过零间隔是否接近中位数，剔除帧应有明确依据。频率快速变化时缩短观察窗口，硬件方波优先 Timer Capture。

## 9. 完整例子

Fs=`1000 Hz`，过零位置 `{0, 10, 20, 30}`，跨 3 周期/30 samples，结果 `100 Hz`。该代码通过 PC 真值测试；未做开发板验证。
