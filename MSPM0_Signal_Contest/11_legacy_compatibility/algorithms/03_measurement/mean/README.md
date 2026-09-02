# Mean：平均值 / DC 测量

> 新比赛工程默认：CMSIS RECIPE。直接 `arm_mean_f32(samples, N, &mean)`；不要复制本目录 `.c/.h`。本目录 API 仅为已验证旧 Application 的 `FROZEN_COMPATIBILITY`。完整代码见 `00_docs/CMSIS_DSP_CONTEST_COOKBOOK.md`。

> **LEVEL A / COMPATIBILITY_API：** 新比赛工程请直接使用详细的 [Mean Recipe](../../00_docs/recipes/mean.md)，不再复制本目录 `.c/.h`。本目录和下方 API 仅为已经集成的旧 Application 保留。

## 1 这个算法是干什么的？

把一组数相加后除以点数。对电压数组，它给出这段记录的平均电压，也就是常用的 DC 测量结果。

## 2 一个最简单的例子

```text
输入: 1, 2, 3, 4, 5 V
和:   15 V
点数: 5
平均: 3 V
```

## 3 原理

`mean = (x[0]+...+x[N-1]) / N`。代码使用补偿求和，减少很多浮点数连续相加时的小量被舍入掉的问题。

## 4 比赛里什么时候用？

测直流电压、估计正弦偏置、对重复且稳定的读数降噪。若目标是交流 RMS，不应把 Mean 当 RMS；应使用 AC_RMS 或 RemoveDC 后 RMS。

## 5 输入

`const float *samples`，长度 `count>0`。输入可以是 V 或其他明确单位，但不能含 NaN/Inf。

## 6 输出

`signal_mean_result_t.mean_value`，单位与输入相同。输入是 V，输出就是 V。

## 7 API怎么调用

```c
signal_mean_result_t result;
if (SignalMean_Process(voltage_v, count, &result) == SIGNAL_ALGORITHM_OK) {
    float dc_voltage_v = result.mean_value;
}
```

## 8 参数怎么改

本模块没有滤波参数。主要选择是 `count`：由应用层决定平均多少点。

## 9 参数改大会怎样

`count` 变大通常让稳定 DC 上的随机噪声更小；代价是测量时间更长、跟踪变化更慢。信号在记录期间漂移时，大 count 只会给出一段时间的平均，不是当前值。

## 10 这个算法的代价是什么

Benefits：简单、RAM 常数、对白噪声 DC 测量有效。

Trade-offs：丢失时间细节；不能区分稳定 DC 与缓慢趋势；Cortex‑M0+ 上为软件浮点累加。

## 11 什么时候不要用

- 需要峰值、RMS、瞬态形状时；
- 信号变化快，却要求瞬时读数时；
- RemoveDC 后测原始 DC；此时均值理应接近 0。

## 12 怎么和前一个模块接

```text
ADC_DMA RAW -> ADC_ToVoltage (V) -> Mean
```

## 13 怎么和后一个模块接

Mean 通常就是结果；也可把 `mean_value` 当作 ZeroCross 的中心阈值，但不要把结果结构误当数组继续传。

```text
┌──────── Mean ────────┐
│ float samples[]      │
│          ↓           │
│ compensated sum / N │
│          ↓           │
│ mean_value (same unit)│
└──────────────────────┘
```

## 14 最小Demo

```c
const float x[] = {1, 2, 3, 4, 5};
signal_mean_result_t r;
(void)SignalMean_Process(x, 5U, &r); /* r.mean_value == 3 */
```

## 15 PC测试

`{1,2,3,4,5}` 的 Expected=3，Measured=3，绝对误差 0，PASS。零长度/空 result 也有错误返回检查。

排查：结果偏大/偏小先确认输入是否已经是 V、是否混入无效尾部、`count` 是否真的是有效点数。

## 16 MCU资源

O(N) 时间、O(1) RAM、无动态内存。内部只需少量 `float` 与索引。精确 Flash/周期数等 TI Arm Clang 实板构建后再记录。

## 17 验证状态

PC_VERIFIED：2026-08-07，GCC C11 严格警告编译与真值测试通过；未做实板验证。

## 18. README Usability Upgrade：完整 API

以下内容逐项对应正式头文件；没有公开的范围或语义保留 `UNKNOWN / NOT EXPOSED`。

### `signal_algorithm_status_t SignalMean_Process( const float *samples, uint32_t count, signal_mean_result_t *result);`

- **作用：** 计算一组浮点样本的算术平均值。

| 参数 | 真实类型 | 含义/单位/要求 |
|---|---|---|
| `samples` | `const float *` | 输入样本，只读；单位由调用者决定，输出保持相同单位。 |
| `count` | `uint32_t` | 样本点数，必须大于 0。 |
| `result` | `signal_mean_result_t *` | 输出平均值。 |

- **返回：** 成功返回 SIGNAL_ALGORITHM_OK；空指针、零长度或非有限数返回错误码。
- **前置/后置：** 使用补偿求和降低长数组累计舍入误差，不修改输入数组。

```c
signal_algorithm_status_t status_or_value = SignalMean_Process(samples, count, result);
```

## 19. Call Sequence / Connecting / Buffer Rules

```text
准备输入/config/workspace -> SignalMean_Process -> 检查返回码 -> 读取 result/output -> 交给下一模块
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
