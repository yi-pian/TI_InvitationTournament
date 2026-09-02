# Threshold 简单阈值判断

**等级：LEVEL A — DIRECT RECIPE。** 单次判限无需正式模块。

## 1. 它解决什么问题

判断数组里是否存在大于等于某个阈值的样本，并可得到第一次超过阈值的位置。

## 2. 最简单实现

### 比赛现场直接复制这一段

<!-- DIRECT_COPY_BEGIN -->
```c
#include <stdint.h>

static int recipe_find_first_above(const float *x, uint32_t n,
                                   float threshold, uint32_t *index)
{
    uint32_t i;
    for (i = 0U; i < n; ++i) {
        if (x[i] >= threshold) {
            if (index != 0) *index = i;
            return 1;
        }
    }
    return 0;
}
```
<!-- DIRECT_COPY_END -->

返回 1 表示找到，0 表示整帧都未达到。

## 3. 这段代码放哪里

放在电压/幅值结果已经有明确单位之后。raw code 阈值和 V 阈值不能混用。

## 4. 每一行什么意思

循环按时间顺序检查；第一次满足 `>= threshold` 时返回并保存索引，因此不会继续浪费 CPU。

## 5. main / processing 实际例子

```c
uint32_t trigger_index;
if (recipe_find_first_above(voltage_v, SIGNAL_SAMPLE_COUNT,
                            2.0f, &trigger_index)) {
    /* 从 trigger_index 开始处理或显示。 */
}
```

## 6. 题目里需要改什么

改阈值、比较方向、输入单位和 N。找低于阈值时把 `>=` 改为 `<=`；不要同时保留两个难以辨认的版本。

## 7. 什么情况下这种方法会不准

阈值附近有噪声时会抖动；需要准确过零位置时，单点阈值不能提供亚采样插值，也没有滞回。

## 8. 精度不够怎么办

重复边沿/测频使用正式 Zero Cross（带滞回）和 Zero-Cross Interpolation；硬件触发优先 Comparator/Timer Capture。

## 9. 完整例子

输入 `{0.2, 0.8, 1.2}`、threshold=`1.0`，应返回 1 且 index=2。该代码通过 PC 真值测试；未做开发板验证。
