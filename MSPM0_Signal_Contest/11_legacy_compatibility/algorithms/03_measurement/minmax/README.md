# MinMax：找最小值和最大值

> 新比赛工程默认：CMSIS DIRECT。使用 `arm_min_f32` 和 `arm_max_f32`；不要复制本目录核心。旧 API 仅为 `FROZEN_COMPATIBILITY`。

> **LEVEL A / COMPATIBILITY_API：** 新比赛工程请直接使用详细的 [Min/Max Recipe](../../00_docs/recipes/minmax.md)。旧 `.c/.h` 暂不删除，避免破坏现有 Application。

## 1 这个算法是干什么的？

从数组中找到最低点、最高点，并告诉你它们在第几个样本。它是最简单的幅度范围检查工具。

## 2 一个最简单的例子

```text
输入:       2, -1, 5, 3, 5
minimum:   -1，index=1
maximum:    5，index=2（第一次出现）
```

## 3 原理

从第一个样本开始保存当前 min/max，随后逐点比较并更新。只扫描一次，复杂度 O(N)。相等时不更新，所以索引是第一次出现位置。

## 4 比赛里什么时候用？

快速检查电压范围、确认信号是否进入预期窗口、定位最高/最低样本。干净波形的 Vpp 可以由 `max-min` 得到。

## 5 输入

`const float *samples`、`count>0`。单位可为 V 或零偏 V，但全数组必须一致且不能有 NaN/Inf。

## 6 输出

`min_value/max_value` 与输入同单位，`min_index/max_index` 是从 0 开始的无单位索引。

## 7 API怎么调用

```c
signal_minmax_result_t result;
signal_algorithm_status_t status =
    SignalMinMax_Process(voltage_v, count, &result);
```

## 8 参数怎么改

没有阈值参数。应用层只需决定输入记录和 `count`。若只分析帧的一部分，传入起始指针和该段点数，同时记住返回索引相对于这段起点。

## 9 参数改大会怎样

增大 `count` 增加捕获真实峰谷的机会，也增加遇到偶发毛刺的机会，并延长测量时间。

## 10 这个算法的代价是什么

Benefits：一次扫描、常数 RAM、结果直观。

Trade-offs：极端值对异常点最敏感；离散采样可能错过连续波形真正峰值；不提供抗噪统计保证。

## 11 什么时候不要用

- ADC 有偶发跳码/毛刺且要稳定幅值；
- 尖峰是否有效尚未判断；
- 记录没有覆盖完整周期，却要宣称全局 Vpp。

此时考虑 Hampel+RobustVPP 或增加合适记录长度，但不能盲目删除真实瞬态。

## 12 怎么和前一个模块接

```text
ADC_ToVoltage -> float voltage_v[] -> MinMax
```

## 13 怎么和后一个模块接

```text
┌──────── MinMax ────────┐
│ samples[] + count      │
│ min / max comparison   │
│ min,max,indexes        │
└───────────┬────────────┘
            ├──> 显示范围
            └──> Vpp = max-min
```

Vpp 模块已经独立实现，通常直接调用 `SignalVPP_Process()`，无需先创建 MinMax result。

## 14 最小Demo

```c
const float x[] = {1, 2, 3, 4, 5};
signal_minmax_result_t r;
(void)SignalMinMax_Process(x, 5U, &r);
/* min=1,index=0; max=5,index=4 */
```

## 15 PC测试

已用 `{1,2,3,4,5}` 验证 min/max 和索引，Measured 与 Expected 全部一致。

排查：索引看似错时确认它是从 0 开始且相对于传入指针；异常大/小时绘制原数组检查毛刺和未填充尾部。

## 16 MCU资源

时间 O(N)、内部 RAM O(1)、无动态内存。每点最多两次比较。

## 17 验证状态

PC_VERIFIED：2026-08-07，GCC C11 严格警告编译及真值测试通过；未做 BOARD_VERIFIED。

## 18. README Usability Upgrade：完整 API

以下内容逐项对应正式头文件；没有公开的范围或语义保留 `UNKNOWN / NOT EXPOSED`。

### `signal_algorithm_status_t SignalMinMax_Process( const float *samples, uint32_t count, signal_minmax_result_t *result);`

- **作用：** 查找浮点样本中的最小值、最大值及首次出现的位置。

| 参数 | 真实类型 | 含义/单位/要求 |
|---|---|---|
| `samples` | `const float *` | 输入样本，只读；结果与输入使用相同单位。 |
| `count` | `uint32_t` | 样本点数，必须大于 0。 |
| `result` | `signal_minmax_result_t *` | 输出最小值、最大值和索引。 |

- **返回：** 成功返回 SIGNAL_ALGORITHM_OK；参数或数值非法时返回错误码。
- **前置/后置：** 对毛刺非常敏感；有异常点时应改用 RobustPeakToPeak 等鲁棒算法。

```c
signal_algorithm_status_t status_or_value = SignalMinMax_Process(samples, count, result);
```

## 19. Call Sequence / Connecting / Buffer Rules

```text
准备输入/config/workspace -> SignalMinMax_Process -> 检查返回码 -> 读取 result/output -> 交给下一模块
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
