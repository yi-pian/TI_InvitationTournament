# Scaling 比例缩放

**等级：CMSIS DIRECT。** 不创建单独 Scaling 模块。

## 1. 它解决什么问题

把每个样本乘同一个比例：`output[i] = input[i] * gain`。输入输出单位由 `gain` 决定。

## 2. 最简单实现

### 比赛现场直接复制这一段

<!-- DIRECT_COPY_BEGIN -->
```c
#include "arm_math.h"

arm_scale_f32(input, gain, output, n);
```
<!-- DIRECT_COPY_END -->

允许输入输出为同一数组。

## 3. 这段代码放哪里

放在需要统一改变数组比例的位置，例如传感器单位换算或已知放大倍数修正后。

## 4. 每一行什么意思

`arm_scale_f32` 把每个样本乘以同一个 `gain` 后写到输出。没有配置结构、Init 或隐藏状态。

## 5. main / processing 实际例子

```c
/* ADC 端看到被测信号的一半，乘 2 还原被测端电压。 */
arm_scale_f32(voltage_v, 2.0f, voltage_v, SIGNAL_SAMPLE_COUNT);
```

## 6. 题目里需要改什么

改 `gain`、输入输出数组和 `N`。把比例参数集中放在 `signal_config.h`，不要散落多个循环。

## 7. 什么情况下这种方法会不准

误差不是固定比例、前端随频率变化、信号削顶或 gain 方向写反时会不准。

## 8. 精度不够怎么办

同时存在比例和零偏时使用正式两点 ADC Gain/Offset Calibration；随频率变化时需要 Frequency Response Correction，而不是一个常数 gain。

## 9. 完整例子

输入 `{1, 2, 3}`、gain=`2`，输出 `{2, 4, 6}`。该代码通过 PC 真值测试；未做开发板验证。
