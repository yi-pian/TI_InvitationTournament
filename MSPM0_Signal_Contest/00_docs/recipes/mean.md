# Mean 平均值 / DC

**等级：CMSIS RECIPE。** 新比赛工程直接调用 SDK CMSIS-DSP；`03_measurement/mean/signal_mean.c/.h` 只为旧工程兼容保留。

## 1. 它解决什么问题

输入：`float samples[N]`，单位可以是 V 或其他明确单位。输出：这 N 点的算术平均值，单位与输入相同。电压波形的平均值通常就是这一帧的 DC。

## 2. 最简单实现

### 比赛现场直接复制这一段

<!-- DIRECT_COPY_BEGIN -->
```c
#include "arm_math.h"

float32_t mean;
arm_mean_f32(x, n, &mean);
```
<!-- DIRECT_COPY_END -->

前提：`x != NULL` 且 `n > 0`。比赛参数是自己定义的固定数组时，在调用前保证这两点即可。

## 3. 这段代码放哪里

放在 `main.c` 的用户代码区，或放在你自己的 `signal_processing.c`。必须等 ADC raw 已经转换成 `voltage_v[]` 后再调用。

## 4. 每一行什么意思

- `arm_mean_f32` 读取 `x[0..n-1]` 并把平均值写入 `mean`。
- `x` 不会被修改；无需 Init、workspace 或 result struct。
- `float32_t` 在当前 CMSIS 头文件中就是单精度浮点类型。

## 5. main / processing 实际例子

```c
#define SIGNAL_SAMPLE_COUNT  (256U)
static float voltage_v[SIGNAL_SAMPLE_COUNT];
static volatile float dc_voltage_v;

static void process_frame(void)
{
    /* 到这里时 voltage_v[] 已由 ADC To Voltage 填好。 */
    arm_mean_f32(voltage_v, SIGNAL_SAMPLE_COUNT, &dc_voltage_v);
    /* 在这里把 dc_voltage_v 交给 UART、TFT 或判限逻辑。 */
}
```

## 6. 题目里需要改什么

只改输入数组和 `N`。输入是 V，输出就是 V；输入是 ADC code，输出只是平均 code，不是电压。

## 7. 什么情况下这种方法会不准

记录太短、信号在帧内漂移、数组尾部没有填满，都会让平均值失真。大量点累加还会产生少量浮点舍入误差，但对常见 12-bit ADC 的几百/几千点比赛测量通常不是主要误差。

## 8. 精度不够怎么办

先增加覆盖时间并确认数组全部有效。动态范围很大或点数极多时，可使用旧兼容 Mean 中的补偿求和，或使用正式 Statistics 模块统一取得均值/方差等多项结果。

## 9. 完整例子

`{1, 2, 3, 4, 5}` 调用 `recipe_mean(x, 5U)` 应得到 `3.0f`。该例是 PC 真值测试的一部分；未做开发板验证。
