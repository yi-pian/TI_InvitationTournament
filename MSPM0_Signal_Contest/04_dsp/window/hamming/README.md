# Hamming Window

> **INTERNAL CHILD / COMPATIBILITY ALIAS：** 正常应用只调用父入口 [`../signal_window.h`](../signal_window.h) 的 `SignalWindow_Apply(..., SIGNAL_WINDOW_HAMMING, ...)`。本目录不再作为独立模块选择。

## 1 这个算法是干什么的？

用两端约 0.08 的余弦权重控制泄漏。

## 2 一个最简单的例子

全 1 输入的首点输出约 0.08。

## 3 原理

`w=0.54-0.46cos(2πn/(N-1))`。它在某些旁瓣指标上优于 Hann，但不是所有距离都更低。

## 4 比赛里什么时候用？

一般频谱并希望特定第一旁瓣更低时，与 Hann 实测比较。

## 5 输入

float 数组、N>=2。

## 6 输出

加窗数组与实际增益。

## 7 API怎么调用

`SignalHamming_Apply(in,out,N,&r);`

## 8 参数怎么改

无参数；窗型本身就是选择。

## 9 参数改大会怎样

N 大时 CG 趋近 0.54，但应使用实际返回值。

## 10 这个算法的代价是什么

Benefits：降低矩形泄漏。Trade-offs：主瓣宽、幅值需校正。

## 11 什么时候不要用

相干且窄主瓣优先、或时域瞬态。

## 12 怎么和前一个模块接

`RemoveDC -> Hamming`

## 13 怎么和后一个模块接

`Hamming -> FFT -> GainCorrection`

## 14 最小Demo

```c
signal_window_result_t r;
(void)SignalHamming_Apply(x,x,N,&r);
```

## 15 PC测试

8 点端点 0.08，CG 解析值一致，PASS。

## 16 MCU资源

O(N)、O(1)，每点一次 cosf。

## 17 验证状态

PC_VERIFIED；未实板。

## 18. README Usability Upgrade：完整 API

以下内容逐项对应正式头文件；没有公开的范围或语义保留 `UNKNOWN / NOT EXPOSED`。

### `signal_algorithm_status_t SignalHamming_Apply( const float *input_samples, float *output_samples, uint32_t count, signal_window_result_t *result);`

- **作用：** 应用对称 Hamming 窗，支持原地，并返回实际窗增益。

| 参数 | 真实类型 | 含义/单位/要求 |
|---|---|---|
| `input_samples` | `const float *` | UNKNOWN / NOT EXPOSED |
| `output_samples` | `float *` | UNKNOWN / NOT EXPOSED |
| `count` | `uint32_t` | UNKNOWN / NOT EXPOSED |
| `result` | `signal_window_result_t *` | UNKNOWN / NOT EXPOSED |

- **返回：** UNKNOWN / NOT EXPOSED
- **前置/后置：** UNKNOWN / NOT EXPOSED

```c
signal_algorithm_status_t status_or_value = SignalHamming_Apply(input_samples, output_samples, count, result);
```

## 19. Call Sequence / Connecting / Buffer Rules

```text
准备输入/config/workspace -> SignalHamming_Apply -> 检查返回码 -> 读取 result/output -> 交给下一模块
```

算法无 SysConfig、无动态内存。输入/输出、是否允许原地、workspace 和 capacity 以第18节真实 `@param/@note` 为准；capacity 是元素数。失败时不得把 result/output 当有效数据。模块上下游和更完整示例见原 README 第12～14节。

## 20. Parameter Guide / Config vs SysConfig

| 可修改项 | 真实入口 | 改变后的主要影响 | RAM/速度 | SysConfig? |
|---|---|---|---|---|
| `count` | 公开 API 参数 | 增大通常增加数据量、邻域或阶数，并增加 RAM/CPU；减小则相反，必须满足下限 | 由 N/容量/复杂度决定 | 否 |

改变 ADC/DAC pin、Timer/DMA/Event 或真实 Fs 属于上游硬件配置，不是本算法 SysConfig。常见错误：单位混用、capacity 不足、忽略返回码、原地规则错误、Fs/N 与数据不一致、把 raw/能量/幅值/百分数混为一谈。

## 21. Verification / Quick Modify

先用原 README 已列 PC 真值/边界方法；再在完整链中用已知输入核对单位和误差。未实板不得写 BOARD_VERIFIED。

| 我想改什么 | 去哪里 | 改什么 | 会影响什么 | SysConfig? |
|---|---|---|---|---|
| N/容量 | Application buffer + API | count/capacity | RAM、CPU、观察长度 | 否 |
| Fs/频率 | Application config/API | actual Fs/target Hz | 频率刻度 | 改真实硬件Fs时才是 |
| 阈值/阶数/半径 | config/API | 对应真实字段 | 灵敏度、带宽或计算量 | 否 |
| 输入单位/标度 | 上游 Adapter/校准 | V、magnitude、energy 等 | 结果物理意义 | 否 |
