# ClippingDetect：检查信号是否撞到上下限

> **LEVEL A / COMPATIBILITY_API：** 新比赛工程请直接使用详细的 [Clipping Detect Recipe](../../00_docs/recipes/clipping_detect.md)。普通上下限计数不再推荐 config/result 模块。

## 1 这个算法是干什么的？

如果输入超过 ADC 或模拟前端范围，波峰会被“削平”。该模块统计有多少点低于低阈值或高于高阈值，给后续测量一个质量警告。

## 2 一个最简单的例子

```text
阈值: low=0.02 V, high=3.28 V
输入: -0.01, 0.00, 1.65, 3.29, 3.31 V
低端2点，高端2点，总4/5，ratio=0.8
```

## 3 原理

逐点判断：`x<=low_limit_v` 是低端削顶候选，`x>=high_limit_v` 是高端候选。只要候选数大于 0，`is_clipped=1`。阈值通常略缩进于实际电源轨，因为模拟输出可能在到达精确 0/3.3 V 前就失真。

## 4 比赛里什么时候用？

Vpp、RMS、FFT/THD 前做数据质量检查；若报警，优先调整量程/增益，而不是继续相信幅值结果。

## 5 输入

`voltage_v[]` 单位 V，`count>0`；`low_limit_v<high_limit_v`，两者也为 V。

## 6 输出

`low_clipped_count/high_clipped_count/clipped_count`（点），`clipped_ratio`（0~1 无量纲），`is_clipped`（0/1）。

## 7 API怎么调用

```c
signal_clipping_detect_config_t cfg = {0.02f, 3.28f};
signal_clipping_detect_result_t result;
SignalClippingDetect_Process(voltage_v, count, &cfg, &result);
```

## 8 参数怎么改

根据 ADC 可用范围和前端输出摆幅改 `low_limit_v/high_limit_v`。例如实际线性范围只有 0.1~3.2 V，就不能仍使用 0.02~3.28 V。

## 9 参数改大会怎样

- 提高 `low_limit_v`：更容易报低端削顶。
- 降低 `high_limit_v`：更容易报高端削顶。
- 阈值向中间缩得太多：误报增加；太靠电源轨：漏报模拟前端提前失真。

## 10 这个算法的代价是什么

Benefits：快、无工作区、能阻止明显坏数据进入精密测量。

Trade-offs：只是阈值证据，不是完整失真分析；噪声尖峰也能触发；无法恢复丢失波形。

## 11 什么时候不要用

- 输入仍是 ADC RAW，却把阈值写成 V；应先 ADC_ToVoltage 或另写清楚 code 阈值。
- 被测信号本来就允许贴近轨；此时 `is_clipped` 需要结合题目解释。
- 想确认轻微非线性失真；应结合 THD/波形观察。

## 12 怎么和前一个模块接

```text
ADC_DMA -> ADC_ToVoltage -> ClippingDetect
```

## 13 怎么和后一个模块接

```text
┌──── ClippingDetect ────┐
│ voltage_v[] + limits   │
│ count samples at rails │
│ ratio + is_clipped     │
└──────────┬─────────────┘
           ├── no  -> Vpp/RMS/FFT
           └── yes -> 调整量程/增益并重采
```

## 14 最小Demo

```c
const float x_v[] = {0.0f, 1.65f, 3.3f};
signal_clipping_detect_config_t cfg = {0.02f, 3.28f};
signal_clipping_detect_result_t r;
(void)SignalClippingDetect_Process(x_v, 3U, &cfg, &r);
/* low=1, high=1, ratio=2/3 */
```

## 15 PC测试

使用 5 点示例真值，Expected low=2、high=2、ratio=0.8；Measured 全部一致，PASS。

排查：总报警先画波形；只有单点报警可能是毛刺；从不报警要确认阈值与电压单位、VREF 和前端线性范围。

## 16 MCU资源

O(N) 比较，O(1) RAM，无动态内存。适合作为每帧质量检查，但不应放进 ADC ISR 做长循环。

## 17 验证状态

PC_VERIFIED：2026-08-07，严格编译和阈值真值测试通过；未 BOARD_VERIFIED。

## 18. README Usability Upgrade：完整 API

以下内容逐项对应正式头文件；没有公开的范围或语义保留 `UNKNOWN / NOT EXPOSED`。

### `signal_algorithm_status_t SignalClippingDetect_Process( const float *voltage_v, uint32_t count, const signal_clipping_detect_config_t *config, signal_clipping_detect_result_t *result);`

- **作用：** 统计达到低/高限幅阈值的电压样本。

| 参数 | 真实类型 | 含义/单位/要求 |
|---|---|---|
| `voltage_v` | `const float *` | 输入电压数组，单位 V，只读。 |
| `count` | `uint32_t` | 样本点数，必须大于 0。 |
| `config` | `const signal_clipping_detect_config_t *` | 低、高限幅判断阈值，单位 V，且 low_limit_v < high_limit_v。 |
| `result` | `signal_clipping_detect_result_t *` | 输出限幅点数、比例和判断标志。 |

- **返回：** 成功返回 SIGNAL_ALGORITHM_OK；参数或数值非法时返回错误码。
- **前置/后置：** 阈值应略缩进于真实 ADC/前端电源轨，避免只有“精确等于满量程”才报警。

```c
signal_algorithm_status_t status_or_value = SignalClippingDetect_Process(voltage_v, count, config, result);
```

## 19. Call Sequence / Connecting / Buffer Rules

```text
准备输入/config/workspace -> SignalClippingDetect_Process -> 检查返回码 -> 读取 result/output -> 交给下一模块
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
