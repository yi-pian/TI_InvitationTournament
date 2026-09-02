# Normalize 归一化到指定峰值

**等级：CMSIS RECIPE。** 适合显示、相关或测试向量；不要把归一化结果冒充原始物理幅值。

## 1. 它解决什么问题

找数组绝对值最大点，把整个数组等比例缩放到 `target_peak`。输出单位通常变成无量纲。

## 2. 最简单实现

### 比赛现场直接复制这一段

<!-- DIRECT_COPY_BEGIN -->
```c
#include "arm_math.h"

float32_t peak;
uint32_t peak_index;
arm_absmax_f32(input, n, &peak, &peak_index);
if (peak > 1.0e-20f) {
    arm_scale_f32(input, target_peak / peak, output, n);
}
```
<!-- DIRECT_COPY_END -->

允许原地；返回 0 表示输入几乎全零，不能归一化。

## 3. 这段代码放哪里

放在显示、波形比较或相关前。物理幅值测量链应保留原数组或先保存原峰值。

## 4. 每一行什么意思

- `arm_absmax_f32` 找最大绝对值，因此正峰和负峰都考虑。
- 全零信号必须跳过缩放，避免除零。
- `arm_scale_f32` 乘 `target_peak/peak`，保持波形形状，只改整体比例。

## 5. main / processing 实际例子

```c
if (recipe_normalize_peak(centered_v, normalized,
                          SIGNAL_SAMPLE_COUNT, 1.0f)) {
    /* normalized[] 的绝对峰值为 1，适合画图或相关。 */
}
```

## 6. 题目里需要改什么

改目标峰值、输入输出 buffer 和 N。常用 `target_peak=1.0f`。

## 7. 什么情况下这种方法会不准

孤立毛刺会成为 peak，导致其余波形被缩得很小；归一化也会主动丢失幅值信息，因此不能在 Vpp/RMS/THD 幅值链前随便使用。

## 8. 精度不够怎么办

确认毛刺不是有效信号后用稳健峰值估计；需要保留物理量时不要归一化，改用校准/Scaling。

## 9. 完整例子

输入 `{-2, 1, 0}`、target=`1`，输出 `{-1, 0.5, 0}`。该代码通过 PC 真值测试；未做开发板验证。
