# Auto Range：自动量程

自动量程是“测量控制 Recipe”，不是把波形归一化到 0～1。它必须改变真实硬件量程后重新采样，并保持结果可追溯到当前量程校准参数。

## 输入与输出

- 输入：一帧 raw/voltage、当前量程编号、每档安全范围/噪声底/校准参数、允许的量程列表。
- 输出：接受当前帧，或请求升/降量程；最终量程、测量值、切换次数和有效状态。

## 控制链

```text
当前档采一帧 → raw与voltage双重检查
→ clipping/靠近电源轨？        → 降低前端增益或扩大输入量程
→ 有效Vpp/RMS只占很少码？      → 提高增益或缩小量程
→ 位于目标利用率窗口？          → 接受
→ 切档 → 等待建立 → 丢弃1~数帧 → 用该档校准参数重新测量
→ 滞回+最大切换次数防止来回振荡
```

### 为什么需要这些门

- 只检查“精确等于 0/4095”会漏掉模拟前端先饱和，阈值应缩进电源轨。
- 小信号档位判断应同时看噪声底，否则会把噪声放大成“满量程利用”。
- 每一档的增益、偏置和频响不同，切档后不能沿用上一档校准。
- 切换瞬间含开关毛刺和滤波器建立过程，必须重新采样。

## 默认策略与失效

- 默认目标可设为峰值占 ADC 满量程约 20%～80%，具体余量由过冲/波形决定。
- 升档阈值和降档阈值必须分开，例如低于 15% 才升增益、高于 85% 才降增益。
- burst、幅值快速变化或每档响应时间很长时，离散逐帧自动量程可能追不上；需要固定安全档或双量程并行硬件。
- 如果所有档都削顶/低于噪声，返回 `OVER_RANGE/UNDER_RANGE`，不能继续输出伪精确结果。

## 适用条件与备选

- 适用：输入幅值相对量程变化较慢，前端有离散可校准档位，并允许切档后重采样。
- 默认：逐帧滞回判定后选择一档并冻结。
- 备选：幅值变化很快时固定安全大档；动态范围仍不够且硬件允许时使用两路并行量程后离线融合。当前正式算法库没有 DualRangeFusion，不能假装已实现。
- 不适用：单次不可重复 burst 在切档前已经结束，或量程切换会改变待测对象。

## 采样、抗噪声与精度

- 每档至少一帧用于判定，接受前建议再测 2～3 帧确认；切换后等待时间由模拟电路而不是算法猜测。
- 判定量使用 Robust Vpp/AC RMS 和 Clipping Detect；尖峰本身是目标时改用保守峰值余量。
- 量程边界用多帧 Median/MAD，避免噪声导致抖档。

## MCU / RAM 与 Primitive

- 每帧 O(N)，控制状态 O(1)；无需大型新 workspace。
- 复用 Clipping Detect Direct Recipe、Vpp/AC RMS、Robust VPP/RMS、ADC Calibration。
- 硬件切档 callback、settling delay 和量程表属于 Application/Frontend，不属于纯算法模块。

## 是否升级 Primitive

当前策略高度依赖具体 PGA/VGA/模拟开关和题目速度，先保持 Recipe/Application 状态机。只有至少两个硬件平台共享同一“range decision”纯逻辑并有可注入回调后，才考虑正式 Helper。Recipe 状态：`DRAFT`。

## 20 项执行契约

| # | 字段 | 本 Recipe 的约束 |
|---|---|---|
| 1 | 用途 | 在避免削顶的前提下提高 ADC 有效码利用率 |
| 2 | 输入 | 帧、当前档、各档安全窗口/噪声/校准/建立时间 |
| 3 | 输出 | accept/up/down、最终档、测量值、切换次数、状态 |
| 4 | 完整逻辑链 | 见“控制链”，每次切档后必须建立、丢帧、重新测量 |
| 5 | 步骤原因 | 见四个门；模拟饱和、噪声、档位校准和切换毛刺分开处理 |
| 6 | 默认算法 | 双阈值滞回的逐帧离散档位状态机 |
| 7 | 可选增强 | 多帧确认、双量程并行、最安全档固定策略 |
| 8 | 适用条件 | 输入变化慢于“采集+切档+建立+确认”循环 |
| 9 | 不适用条件 | burst/快速变化、未知 settling、所有档均过量程/欠量程 |
| 10 | 采样率 | 由被测信号 Recipe 决定；切档状态机不得改变真实 Fs 语义 |
| 11 | 点数/周期数 | 每档至少 1 判定帧，接受前建议 2～3 帧确认 |
| 12 | 抗噪 | Robust Vpp/AC RMS、边界多帧 Median/MAD、滞回 |
| 13 | 精度增强 | 每档独立校准/频响，切档建立时间实测，记录 range id |
| 14 | 计算量 | 每帧 O(N)，控制 O(1) |
| 15 | RAM | 一帧 + O(1) 状态；多档参数 O(R) |
| 16 | Primitive | Clipping、Vpp/AC RMS、Robust VPP/RMS、ADC Calibration |
| 17 | 仓库路径 | 本 Recipe + 对应 Primitive；切档 callback 属于目标 Application/Frontend |
| 18 | 伪代码 | `measure -> over/under/in-window -> hysteretic decision -> set -> settle/discard -> retry` |
| 19 | MCU 调用 | 下方为状态机伪代码，`frontend_set_range` 必须由目标硬件真实 API 替换 |
| 20 | 失败排查 | 查当前档/校准表、轨边阈值、噪声底、切换方向、settling、抖档和最大迭代 |

```c
/* APPLICATION PSEUDOCODE：不得照抄 frontend_set_range 这个占位名。 */
if (frame_clipped) decision = RANGE_LOWER_GAIN;
else if (utilization < low_threshold) decision = RANGE_HIGHER_GAIN;
else if (utilization <= high_threshold) decision = RANGE_ACCEPT;
```
