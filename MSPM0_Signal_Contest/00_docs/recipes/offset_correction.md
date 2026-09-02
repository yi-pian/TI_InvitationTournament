# Offset Correction 固定偏移修正

**等级：CMSIS DIRECT。** 不创建单独 Offset 模块。

## 1. 它解决什么问题

给所有样本加同一个偏移：`output[i] = input[i] + offset`。例如测量值恒定偏高 0.02 V，可传 `offset=-0.02f`。

## 2. 最简单实现

### 比赛现场直接复制这一段

<!-- DIRECT_COPY_BEGIN -->
```c
#include "arm_math.h"

arm_offset_f32(input, offset, output, n);
```
<!-- DIRECT_COPY_END -->

允许输入输出为同一数组。

## 3. 这段代码放哪里

在 ADC To Voltage 之后、所有电压测量之前。不要和 Remove DC 混淆：这里修正已知固定误差，Remove DC 会删除当前整帧平均值。

## 4. 每一行什么意思

`arm_offset_f32` 逐点把固定 `offset` 加到输入。offset 的正负号由“修正后 = 测量值 + offset”定义。

## 5. main / processing 实际例子

```c
#define SIGNAL_OFFSET_CORRECTION_V  (-0.020f)
arm_offset_f32(voltage_v, SIGNAL_OFFSET_CORRECTION_V,
               voltage_v, SIGNAL_SAMPLE_COUNT);
```

## 6. 题目里需要改什么

只改偏移值、数组和 N。偏移单位必须和数组相同。

## 7. 什么情况下这种方法会不准

偏移会随温度、量程、增益或时间变化时，一个常数不能解决；把测得的当前信号 DC 当“误差”减掉也会删除有效信息。

## 8. 精度不够怎么办

使用两个可靠参考点做正式 ADC Gain/Offset Calibration；需要实时去基线而非校准时使用 Remove DC Recipe。

## 9. 完整例子

输入 `{1.02, 2.02}` V、offset=`-0.02 V`，输出 `{1.00, 2.00}` V。该代码通过 PC 真值测试；未做开发板验证。
