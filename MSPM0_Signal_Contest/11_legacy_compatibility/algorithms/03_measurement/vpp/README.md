# Vpp：峰峰值测量

> 新比赛工程默认：CMSIS RECIPE，即 `arm_max_f32 + arm_min_f32 + maximum - minimum`。普通 Vpp 不再作为正式算法核心；强毛刺场景使用 `robust_peak_to_peak`。

> **LEVEL A / COMPATIBILITY_API：** 新比赛工程请直接使用详细的 [Vpp Recipe](../../00_docs/recipes/vpp.md)。它给出完整可复制代码、放置位置、参数、误差和真值例子；本目录 `.c/.h` 只兼容旧 Application。

## 新比赛工程：先看这里

**适合：** 已有一帧 `float voltage_v[N]`，题目要离散样本的最大值、最小值和 Vpp。

**不复制本目录源码。** 母版已配置 CMSIS-DSP，直接调用：

```c
#include "arm_math.h"

float32_t minimum_v, maximum_v, value_vpp;
uint32_t min_index, max_index;
arm_min_f32(voltage_v, N, &minimum_v, &min_index);
arm_max_f32(voltage_v, N, &maximum_v, &max_index);
value_vpp = maximum_v - minimum_v;
```

**输入 / 输出：** `float voltage_v[N]` -> `vpp.min_voltage_v`、`max_voltage_v`、`amplitude_vpp`，单位均为 V。

| 题目变化 | 修改 |
|---|---|
| 波形周期更长 | 增大上游采样记录，使它覆盖峰和谷 |
| 刷新要更快 | 减小 N，但仍须覆盖峰/谷 |
| 孤立毛刺拉高结果 | 先确认毛刺不是有效信号，再考虑 Robust VPP/Hampel |

**Build / 最小验证：** `{0.5, 2.5, 1.0, 2.0}` 应得 min=0.5、max=2.5、Vpp=2.0 V。隔离复制工程已 `SysConfig / Compile / Full Link PASS`，Flash 1152 B、SRAM（含栈）525 B。完整代码见 `README_MINIMAL_EXAMPLE.c`。

**连接：** `ADC DMA -> ADC To Voltage -> VPP -> 显示/阈值判断`。常见错误是输入仍为 raw code、记录没覆盖完整波形、把单点毛刺当峰值。

> 下文旧 `SignalVPP_Process` API 只供维护既有 Application；新工程以上面的 CMSIS Recipe 为准。

## 你真的需要这个模块吗？

**已有一帧电压，并且要计算峰峰值时需要。** 这是 C `ALGORITHM_MODULE`，只处理内存数据。

## 你应该已经有什么输入数据

`const float voltage_v[N]`；若仍是 ADC code，先接 ADC To Voltage。

## 最短接入步骤

1. **文件：** 不复制本目录源码；include `arm_math.h`。
2. **参数：** 样本数 `N`。
3. **Workspace / Result：** 准备 min/max、两个下标和 Vpp 标量；无需数组 workspace。
4. **调用：** `arm_min_f32`、`arm_max_f32`，然后 `maximum_v-minimum_v`。
5. **输出：** `result.min_voltage_v`、`result.max_voltage_v`、`result.amplitude_vpp`，单位 V。
6. **连接下一步：** 结果显示/判限；毛刺污染明显时改用 Robust VPP。
7. **Build / 最小验证：** 输入一个已知最小值/最大值数组，核对结果等于 `max-min`。

> 算法边界：不配置 Pin，不修改 SysConfig，不调用 DriverLib，也不需要 Platform Adapter。上游硬件变化时，只把真实 `Fs/N/VREF` 等事实同步到算法参数。

## 1 这个算法是干什么的？

Vpp 表示波形从最低点到最高点一共跨了多少伏。它直接计算 `max_voltage_v - min_voltage_v`。

## 2 一个最简单的例子

```text
电压: 1.0, 1.5, 2.0, 1.5 V
min = 1.0 V, max = 2.0 V
Vpp = 1.0 V
```

## 3 原理

扫描数组找最小和最大样本，再相减。对理想 `DC + A*sin()`，理论 Vpp 为 `2*A`。数字采样点不一定正好落在峰顶，因此测量值可能略低。

## 4 比赛里什么时候用？

测正弦、方波、三角波的电压摆幅，或快速判断输出范围。它不要求波形一定是正弦。

## 5 输入

`const float *voltage_v`，单位 V；`count>0`，且记录应覆盖真实最高/最低位置。

## 6 输出

`amplitude_vpp`、`min_voltage_v`、`max_voltage_v`，全部单位 V。

## 7 API怎么调用

