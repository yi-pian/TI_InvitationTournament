# RMS 有效值（包含 DC）

**等级：CMSIS DIRECT。** 普通 RMS 不需要正式模块对象。

## 1. 它解决什么问题

输入：`float voltage_v[N]`。输出：整段信号的总 RMS，单位 V。总 RMS 同时包含 DC 和交流分量。

## 2. 最简单实现

### 比赛现场直接复制这一段

<!-- DIRECT_COPY_BEGIN -->
```c
#include "arm_math.h"

float32_t rms;
arm_rms_f32(x, n, &rms);
```
<!-- DIRECT_COPY_END -->

前提：`x != NULL`、`n > 0`。

## 3. 这段代码放哪里

放在 `voltage_v[]` 准备好之后。不要先 Remove DC，除非题目要的是 AC RMS。

## 4. 每一行什么意思

- `arm_rms_f32` 完成平方、平均和开方，把结果写入 `rms`。
- 它只读输入，不需要 Init 或 workspace。

## 5. main / processing 实际例子

```c
float32_t total_rms_v;
arm_rms_f32(voltage_v, SIGNAL_SAMPLE_COUNT, &total_rms_v);
/* total_rms_v 可直接显示；它包含波形 DC。 */
```

## 6. 题目里需要改什么

只改 buffer 和 `N`。如果 ADC 仍是 raw code，必须先换算成 V，才能把结果称为 Vrms。

## 7. 什么情况下这种方法会不准

记录没有代表题目要求的时间区间、存在削顶/毛刺、转换比例错误，都会影响结果。极大数平方可能溢出，但常见 0～3.3 V 信号不会遇到这个问题。

## 8. 精度不够怎么办

先确认记录包含足够周期并校准 VREF/前端增益。需要排除 DC 时改用 AC RMS Recipe；需要抵抗少量离群点时再评估 Robust RMS，且必须确认异常点不是有效尖峰。

## 9. 完整例子

输入 `{1.0f, -1.0f, 1.0f, -1.0f}`，RMS 应为 `1.0 V`。该代码通过 PC 真值测试；未做开发板验证。
