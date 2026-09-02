# Remove DC 去直流

**等级：CMSIS RECIPE。** 使用 CMSIS Mean + Offset，可原地处理。

## 1. 它解决什么问题

输入：`float input_v[N]`。输出：`output_v[N] = input_v[N] - 本帧平均值`，单位仍为 V；函数返回被减掉的 DC。

## 2. 最简单实现

### 比赛现场直接复制这一段

<!-- DIRECT_COPY_BEGIN -->
```c
#include "arm_math.h"

float32_t removed_dc_v;
arm_mean_f32(input, n, &removed_dc_v);
arm_offset_f32(input, -removed_dc_v, output, n);
```
<!-- DIRECT_COPY_END -->

允许 `input == output`；前提是指针有效且 `n > 0`。

## 3. 这段代码放哪里

通常放在 Window/FFT、Zero Cross 之前：

```text
ADC To Voltage -> Remove DC -> Window / Zero Cross
```

## 4. 每一行什么意思

- `arm_mean_f32` 先求完整帧平均值。
- `arm_offset_f32` 给每点加上负平均值。
- `removed_dc_v` 可用于显示原始 DC 或调试。

## 5. main / processing 实际例子

```c
static float centered_v[SIGNAL_SAMPLE_COUNT];
float32_t removed_dc_v;
arm_mean_f32(voltage_v, SIGNAL_SAMPLE_COUNT, &removed_dc_v);
arm_offset_f32(voltage_v, -removed_dc_v, centered_v, SIGNAL_SAMPLE_COUNT);
```

节省 RAM 时可把 `arm_offset_f32` 的输入和输出都传 `voltage_v`。

## 6. 题目里需要改什么

改输入、输出 buffer 和 `N`。它没有截止频率参数，不是通用高通滤波器。

## 7. 什么情况下这种方法会不准

帧内基线明显漂移时，减一个常数不能消除趋势；若题目本来要测 DC，调用它会主动删除目标量。

## 8. 精度不够怎么办

先确认采样窗口覆盖合理周期。缓慢趋势需要 Detrend/高通；测 DC、总 RMS 或绝对电压时保留原始 `voltage_v[]`。

## 9. 完整例子

输入 `{2, 4, 2, 4}`，返回 DC=`3`，输出 `{-1, 1, -1, 1}`。该代码通过 PC 真值测试；未做开发板验证。