```c
signal_vpp_result_t result;
if (SignalVPP_Process(voltage_v, count, &result) == SIGNAL_ALGORITHM_OK) {
    float amplitude_vpp = result.amplitude_vpp;
}
```

## 8 参数怎么改

模块没有平滑参数。主要改变 `count` 和记录起点；有毛刺不能靠“调 Vpp 参数”解决，应先判断毛刺是否有效信号，再选择 RobustVPP。

## 9 参数改大会怎样

`count` 变大更容易覆盖峰谷，但也更容易遇到偶发异常点；测量延迟和采集 RAM 同时增加。

## 10 这个算法的代价是什么

Benefits：不限波形、一次扫描、无需三角函数。

Trade-offs：一个毛刺能把结果拉得很大；采样率不足会错过峰顶；不自动判断削顶。

## 11 什么时候不要用

- 有孤立异常值而又不能确认其来源；
- 单次记录只覆盖半个周期；
- 想从已削顶波形恢复原幅度；
- 想测功率等效值，此时用 RMS。

## 12 怎么和前一个模块接

```text
ADC_DMA -> ADC_ToVoltage -> Vpp
```

## 13 怎么和后一个模块接

```text
┌────────── Vpp ──────────┐
│ voltage_v[] + count     │
│ min / max               │
│ amplitude_vpp = max-min │
└────────────┬────────────┘
             ↓
       显示 / 量程判断
```

可以并行对同一 `voltage_v[]` 调用 RMS 和 ClippingDetect，它们互不修改输入。

## 14 最小Demo

```c
const float x_v[] = {1.0f, 1.5f, 2.0f, 1.5f};
signal_vpp_result_t r;
(void)SignalVPP_Process(x_v, 4U, &r); /* r.amplitude_vpp == 1.0 V */
```

## 15 PC测试

合成 `Fs=100 kHz, f=1 kHz, peak=0.5 V, DC=1.65 V` 的 10 周期正弦。Expected Vpp=1.0 V，绝对误差 `1.192e-7 V`，PASS。

排查：结果过大看原始数组是否有毛刺；过小检查记录是否覆盖峰谷、采样率是否足够、前端是否限幅。

## 16 MCU资源

时间 O(N)，内部 RAM O(1)，无动态内存。输入缓冲区由上游拥有。

## 17 验证状态

PC_VERIFIED：2026-08-07，严格编译和合成正弦真值测试通过；未实板验证。

## 18. 完整 API、调用顺序与 Buffer 规则

唯一公开函数 `SignalVPP_Process(voltage_v, count, result)` 在一帧 voltage ready 后同步调用一次：`voltage_v` 为只读 `float[count]`（V），`count>0`，`result` 非空。成功返回 OK 并写 `min_voltage_v`、`max_voltage_v`、`amplitude_vpp=max-min`；空指针/零长度或非有限样本返回对应算法错误。它不修改输入、不需要 workspace、允许同一输入继续做 RMS/AC RMS。

```text
ADC To Voltage -> SignalVPP_Process -> 检查 OK -> result.amplitude_vpp
```

Realistic glue：

```c
signal_vpp_result_t vpp;
if (SignalADCToVoltage_Process(raw, voltage, N, &scale) == SIGNAL_ALGORITHM_OK &&
    SignalVPP_Process(voltage, N, &vpp) == SIGNAL_ALGORITHM_OK) {
    /* vpp.amplitude_vpp 单位 V */
}
```

全部 CONFIG ONLY；SysConfig Not Applicable。真正的 ADC 通道/VREF 在上游配置。

## 19. Common Modification / Result Meaning

本模块没有阈值或窗口参数。N 增大：更可能覆盖峰谷，也更可能纳入孤立毛刺；RAM 增量来自上游输入，不来自本模块。结果是当前离散样本的 max-min，不会插值恢复采样点之间的峰值。若毛刺不是有效信号，改用 Robust VPP 或先做有依据的异常点处理。

## 20. Quick Modify Table

| 我想改什么 | 去哪里 | 改什么 | 影响 | SysConfig? |
|---|---|---|---|---|
| 观察长度 | Acquisition/Application | N/count | 峰谷覆盖、毛刺概率、延迟 | 否 |
| 电压比例 | ADC To Voltage config | VREF/scale/offset | min/max/Vpp 单位准确性 | 硬件变化时是 |
| 抗孤立毛刺 | 模块链 | 换 Robust VPP/Hampel（确认不会删真峰） | 稳健性/峰值含义 | 否 |
| ADC 通道 | 上游 `.syscfg` | channel/pin | 输入信号 | 是 |

## API Reference

`SignalVPP_Process(voltage_v, count, result)`：输出 `min_voltage_v`、`max_voltage_v`、`amplitude_vpp`。
