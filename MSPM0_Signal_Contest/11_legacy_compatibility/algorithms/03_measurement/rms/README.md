# RMS：总有效值

> 新比赛工程默认：CMSIS DIRECT。`arm_rms_f32(samples, N, &rms)`；Q15/Q31 版本按数据类型选择。旧 `SignalRMS_Process` 只服务现有兼容 Application。

> **LEVEL A / COMPATIBILITY_API：** 新比赛工程请直接使用详细的 [RMS Recipe](../../00_docs/recipes/rms.md)。本目录复杂状态码/结果结构入口不再是现场默认。

## 新比赛工程：先看这里

**适合：** 已有 `float voltage_v[N]`，题目要包含 DC 分量的总 RMS。只要交流 RMS 时改用 AC RMS。

**不复制本目录源码。** 母版已配置 CMSIS-DSP，直接调用：

```c
#include "arm_math.h"

float32_t total_rms_v;
arm_rms_f32(voltage_v, N, &total_rms_v);
```

**输入 / 输出：** `float voltage_v[N]` -> `total_rms_v`（V）。CMSIS 函数不改输入 Buffer。

| 题目变化 | 修改 |
|---|---|
| 要包含 DC 的有效值 | 保持使用 RMS |
| 只要交流分量 | 换 AC RMS，或先 Remove DC |
| 低频导致读数抖动 | 让采样记录覆盖更多完整周期 |

**Build / 最小验证：** `{1,-1,1,-1}` 应得 1 V。隔离复制工程已 `SysConfig / Compile / Full Link PASS`，Flash 2080 B、SRAM（含栈）521 B。完整代码见 `README_MINIMAL_EXAMPLE.c`。

**连接：** `ADC To Voltage -> RMS -> 显示/功率换算`。常见错误是把 RMS 与 AC RMS 混淆、输入单位不是 V 或 N 为 0。

> 下文旧 `SignalRMS_Process` API 只供维护既有 Application；新工程以上面的 CMSIS 调用为准。

## 你真的需要这个模块吗？

**已有一帧样本，并且要计算包含 DC 的总有效值时需要。** 这是 C `ALGORITHM_MODULE`，只处理内存数据。

## 你应该已经有什么输入数据

`const float samples[N]`；先确认 DC 是否应该计入结果。

## 最短接入步骤

1. **文件：** 不复制本目录源码；include `arm_math.h`。
2. **参数：** 样本数 `N`。
3. **Workspace / Result：** 准备一个 `float32_t` 结果；不需要额外 workspace。
4. **调用：** `arm_rms_f32(samples, N, &result)`。
5. **输出：** 总 RMS，单位与输入相同。
6. **连接下一步：** 显示/判限；只关心交流有效值时改用 AC RMS。
7. **Build / 最小验证：** 常量数组的 RMS 应等于该常量绝对值。

> 算法边界：不配置 Pin，不修改 SysConfig，不调用 DriverLib，也不需要 Platform Adapter。上游硬件变化时，只把真实 `Fs/N/VREF` 等事实同步到算法参数。

## 1 这个算法是干什么的？

RMS（均方根）回答：“这个变化电压产生的平方能量，等效于多大的直流电压？”它适合正弦，也适合方波、三角波和失真波形。

## 2 一个最简单的例子

```text
输入: 3 V, 4 V
平方: 9, 16
平均: (9+16)/2 = 12.5
开方: RMS = sqrt(12.5) ≈ 3.5355 V
```

## 3 原理

`RMS = sqrt(mean(x[n]^2))`。为什么先平方：正负电压不会互相抵消，而且能量/功率通常与电压平方成正比；为什么最后开方：把单位从 V² 变回 V。

本实现用补偿求和累加平方，减少长数组浮点舍入误差。

## 4 比赛里什么时候用？

测总有效值、比较不同波形的等效幅度、判断输出能量。对于 `DC + AC`，总 RMS 同时包含两者。

## 5 输入

`const float *voltage_v`，单位 V；`count>0`。记录应能代表你要报告的时间区间。

## 6 输出

`signal_rms_result_t.rms_v`，单位 V，始终非负。

## 7 API怎么调用

```c
signal_rms_result_t result;
signal_algorithm_status_t status =
    SignalRMS_Process(voltage_v, count, &result);
```

## 8 参数怎么改

