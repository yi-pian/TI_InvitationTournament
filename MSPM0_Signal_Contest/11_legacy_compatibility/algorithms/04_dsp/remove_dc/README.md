# RemoveDC：去掉整段信号的平均偏置

> 新比赛工程默认：CMSIS RECIPE。`arm_mean_f32(x,N,&mean)` 后 `arm_offset_f32(x,-mean,y,N)`。它只是逐帧去均值，不等同于跟踪漂移的高通滤波器。

> **LEVEL A / COMPATIBILITY_API：** 新比赛工程请直接使用详细的 [Remove DC Recipe](../../00_docs/recipes/remove_dc.md)。它允许原地处理且不需要 result struct；旧 API 只为兼容保留。

## 新比赛工程：先看这里

**适合：** 已有 `float voltage_v[N]`，要在 FFT、过零或相关之前减去这一帧的平均 DC。

**不复制本目录源码。** 母版已配置 CMSIS-DSP，直接调用：

```c
#include "arm_math.h"

static float centered_v[N];
float32_t removed_dc_v;
arm_mean_f32(voltage_v, N, &removed_dc_v);
arm_offset_f32(voltage_v, -removed_dc_v, centered_v, N);
```

**输入 / 输出：** `float voltage_v[N]` -> `float centered_v[N]` + 被减掉的均值 V。允许输入输出指向同一数组，原地处理会覆盖原电压。

| 题目变化 | 修改 |
|---|---|
| 想省一组 Buffer | 输入输出传同一指针 |
| DC 缓慢漂移 | 调整每帧 N/更新时间，并验证均值是否跟得上 |
| 需要保留 DC 测量 | 读取 `removed_mean_v`，或另存原数组 |

**Build / 最小验证：** `{1,2,1,2}` 输出均值应接近 0，被减均值为 1.5。隔离复制工程已 `SysConfig / Compile / Full Link PASS`，Flash 1288 B、SRAM（含栈）533 B。完整代码见 `README_MINIMAL_EXAMPLE.c`。

**连接：** `ADC To Voltage -> Remove DC -> Window / Zero Cross / Correlation`。它只去常量均值，不是高通滤波器。常见错误是原地覆盖后又想读原 DC 波形。

> 下文旧 `SignalRemoveDC_Process` API 只供维护既有 Application；新工程以上面的 CMSIS Recipe 为准。

## 你真的需要这个模块吗？

**已有一帧 `float` 数据，并且后续过零、FFT 或 AC 测量不希望被均值干扰时需要。** 这是 C `ALGORITHM_MODULE`，只处理内存数据。

## 你应该已经有什么输入数据

`const float input[N]`；若 DC 本身也要测量，先保存 Mean/DC 结果。

## 最短接入步骤

1. **文件：** 新工程不复制本目录源码；include `arm_math.h`。只有维护旧 Application 才读取下文兼容 API。
2. **参数：** 样本数 `N`。
3. **Workspace / Result：** 准备 `float output[N]` 和 `signal_remove_dc_result_t result`；API 支持时可按文档确认是否原地处理。
4. **调用：** `SignalRemoveDC_Process(input, output, N, &result)`。
5. **输出：** 去均值后的 AC buffer，以及被移除的 DC/mean。
6. **连接下一步：** Zero Cross、Window/FFT、AC RMS。
7. **Build / 最小验证：** 常量数组去 DC 后应接近全 0，result 中均值应接近原常量。

> 算法边界：不配置 Pin，不修改 SysConfig，不调用 DriverLib，也不需要 Platform Adapter。上游硬件变化时，只把真实 `Fs/N/VREF` 等事实同步到算法参数。

## 1 这个算法是干什么的？

如果正弦被抬到 1.65 V，它并不围绕 0 V 摆动。RemoveDC 先求平均值，再从每个样本减去平均值，让输出围绕 0 V，便于过零、FFT 和交流分析。

## 2 一个最简单的例子

```text
输入: 1, 2, 3, 2 V
平均: 2 V
输出: -1, 0, +1, 0 V
```

## 3 原理

`y[n] = x[n] - mean(x)`。这只删除频率为 0 的常量项。它不是高通滤波器，也不能很好地消除整段中的线性漂移；后者应考虑 Detrend。

## 4 比赛里什么时候用？

带中点偏置的 ADC 信号做 ZeroCross、FFT、相关或 AC RMS 前。若 FFT 需要观察 DC 分量，则不要去掉。

## 5 输入

`const float *input_voltage_v`，单位 V；`count>0`。可以与输出指向同一数组。

## 6 输出

