# Statistics：一次得到均值、范围和噪声离散程度

> **LEVEL C / REAL ALGORITHM MODULE：** 它一次输出 mean/min/max/RMS/standard deviation，并统一补偿求和和数值检查；多输出和数值语义值得保留为模块。

> 新比赛工程默认：CMSIS DIRECT。普通 Mean/Variance/Std 使用 `arm_mean_f32`、`arm_var_f32`、`arm_std_f32`；不要复制本目录核心。本目录 Welford/组合结果 API 仅为旧 Application 和需要明确比较其单遍数值行为的场景保留。

**新比赛工程不复制本目录源码。** 无 SysConfig/Pin；调用代码见 `00_docs/CMSIS_DSP_CONTEST_COOKBOOK.md`。只有证明当前 Welford 组合接口在精度、遍历次数或资源上更适合目标后，才把它作为 `SPECIAL_BACKEND` 评估。

## 1 这个算法是干什么的？

它给一段数据做“体检”：平均多少、最小/最大多少、围绕平均值散得多开。标准差常用于粗略描述噪声大小。

## 2 一个最简单的例子

```text
输入: 1,2,3,4,5
mean = 3
min/max = 1/5
总体方差 = 2
样本方差 = 2.5
总体标准差 = sqrt(2)
```

## 3 原理

方差描述 `(x-mean)^2` 的平均。代码使用 Welford 在线更新均值和平方偏差，避免先计算两个很接近的大数 `mean(x²)-mean(x)²` 再相减造成精度损失。

总体方差除以 N；样本方差除以 N-1。只有一个点时，样本方差无定义，所以 `sample_variance_valid=0`。

## 4 比赛里什么时候用？

评估 ADC 静态噪声、比较滤波前后标准差、检查数据范围、为阈值选择提供依据。

## 5 输入

`const float *samples`，`count>0`。单位必须一致；不能包含 NaN/Inf。

## 6 输出

- `mean_value/min_value/max_value/population_stddev/sample_stddev`：与输入相同单位。
- `population_variance/sample_variance`：输入单位的平方，例如 V²。
- `sample_variance_valid`：count>=2 时为 1。

## 7 API怎么调用

```c
signal_statistics_result_t r;
if (SignalStatistics_Process(voltage_v, count, &r)
        == SIGNAL_ALGORITHM_OK) {
    float noise_std_v = r.population_stddev;
}
```

## 8 参数怎么改

没有内部参数。应用层选择数据段和 `count`；做静态噪声测试时输入应尽量是稳定 DC，而不是把正弦波的标准差叫“噪声”。

## 9 参数改大会怎样

`count` 大通常使稳定分布的统计量更可靠，但测量时间增加。若信号状态在这段时间改变，最终统计量会混合多个状态。

## 10 这个算法的代价是什么

Benefits：常数 RAM；一次扫描；数值稳定；同时返回常用摘要。

Trade-offs：时间顺序完全丢失；异常点影响均值和方差；标准差不是频谱噪声密度。

## 11 什么时候不要用

- 需要频率、相位、瞬态发生时刻；
- 把周期波形本身的变化误当噪声；
- 有严重离群点且要鲁棒统计，应先判断异常是否真实，再用 MAD/Hampel/robust 方法。

## 12 怎么和前一个模块接

```text
ADC_ToVoltage -> Statistics
```

## 13 怎么和后一个模块接

```text
┌──────── Statistics ────────┐
│ samples[]                  │
│ Welford mean + M2          │
│ mean/min/max/std/variance  │
└──────────────┬─────────────┘
               ↓
       噪声报告 / 阈值选择
```

result 不是样本数组，不能直接交给 FFT。

## 14 最小Demo

```c
const float x[] = {1,2,3,4,5};
signal_statistics_result_t r;
(void)SignalStatistics_Process(x, 5U, &r);
/* mean=3, population_variance=2 */
```

## 15 PC测试

用 `{1,2,3,4,5}` 检查均值 3、总体方差 2、样本方差 2.5、总体标准差 `sqrt(2)`；全部 PASS。

排查：标准差异常大先检查信号是否本来就在变化、数组尾部是否有效、是否有毛刺；与软件结果不一致时确认使用的是总体还是样本方差。

## 16 MCU资源

O(N) 时间、O(1) RAM、最后两次 `sqrtf`。无动态内存。若只需 Mean，不要为了方便调用 Statistics，多余平方/开方会浪费 Cortex‑M0+ 周期。

## 17 验证状态

PC_VERIFIED：2026-08-07，严格编译及已知方差测试通过；未 BOARD_VERIFIED。

## 18. README Usability Upgrade：完整 API

以下内容逐项对应正式头文件；没有公开的范围或语义保留 `UNKNOWN / NOT EXPOSED`。

### `signal_algorithm_status_t SignalStatistics_Process( const float *samples, uint32_t count, signal_statistics_result_t *result);`

- **作用：** 一次扫描计算数量、均值、极值、总体/样本方差和标准差。

| 参数 | 真实类型 | 含义/单位/要求 |
|---|---|---|
| `samples` | `const float *` | 输入浮点样本，只读；均值/极值/标准差保持输入单位，方差为输入单位的平方。 |
| `count` | `uint32_t` | 样本点数，必须大于 0。 |
| `result` | `signal_statistics_result_t *` | 输出统计量；count<2 时样本方差字段为 0 且 valid=0。 |

- **返回：** 成功返回 SIGNAL_ALGORITHM_OK；参数或数值非法时返回错误码。
- **前置/后置：** 使用 Welford 更新，适合避免“大直流 + 小波动”时的严重相消误差。

```c
signal_algorithm_status_t status_or_value = SignalStatistics_Process(samples, count, result);
```

## 19. Call Sequence / Connecting / Buffer Rules

```text
准备输入/config/workspace -> SignalStatistics_Process -> 检查返回码 -> 读取 result/output -> 交给下一模块
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
