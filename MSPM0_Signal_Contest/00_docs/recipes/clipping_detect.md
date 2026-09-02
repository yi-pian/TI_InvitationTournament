# Clipping Detect 削顶计数

**等级：LEVEL A — DIRECT RECIPE。** 已知上下限后的比较计数不需要 config/result 模块。

## 1. 它解决什么问题

统计一帧中有多少点达到低端或高端阈值，用于判断 ADC/前端是否可能削顶。

## 2. 最简单实现

### 比赛现场直接复制这一段

<!-- DIRECT_COPY_BEGIN -->
```c
#include <stdint.h>

static uint32_t recipe_count_clipped(const float *x, uint32_t n,
                                     float low_limit, float high_limit)
{
    uint32_t i;
    uint32_t clipped = 0U;

    for (i = 0U; i < n; ++i) {
        if ((x[i] <= low_limit) || (x[i] >= high_limit)) ++clipped;
    }
    return clipped;
}
```
<!-- DIRECT_COPY_END -->

前提：`low_limit < high_limit`。

## 3. 这段代码放哪里

放在 ADC To Voltage 后，和 Vpp/RMS 并行。最好在任何滤波前判断，否则滤波可能把削顶平台变得不明显。

## 4. 每一行什么意思

每点同时检查低端和高端，只要触碰任一边界就增加计数；最终计数除以 N 可得到削顶比例。

## 5. main / processing 实际例子

```c
uint32_t clipped = recipe_count_clipped(
    voltage_v, SIGNAL_SAMPLE_COUNT, 0.01f, 3.29f);
int is_clipped = (clipped != 0U);
```

## 6. 题目里需要改什么

按真实输入范围设置上下限。不要机械写 0/3.3 V；前端偏置、内部参考和保留裕量都会改变合理阈值。

## 7. 什么情况下这种方法会不准

信号本来就应等于边界、阈值离真实饱和值太远、或滤波后再判断，都会造成误报/漏报。它只能提示“可能削顶”，不能恢复真实峰值。

## 8. 精度不够怎么办

同时查看 raw code 直方图和波形平台；调整前端增益/偏置或量程，而不是靠算法猜回被削掉的波形。

## 9. 完整例子

输入 `{0.0, 1.0, 3.3}`、范围 `(0.01, 3.29)`，削顶计数为 2。该代码通过 PC 真值测试；未做开发板验证。