- `output_centered_v[count]`：均值约为 0 的电压，仍然是 V。
- `removed_mean_v`：被减去的平均电压，V。

## 7 API怎么调用

```c
signal_remove_dc_result_t result;
SignalRemoveDC_Process(voltage_v, centered_v, count, &result);

/* 节省 RAM 的原地版本 */
SignalRemoveDC_Process(voltage_v, voltage_v, count, &result);
```

## 8 参数怎么改

没有滤波参数。选择在何处放置该积木，以及使用多长的 `count`。若记录包含非整数周期，均值可能不等于真实 DC，可增加完整周期或改用带模型的算法。

## 9 参数改大会怎样

`count` 增大时，对稳定 DC 的估计通常更稳，但延迟增加；若 DC 在记录内漂移，一个常量均值无法贴合每个时刻。

## 10 这个算法的代价是什么

Benefits：消除中点偏置；减少 FFT 的 DC 峰；让 0 V 过零阈值有意义；支持原地省 RAM。

Trade-offs：原地会永久覆盖原电压；删掉真实 DC；两遍扫描；不处理趋势。

## 11 什么时候不要用

- 测 DC/总 RMS；
- 频谱中要保留 DC；
- 单次瞬态的平均值本身具有物理意义；
- 明显线性漂移，需要 Detrend 而非仅减常量。

## 12 怎么和前一个模块接

```text
ADC_DMA RAW -> ADC_ToVoltage (含偏置 V) -> RemoveDC
```

## 13 怎么和后一个模块接

```text
┌───────────────────────┐
│       Remove DC       │
├───────────────────────┤
│ Input: voltage_v[]    │
│ calculate mean        │
│ y[n] = x[n] - mean    │
│ Output: centered_v[]  │
└───────────┬───────────┘
            ├──> RMS
            ├──> ZeroCross
            └──> Window -> FFT
```

## 14 最小Demo

```c
float x_v[] = {1, 2, 3, 2};
signal_remove_dc_result_t r;
(void)SignalRemoveDC_Process(x_v, x_v, 4U, &r);
/* x_v 变为 {-1,0,1,0}，r.removed_mean_v=2 */
```

## 15 PC测试

对 DC=1.65 V、峰值=0.5 V 的完整周期正弦处理。Expected removed mean=1.65 V；输出平均绝对值误差约 `3.064e-8 V`，PASS。

排查：输出仍明显偏离 0，检查是否混入未填充尾部、NaN 或记录太短；原数据意外丢失，检查是否使用了原地模式。

## 16 MCU资源

时间 O(2N)，内部 RAM O(1)。非原地需要 `4*N` 字节输出；原地不需额外数组。Cortex‑M0+ 使用软件浮点。

## 17 验证状态

PC_VERIFIED：2026-08-07，GCC C11 严格编译、偏置正弦与输出均值测试通过；未 BOARD_VERIFIED。

## 18. 完整 API、调用顺序与 Buffer 规则

`SignalRemoveDC_Process(input_voltage_v, output_centered_v, count, result)` 是唯一公开 API：输入/输出各至少 `count` 个 float，单位 V；`count>0`；result 非空。输入输出可完全相同以原地处理；除此之外不要部分重叠。成功写 centered 数据和 `removed_mean_v`；非法指针/长度/非有限样本返回错误。

```text
Voltage ready -> RemoveDC -> 检查 OK -> centered -> ZeroCross/Window/FFT
```

```c
signal_remove_dc_result_t dc;
if (SignalRemoveDC_Process(voltage, voltage, N, &dc)
        == SIGNAL_ALGORITHM_OK) {
    /* voltage 已被覆盖；dc.removed_mean_v 保留原均值 */
}
```

## 19. Modification / Algorithm Scope

没有截止频率或阈值参数；只改 count 和是否原地。原地 RAM 最省但原电压丢失，不能随后用它测 DC/总 RMS；非原地额外 `4N` bytes。

结果仍是 V，`removed_mean_v` 是当前帧均值。该函数不是高通滤波器，无法消除斜坡漂移。

## 20. Quick Modify Table

| 我想改什么 | 去哪里 | 改什么 | 影响 | 是否需同步上游 |
|---|---|---|---|---|
| 保留原电压 | buffer/call | 使用不同 output | 额外 `4N` RAM | 否 |
| 最省 RAM | call | input==output | 覆盖原帧 | 否 |
| 观测长度 | Application | count | 均值估计/延迟 | 否 |
| 保留 DC 频谱 | 模块链 | 跳过 RemoveDC | DC bin 保留 | 否 |

## API Reference

`SignalRemoveDC_Process(input_voltage_v, output_centered_v, count, result)`：支持输入输出同址。
