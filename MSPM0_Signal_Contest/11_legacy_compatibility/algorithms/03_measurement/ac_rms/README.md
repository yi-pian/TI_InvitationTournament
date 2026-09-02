# AC_RMS：只测交流有效值

> 新比赛工程默认：CMSIS RECIPE。`arm_mean_f32` 求 DC，`arm_offset_f32` 去 DC，再 `arm_rms_f32`；完整 buffer 代码见 CMSIS Cookbook。

> **LEVEL A / COMPATIBILITY_API：** 新比赛工程请直接使用详细的 [AC RMS Recipe](../../00_docs/recipes/ac_rms.md)。旧 API 继续供现有 Application 构建，不建议新工程复制。

## 比赛复制版：先看这里

**适合：** 输入带偏置，但题目只要交流分量 RMS。模块内部先求均值再计算去均值后的 RMS。

**复制到 `modules/`：** `signal_ac_rms.c`、`signal_ac_rms.h`、`signal_algorithm_status.h`、`signal_math_backend.h`、`signal_math_backend_config.h`。后三个来自 `03_measurement/common/`。不需要 SysConfig/Pin，默认 backend 不需要 IQMath。

```c
#include "signal_ac_rms.h"

signal_ac_rms_result_t ac;
signal_algorithm_status_t status = SignalACRMS_Process(voltage_v, N, &ac);
if (status == SIGNAL_ALGORITHM_OK) {
    float dc_v = ac.mean_voltage_v;
    float ac_rms_v = ac.ac_rms_v;
    // ===== 这里写你自己的逻辑 =====
}
```

**输入 / 输出：** `float voltage_v[N]` -> 平均值 `mean_voltage_v` 和交流有效值 `ac_rms_v`，单位 V；不改输入 Buffer。

| 题目变化 | 修改 |
|---|---|
| 只要总 RMS | 改用 RMS |
| 低频读数不稳 | 增大记录，使其覆盖完整周期 |
| 偏置缓慢漂移 | 缩短/滑动记录，并验证估计偏差 |

**Build / 最小验证：** `{2,0,2,0}` 应得 mean=1 V、AC RMS=1 V。隔离复制工程已 `SysConfig / Compile / Full Link PASS`，Flash 2136 B、SRAM（含栈）525 B。完整代码见 `README_MINIMAL_EXAMPLE.c`。

**连接：** `ADC To Voltage -> AC RMS -> 显示`。常见错误是记录不足一个周期、把 `mean_voltage_v` 当 AC RMS，或先 Remove DC 后又期待这里返回原 DC。

> 下文保留详细算法说明；比赛 COPY 以本节为准。

## 你真的需要这个模块吗？

**已有一帧样本，并且要排除 DC 后计算交流有效值时需要。** 这是 C `ALGORITHM_MODULE`，只处理内存数据。

## 你应该已经有什么输入数据

`const float samples[N]`。

## 最短接入步骤

1. **文件：** 复制本节顶部清单到 `modules/`，include `signal_ac_rms.h`；无需另加算法仓库 Include Path。
2. **参数：** 样本数 `N`。
3. **Workspace / Result：** 准备 `signal_ac_rms_result_t result`；不需要额外 workspace。
4. **调用：** `SignalACRMS_Process(samples, N, &result)`。
5. **输出：** 均值/DC 与 AC RMS，单位与输入相同。
6. **连接下一步：** 显示/判限；需要包含 DC 的总有效值时使用 RMS。
7. **Build / 最小验证：** 常量数组的 AC RMS 应接近 0。

> 算法边界：不配置 Pin，不修改 SysConfig，不调用 DriverLib，也不需要 Platform Adapter。上游硬件变化时，只把真实 `Fs/N/VREF` 等事实同步到算法参数。

## 1 这个算法是干什么的？

ADC 常把双极性信号抬到 1.65 V。总 RMS 会把这 1.65 V 也算进去；AC_RMS 先求这段数据的平均值，再计算每个样本相对平均值的 RMS，所以输出只代表交流变化。

## 2 一个最简单的例子

```text
输入: 1 V, 3 V
平均/DC: 2 V
去均值: -1 V, +1 V
AC RMS: sqrt((1²+1²)/2) = 1 V
```

## 3 原理

先算 `mean`，再算 `sqrt(mean((x[n]-mean)^2))`。两遍扫描避免先创建一份去直流数组。对于完整周期正弦，峰值 A 的 AC RMS 为 `A/sqrt(2)`。

## 4 比赛里什么时候用？

测带偏置正弦、三角波、方波的交流有效值；同时想得到 DC 与 AC RMS，但不想额外占 RemoveDC 输出 RAM。

## 5 输入

`const float *voltage_v`，单位 V，`count>0`。记录最好覆盖整数个或多个完整周期，否则均值可能包含周期截断误差。