没有截止频率或窗口参数。应用层选择 `count` 和是否在前面接 RemoveDC。

## 9 参数改大会怎样

增大 `count` 让稳定周期信号的结果更稳定，尤其当记录覆盖更多完整周期；代价是采集时间、RAM 和延迟变大。若信号随时间变化，长记录得到的是更长时间的能量平均。

## 10 这个算法的代价是什么

Benefits：数学含义明确、支持任意波形、RAM 常数。

Trade-offs：异常大点经过平方影响更强；软件浮点乘法/开方耗时；不会自动移除 DC。

## 11 什么时候不要用

- 只想测 AC 而输入带 1.65 V 偏置：改用 AC_RMS 或先 RemoveDC。
- 想测峰值/Vpp：RMS 不能代替峰值。
- 单次突发不能代表长期信号时，不应把结果解释成稳态 RMS。

## 12 怎么和前一个模块接

```text
总 RMS: ADC_ToVoltage ─────────> RMS
AC RMS: ADC_ToVoltage -> RemoveDC -> RMS
```

## 13 怎么和后一个模块接

```text
┌──────── RMS ─────────┐
│ voltage_v[]          │
│ square -> mean -> sqrt│
│ rms_v                │
└──────────┬───────────┘
           ↓
  显示 / 增益 / 功率换算
```

若后续要换算功率，还必须知道负载模型；本模块只输出电压 RMS。

## 14 最小Demo

```c
const float x_v[] = {3.0f, 4.0f};
signal_rms_result_t r;
(void)SignalRMS_Process(x_v, 2U, &r); /* 约 3.5355 V */
```

## 15 PC测试

已测试带 DC 正弦：DC=1.65 V、峰值=0.5 V。Expected 总 RMS=`sqrt(1.65²+0.5²/2)=1.687453628 V`，Measured 相同，PASS。

排查：结果比 AC 理论值大，先检查 DC；异常偏大检查毛刺/削顶；偏小检查 ADC 比例和记录是否代表完整周期。

## 16 MCU资源

O(N) 乘加 + 一次 `sqrtf`，内部 O(1) RAM，无动态内存。Cortex‑M0+ 无 FPU，高吞吐链应按帧调用并测量周期。

## 17 验证状态

PC_VERIFIED：2026-08-07，GCC C11 严格编译与总 RMS 真值测试通过；未 BOARD_VERIFIED。

## 18. 完整 API、调用顺序与 Buffer 规则

`SignalRMS_Process(voltage_v, count, result)` 是唯一公开 API：`voltage_v` 为只读 V 数组，`count>0`，`result` 非空。每帧同步调用一次；成功写 `result.rms_v` 并返回 OK，非法指针/长度/非有限样本返回错误。模块不修改输入、不需要 workspace/Init，可与 VPP、Mean 并行复用同一帧。

```text
ADC To Voltage -> RMS（总RMS，含DC） -> rms_v
ADC To Voltage -> RemoveDC -> RMS（交流近似）
```

```c
signal_rms_result_t rms;
if (SignalRMS_Process(voltage_v, N, &rms) == SIGNAL_ALGORITHM_OK) {
    /* rms.rms_v 单位 V，不是功率 */
}
```

全部 CONFIG ONLY；SysConfig Not Applicable。

## 19. Common Modification / Result Meaning

唯一常改的是 `count` 和前面是否 RemoveDC。N 增大通常让周期平均更稳，但延迟增加；输入 buffer RAM 由上游决定，本模块 O(1)。`rms_v` 包含 DC；要 AC-only 用 AC RMS。把电压 RMS 换成功率还需要负载阻抗/电路模型，本模块不暴露该参数。

## 20. Quick Modify Table

| 我想改什么 | 去哪里 | 改什么 | 影响 | SysConfig? |
|---|---|---|---|---|
| 含/不含DC | 模块选择/链路 | RMS 或 AC RMS/RemoveDC | 结果物理意义 | 否 |
| 观测长度 | Application | count/N | 稳定性、延迟 | 否 |
| 电压标定 | ADC To Voltage | VREF/scale/offset | RMS 比例 | 硬件变化时是 |
| 输入通道 | Acquisition `.syscfg` | ADC channel | 信号来源 | 是 |

## API Reference

`SignalRMS_Process(voltage_v, count, result)`：输出包含 DC 的 `result->rms_v`。
