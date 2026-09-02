# VPP 峰峰值

**等级：CMSIS RECIPE。** 这是新比赛工程的默认入口；旧 `signal_vpp.c/.h` 仅兼容现有 Application。

## 1. 它解决什么问题

输入：`float voltage_v[N]`。输出：`float vpp_v`，单位 V。Vpp 是当前离散样本中的最大值减最小值。

## 2. 最简单实现

### 比赛现场直接复制这一段

<!-- DIRECT_COPY_BEGIN -->
```c
#include "arm_math.h"

float32_t minimum_v, maximum_v, vpp_v;
uint32_t min_index, max_index;
arm_min_f32(voltage_v, n, &minimum_v, &min_index);
arm_max_f32(voltage_v, n, &maximum_v, &max_index);
vpp_v = maximum_v - minimum_v;
```
<!-- DIRECT_COPY_END -->

前提：`voltage_v != NULL`、`n > 0`、数组已全部填满。

## 3. 这段代码放哪里

放在 ADC 采集完成并转换得到 `voltage_v[]` 之后：

```text
ADC DMA -> ADC To Voltage -> recipe_vpp -> 显示/判限
```

## 4. 每一行什么意思

- CMSIS 分别给出最小值、最大值及对应下标。
- 最后一行做 `maximum_v - minimum_v` 得到峰峰值。
- 无需 Init、context 或 result struct。

## 5. main / processing 实际例子

```c
#define SIGNAL_SAMPLE_COUNT  (1024U)
static float voltage_v[SIGNAL_SAMPLE_COUNT];
static volatile float measured_vpp_v;

static void process_frame(void)
{
    /* voltage_v[] 已准备好。 */
    float32_t minimum_v, maximum_v;
    uint32_t min_index, max_index;
    arm_min_f32(voltage_v, SIGNAL_SAMPLE_COUNT, &minimum_v, &min_index);
    arm_max_f32(voltage_v, SIGNAL_SAMPLE_COUNT, &maximum_v, &max_index);
    measured_vpp_v = maximum_v - minimum_v;
    /* UART_PrintFloat(measured_vpp_v); 或 TFT 显示。 */
}
```

## 6. 题目里需要改什么

改 `N`、输入 buffer 和电压换算参数。Vpp Recipe 本身没有阈值、VREF 或 Fs 参数。

## 7. 什么情况下这种方法会不准

- 一帧没有覆盖真实峰和谷；
- Fs 太低，采样点错过峰顶；
- 孤立毛刺把最大/最小值拉走；
- 前端或 ADC 已削顶，此时得到的是量程边界而非真实 Vpp。

## 8. 精度不够怎么办

先增加合理采样点数和采样率。确认异常点不是目标波形后，再看 Robust Peak-to-Peak、Median/Hampel；真实脉冲和过冲不能随便滤掉。

## 9. 完整例子

输入 `{0.5f, 2.5f, 1.0f, 2.0f}`，结果应为 `2.0 V`。该代码通过 PC 真值测试；未做开发板验证。