## 6 输出

- `mean_voltage_v`：这段记录的平均/DC，V。
- `ac_rms_v`：相对该均值的 RMS，V。

## 7 API怎么调用

```c
signal_ac_rms_result_t result;
if (SignalACRMS_Process(voltage_v, count, &result)
        == SIGNAL_ALGORITHM_OK) {
    /* result.mean_voltage_v 和 result.ac_rms_v 可用 */
}
```

## 8 参数怎么改

没有滤波参数。通过记录起点和 `count` 控制观测时长；周期已知时尽量取整数周期。

## 9 参数改大会怎样

更多完整周期通常让 DC 与 AC RMS 更稳定；代价是延迟增加。若包含缓慢变化，长记录会把部分低频变化纳入/排除的边界变得依赖记录长度。

## 10 这个算法的代价是什么

Benefits：O(1) 额外 RAM；不用保留 centered 数组；同时输出 DC。

Trade-offs：读取输入两遍；平均值定义的“DC”只针对当前记录；软件浮点和开方有 CPU 成本。

## 11 什么时候不要用

- 需要包含 DC 的总 RMS：用 RMS。
- 后续还要 FFT/ZeroCross 的 centered 数组：先 RemoveDC，再复用输出可能更省总计算。
- 单次瞬态：去均值后的 RMS 仅描述该记录，不能当稳态 AC。

## 12 怎么和前一个模块接

```text
ADC_DMA -> ADC_ToVoltage -> AC_RMS
```

## 13 怎么和后一个模块接

```text
┌──────── AC_RMS ────────┐
│ voltage_v[]            │
│ mean -> deviations²    │
│ mean_voltage_v         │
│ ac_rms_v               │
└──────────┬─────────────┘
           ↓
  显示 / 增益 / 误差比较
```

本模块不输出 centered 数组，不能直接把 result 接给 FFT。

## 14 最小Demo

```c
const float x_v[] = {1.0f, 3.0f};
signal_ac_rms_result_t r;
(void)SignalACRMS_Process(x_v, 2U, &r);
/* mean=2 V, ac_rms=1 V */
```

## 15 PC测试

`Fs=100 kHz, f=1 kHz, peak=0.5 V, DC=1.65 V`，1000 点覆盖 10 周期。Expected DC=1.65 V、AC RMS=0.3535533906 V，Measured 在 `2e-6 V` 容差内，PASS。

排查：AC RMS 偏差先检查是否覆盖完整周期、ADC 是否削顶、VREF/比例是否正确；DC 飘动时比较多帧均值。

## 16 MCU资源

时间 O(2N)，内部 O(1) RAM，无动态内存。比 `RemoveDC + RMS` 少一个输出 buffer，但若 centered 数据还要复用，后者可能整体更划算。

## 17 验证状态

PC_VERIFIED：2026-08-07，严格编译和偏置正弦真值测试通过；未实板验证。

## 18. 完整 API、调用顺序与 Buffer 规则

唯一公开函数 `SignalACRMS_Process(voltage_v, count, result)`：输入只读 `float[count]` V、`count>0`、result 非空；同步两遍扫描，无 Init。成功写当前记录均值 `mean_voltage_v` 与相对均值的 `ac_rms_v`；非法输入/非有限值返回错误。输入不被修改，无 workspace/动态内存。

```text
ADC To Voltage -> AC RMS -> {mean_voltage_v, ac_rms_v}
```

```c
signal_ac_rms_result_t ac;
if (SignalACRMS_Process(voltage_v, N, &ac) == SIGNAL_ALGORITHM_OK) {
    /* 同时得到当前帧 DC 与 AC RMS */
}
```

全部 CONFIG ONLY；SysConfig Not Applicable。若后续还要 centered 数组，改为 RemoveDC 原地/非原地后再 RMS，避免重复计算。

## 19. Common Modification / Result Meaning

只改 count/帧起点。N 增大能覆盖更多周期但不能自动消除非整数周期截断；`mean_voltage_v` 是这段记录的平均值，不保证等于长期 DC。输入 buffer 可在调用后继续用于其他算法，本模块 O(1) RAM。

## 20. Quick Modify Table

| 我想改什么 | 去哪里 | 改什么 | 影响 | SysConfig? |
|---|---|---|---|---|
| 总 RMS | 模块选择 | 改用 RMS | 包含 DC | 否 |
| 观测长度 | Application | count/N | 周期覆盖、延迟 | 否 |
| 保留去DC数组 | 模块链 | RemoveDC -> RMS | 多一块或原地 buffer | 否 |
| 电压标定/通道 | 上游 ADC 链 | scale/channel | 数值/来源 | 改硬件时是 |

## API Reference

`SignalACRMS_Process(voltage_v, count, result)`：输出 `mean_voltage_v` 与 `ac_rms_v`。
