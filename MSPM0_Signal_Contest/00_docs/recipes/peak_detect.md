# Peak Detect 指定范围找最大值

**等级：LEVEL A — DIRECT RECIPE。** 普通主峰搜索就是范围内找最大值。

## 1. 它解决什么问题

输入：`float values[N]` 和搜索范围 `[first,last]`。输出：最大值和它的索引。常用于 magnitude 频谱中跳过 DC 后找主峰。

## 2. 最简单实现

### 比赛现场直接复制这一段

<!-- DIRECT_COPY_BEGIN -->
```c
#include <stdint.h>

static uint32_t recipe_peak_index(const float *x, uint32_t first,
                                  uint32_t last, float *peak_value)
{
    uint32_t i;
    uint32_t peak = first;

    for (i = first + 1U; i <= last; ++i) {
        if (x[i] > x[peak]) peak = i;
    }
    if (peak_value != 0) *peak_value = x[peak];
    return peak;
}
```
<!-- DIRECT_COPY_END -->

前提：`first <= last`，范围在数组内。

## 3. 这段代码放哪里

在 FFT Magnitude 之后，或任意需要找最大样本的处理位置。

## 4. 每一行什么意思

从 `first` 假定当前最大点，逐点比较；只在更大时更新，因此相同峰值返回最早位置。

## 5. main / processing 实际例子

```c
float peak_magnitude;
uint32_t peak_bin = recipe_peak_index(
    magnitude, 1U, SIGNAL_SAMPLE_COUNT / 2U, &peak_magnitude);
float coarse_frequency_hz = (float)peak_bin * Fs / (float)N;
```

## 6. 题目里需要改什么

改搜索起止 index。频谱找主频通常从 1 开始跳过 DC，并根据题目频率范围把 Hz 换成 bin 范围。

## 7. 什么情况下这种方法会不准

它只返回离散最大 bin；泄漏、多音、强 DC、搜索范围错误或噪声尖峰都会影响结果，不能直接得到 fractional-bin 高频率精度。

## 8. 精度不够怎么办

先正确 Remove DC/Window，再接正式 FFT Parabolic/Log-Parabolic 插值。需要多个峰时使用带最小间距和阈值的专用逻辑，而不是重复把同一主瓣当多个峰。

## 9. 完整例子

输入 `{0, 1, 5, 3}`、范围 1～3，返回 index=2、value=5。该代码通过 PC 真值测试；未做开发板验证。
