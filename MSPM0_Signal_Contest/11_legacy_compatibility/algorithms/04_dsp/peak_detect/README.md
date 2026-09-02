# PeakDetect：在指定频带找最大 bin

> **LEVEL A / COMPATIBILITY_API：** 普通最大 bin 搜索请直接使用详细的 [Peak Detect Recipe](../../00_docs/recipes/peak_detect.md)。需要多峰、最小间距或复杂判定时再建立题目专用逻辑；旧 API 仅兼容现有工程。

## 比赛复制版：先看这里

**适合：** 已有 magnitude/energy 数组，要在已知索引范围找最大值。它返回 bin index，不直接知道 Fs。

**复制到 `modules/`：** `signal_peak_detect.c`、`signal_peak_detect.h` 和 `03_measurement/common/signal_algorithm_status.h`。无 SysConfig/Pin。

```c
#include "signal_peak_detect.h"

signal_peak_detect_result_t peak;
signal_algorithm_status_t status = SignalPeakDetect_Process(
    magnitude, N / 2U + 1U, start_bin, end_bin, &peak);
if (status == SIGNAL_ALGORITHM_OK) {
    float coarse_frequency_hz = (float)peak.peak_index * Fs / (float)N;
    // ===== 这里写你自己的逻辑，或接 FFT Interpolation =====
}
```

**输入 / 输出：** `float values[count]` + 闭区间 `[start_index,end_index]` -> `peak_index` 和 `peak_value`。通常 `start_bin=1` 排除 DC。

| 题目变化 | 修改 |
|---|---|
| 限定搜索频带 | `start_bin=ceil(fmin*N/Fs)`，`end_bin=floor(fmax*N/Fs)` |
| DC 很强 | 从 bin 1 或更高开始 |
| 要亚 bin 精度 | 把 `peak_index` 交给 FFT Interpolation |

**Build / 最小验证：** `{0,1,4,2,0.5}` 在 1～4 搜索应返回 index=2/value=4。隔离复制工程已 `SysConfig / Compile / Full Link PASS`，Flash 952 B、SRAM（含栈）521 B。完整代码见 `README_MINIMAL_EXAMPLE.c`。

常见错误：Hz 和 bin 混用、`end_index>=count`、搜索区包含不想要的 DC/谐波、平顶峰却当成唯一频率。

> 下文保留详细范围规则/API；比赛 COPY 以本节为准。

## 你真的需要这个模块吗？

**已有 magnitude 频谱，并且要在指定频带找整数 bin 主峰时需要。** 这是 C `ALGORITHM_MODULE`，只处理内存数据。

## 你应该已经有什么输入数据

`const float magnitude[bin_count]`，以及已经换算好的搜索起点/终点 bin。

## 最短接入步骤

1. **文件：** 复制顶部清单到 `modules/`，include `signal_peak_detect.h`；无需另加算法仓库 Include Path。
2. **参数：** `bin_count`、`start_index`、`end_index`（闭区间）。
3. **Workspace / Result：** 准备 `signal_peak_detect_result_t result`；不需要大 workspace。
4. **调用：** `SignalPeakDetect_Process(magnitude, bin_count, start_index, end_index, &result)`。
5. **输出：** `result.peak_index` 和 `result.peak_value`；频率由下游按 `peak_index * Fs / N` 换算。
6. **连接下一步：** FFT Parabolic/Log Parabolic Interpolation，或 Harmonic。
7. **Build / 最小验证：** 人工构造一个已知最大 bin 的 magnitude 数组，结果必须返回该 bin；边界 bin 要按 API 限制处理。

> 算法边界：不配置 Pin，不修改 SysConfig，不调用 DriverLib，也不需要 Platform Adapter。上游硬件变化时，只把真实 `Fs/N/VREF` 等事实同步到算法参数。

## 1 这个算法是干什么的？

在 start~end bin 内找最大值及索引。

## 2 一个最简单的例子

`[0,2,5,3]` 搜 1~3，峰为 index2/value5。

## 3 原理

一次线性比较；相等保留第一次出现。

## 4 比赛里什么时候用？

已知搜索频带内找基波/杂散候选。

## 5 输入

float 值、总长度、包含式 start/end。

## 6 输出

peak_index（bin）与 peak_value（输入谱单位）。

## 7 API怎么调用

`SignalPeakDetect_Process(mag,bins,1,bins-2,&r);`

## 8 参数怎么改

start=1 常用于排 DC；按题目限制 end 避开 Nyquist/无关频带。

## 9 参数改大会怎样

搜索范围变宽更可能找到未知峰，也更可能把强干扰当目标。

## 10 这个算法的代价是什么

Benefits：简单可控。Trade-offs：不判断谐波关系、噪声阈值或局部显著性。

## 11 什么时候不要用

基波不是全局最大、需要多个峰、或 DC 未排除时无脑全谱搜索。

## 12 怎么和前一个模块接

`Magnitude -> PeakDetect`

## 13 怎么和后一个模块接

`Peak index -> ParabolicInterpolation / Harmonic`

## 14 最小Demo

```c
signal_peak_detect_result_t r;
(void)SignalPeakDetect_Process(m, count, 1U, count-2U, &r);
```

## 15 PC测试

已知 peak index10 与两条 FFT 正弦均 PASS。

## 16 MCU资源

O(range)，O(1)，只比较。

## 17 验证状态

PC_VERIFIED；未实板。

## 18. 完整 API、调用顺序与 Buffer 规则

唯一公开函数 `SignalPeakDetect_Process(values, count, start_index, end_index, result)`：values 为只读非负幅值/能量数组；搜索闭区间 `[start,end]` 必须落在 `0..count-1`；result 非空。成功返回首次出现的最大值 `peak_index/peak_value`；范围/数值非法返回错误。无 Init/workspace，不修改输入。

```text
Magnitude/GainCorrection -> Peak Detect -> integer peak bin
                                         -> Parabolic Interpolation
```

```c
status = SignalPeakDetect_Process(magnitude, bin_count,
    first_bin, last_bin, &peak);
```

## 19. Parameter / Result Meaning / Config

start 常设 1 排除 DC；end 常由 expected_max_hz 换成 bin 并限制到 Nyquist。范围增大能找到更多候选但可能选择无关杂散；缩小需要事先知道目标频带。`peak_value` 单位完全继承输入；整数 bin 的 Hz 为 `peak_index*Fs/N`。

全部 CONFIG ONLY；SysConfig Not Applicable。常见错误：end 当成不包含端点、未排除 DC、搜索范围越界、对 dB/energy/magnitude 混淆单位、把整数 bin 当精确频率、边界峰继续做三点插值。

## 20. Quick Modify Table

| 我想改什么 | 去哪里 | 改什么 | 影响 | SysConfig? |
|---|---|---|---|---|
| 排除DC | call | `start_index=1` | 不选bin0 | 否 |
| 搜索频带 | Application换算/call | start/end bin | 候选峰 | 否 |
| 更细频率 | 后续模块 | Parabolic/Log interpolation | fractional bin | 否 |
| 多个峰 | 上层循环/集成Glue | 屏蔽已选邻域再找 | 结果数量 | 否 |

## API Reference

`SignalPeakDetect_Process(values, count, start_index, end_index, result)`：搜索区间为闭区间。
