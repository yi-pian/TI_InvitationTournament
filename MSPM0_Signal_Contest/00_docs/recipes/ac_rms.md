# AC RMS 交流有效值

**等级：CMSIS RECIPE。** 它用 Mean、Offset 和 RMS 三个 CMSIS 算子得到去掉平均值后的 RMS。

## 1. 它解决什么问题

输入：带 DC 偏置的 `float voltage_v[N]`。输出：交流分量的 RMS，单位 V；可选同时取得本帧 DC。

## 2. 最简单实现

### 比赛现场直接复制这一段

<!-- DIRECT_COPY_BEGIN -->
```c
#include "arm_math.h"

float32_t dc_v, ac_rms_v;
arm_mean_f32(x, n, &dc_v);
arm_offset_f32(x, -dc_v, ac_workspace_v, n);
arm_rms_f32(ac_workspace_v, n, &ac_rms_v);
```
<!-- DIRECT_COPY_END -->

前提：`x != NULL`、`ac_workspace_v` 至少有 `n` 点、`n > 0`。确认不再需要原数组时，CMSIS offset 也允许输入输出使用同一地址。

## 3. 这段代码放哪里

在 ADC To Voltage 后直接调用。不要先调用 Remove DC 又在这里重复求平均，二选一即可。

## 4. 每一行什么意思

- `arm_mean_f32` 求本帧 DC。
- `arm_offset_f32` 给每点加 `-dc_v`，得到交流数组。
- `arm_rms_f32` 计算交流数组的有效值。

## 5. main / processing 实际例子

```c
static float32_t ac_workspace_v[SIGNAL_SAMPLE_COUNT];
float32_t dc_v, ac_rms_v;
arm_mean_f32(voltage_v, SIGNAL_SAMPLE_COUNT, &dc_v);
arm_offset_f32(voltage_v, -dc_v, ac_workspace_v, SIGNAL_SAMPLE_COUNT);
arm_rms_f32(ac_workspace_v, SIGNAL_SAMPLE_COUNT, &ac_rms_v);
```

## 6. 题目里需要改什么

改输入 buffer 和 `N`。如果题目要“总 RMS”，不要用本 Recipe，应使用普通 RMS。

## 7. 什么情况下这种方法会不准

帧内 DC 正在变化时，一个全帧平均值不能代表时变基线；毛刺和削顶也会进入平方和。非常短、非整数周期记录仍能得到“这段记录”的 AC RMS，但不一定代表稳态周期值。

## 8. 精度不够怎么办

延长记录、确认基线稳定、校准电压比例。基线缓慢弯曲时需要 Detrend/高通的正式方法，而不是继续套一个全帧平均。

## 9. 完整例子

输入 `{2, 4, 2, 4}` V，DC=`3 V`，AC RMS=`1 V`。该代码通过 PC 真值测试；未做开发板验证。
