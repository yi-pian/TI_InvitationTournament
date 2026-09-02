# Duty / Pulse Width / Rise Time / Fall Time：脉冲时序

Recipe 状态：`DRAFT`。Duty 子能力已有新的正式 Primitive；Pulse Width 扩展、Rise/Fall Time 与 Slew Rate 的完整链仍有 `IMPLEMENTATION_GAP`。

## 输入与输出

- 输入：`voltage_v[N]`、`Fs`，可选已知低/高平台和阈值比例。
- 输出：周期、频率、高/低脉宽、占空比、平均上升时间、平均下降时间、有效边沿数与离群边沿数。

## 完整逻辑链

```text
电压帧 → 削顶检查
→ 稳健低/高平台估计（平台样本分位数或直方图双峰）
→ 生成阈值：50%用于脉宽/占空比，10/90或20/80用于rise/fall
→ 带滞回状态机寻找上升/下降crossing夹点
→ 对每个crossing做线性插值，得到fractional sample
→ 按 Rising_i → Falling_i → Rising_(i+1) 配对
→ period、high_width、low_width、duty
→ 同一边沿内配对低阈值/高阈值 → rise/fall time
→ 边沿质量检查 → Median/MAD剔除异常 → 分方向平均
```

### 每一步为什么存在

1. **稳健平台**：直接用单个 max/min 会被毛刺或过冲移动全部阈值。
2. **不同阈值用途分离**：50% crossing 适合定义逻辑时刻；10/90 或 20/80 描述边沿速度，不能混为一个阈值。
3. **滞回状态机**：噪声在阈值附近来回摆动时只计一次边沿。
4. **线性插值**：把边沿时间从整数采样点细化为小数位置；必须保证阈值两侧样本确实夹住阈值。
5. **严格方向配对**：防止从帧中间开始、漏边沿或振铃造成脉宽配错。
6. **分方向统计**：上升、下降驱动能力可能不同，不能混在一起平均。

## 标量公式

```c
period_s     = (next_rise_pos - rise_pos) / sample_rate_hz;
high_width_s = (fall_pos - rise_pos) / sample_rate_hz;
low_width_s  = (next_rise_pos - fall_pos) / sample_rate_hz;
duty_ratio   = high_width_s / period_s;
rise_time_s  = (rise_high_pos - rise_low_pos) / sample_rate_hz;
fall_time_s  = (fall_low_pos - fall_high_pos) / sample_rate_hz;
```

这些公式很短，只由本 Recipe 使用，不应各自创建带 Init/Result 的模块。

## 推荐、备选与失效

| 情况 | 默认 | 备选 | 失效条件 |
|---|---|---|---|
| ADC 已采到清楚平台 | 自动平台+多阈值 crossing | 用户传入平台/阈值 | 无平台、严重削顶、阈值越界 |
| 数字电平边沿干净 | Timer Capture 直接量 50% 等效逻辑边沿 | ADC Recipe | 比较器阈值/迟滞未知造成系统偏差 |
| 少量异常边沿 | per-edge 结果做 Median/MAD | Hampel 仅用于非边沿区 | 对原始边沿整体 Median/Hampel 会变慢边沿 |
| 振铃跨阈值 | 要求稳定保持若干点后确认状态 | 提高滞回/设置最小边沿间隔 | 振铃本身是待测对象时不能隐藏 |

## 采样与记录要求

- 占空比/脉宽建议高、低平台各至少 5～10 点；记录至少 3 个完整周期，建议 5～20 个。
- rise/fall time 希望阈值区间内至少约 5 个点；只有 1～2 个间隔时虽能插值，但结果对噪声、带宽和采样相位极敏感。
- `Fs` 不只满足 Nyquist，还要满足边沿带宽。若无法提高 ADC Fs，优先考虑比较器+Timer Capture 或外部高速测量。
- 帧起止处不完整的脉冲/边沿必须丢弃，不进入平均。

## 抗噪声与精度增强

- 平台可用 5%/95% 分位数，但这会忽略真实过冲；若要测过冲，平台估计与峰值测量必须分开。
- 阈值附近 crossing 用线性插值；若边沿明显弯曲，可在局部 3～5 点做直线回归，前提是点数足够且区间近似线性。
- per-edge 时间先用 Median/MAD 删除配对错误，再对有效边沿平均；频率/占空比在帧内变化时同时报告标准差。
- ADC/前端幅值校准影响阈值电压，但相对阈值可消去固定增益；前端群延迟和带宽仍会改变边沿形状。

## MCU / RAM 与 Primitive

- 理想实现 O(N+E)，边沿事件和小数位置 O(E)。
- 可复用 `SignalZeroCross_Process` 与 `SignalZeroCrossInterpolation_Process` 做单阈值事件，但当前 API 不负责多阈值、边沿配对和平台估计。
- 可复用 `SignalRobustPeakToPeak_Process` 估计上下平台、`SignalMAD_Process` 筛 per-edge 结果。
- 历史 `fuxian/24_A` 已证明 20/80 crossing + 分方向多边沿平均是高频需求，但其 Application 私有函数不是正式算法源，不能复制回库冒充 Primitive。

## Primitive 缺口

占空比、周期以及高/低脉宽已有正式 [`duty`](../../03_measurement/duty/README.md) Primitive：它完成 50% crossing、滞回确认、线性插值、R-F-R 配对和多完整周期累计。

多阈值 10/90 或 20/80 crossing、rise/fall time、slew rate 和 per-edge MAD 仍没有统一正式 Primitive，继续标记为 `IMPLEMENTATION_GAP`。因此本 Recipe 整体仍为 `DRAFT`；不能用 `duty` 模块假装 rise/fall time 已实现。

## 20 项执行契约

| # | 字段 | 本 Recipe 的约束 |
|---|---|---|
| 1 | 用途 | 占空比、脉宽、周期、上升/下降时间 |
| 2 | 输入 | `voltage_v[N]`、真实 `Fs`、可选平台/阈值比例 |
| 3 | 输出 | period/high/low/rise/fall time、duty、有效边沿数 |
| 4 | 完整逻辑链 | 见“完整逻辑链” |
| 5 | 步骤原因 | 见六条原因；平台、滞回、插值、方向配对不可省略 |
| 6 | 默认算法 | 稳健平台 + 多阈值 crossing + 线性插值 + 严格配对 |
| 7 | 可选增强 | 局部线性回归、Timer Capture、per-edge MAD |
| 8 | 适用条件 | ADC 清楚采到平台/边沿，或已校准比较器 Capture |
| 9 | 不适用条件 | 无平台、阈值区不足两点、强振铃重复跨阈值、帧边界残缺 |
| 10 | 采样率 | 高/低平台各 5～10 点；边沿阈值区建议至少约 5 点 |
| 11 | 点数/周期数 | 至少 3 个完整周期，建议 5～20 个 |
| 12 | 抗噪 | 滞回、稳定保持、最小边沿间距；只对 per-edge 结果做 MAD |
| 13 | 精度增强 | crossing 线性插值、局部回归、多边沿平均、前端带宽校准 |
| 14 | 计算量 | O(N+E) |
| 15 | RAM | Duty 路径 O(1) 额外 RAM；完整 rise/fall 路径仍需 O(E) 事件/小数位置/结果 |
| 16 | Primitive | Duty、ZeroCross、ZeroCrossInterpolation、RobustPeakToPeak、MAD；多阈值 rise/fall Primitive 仍缺失 |
| 17 | 仓库路径 | `03_measurement/duty`、`03_measurement/frequency_zero_cross`、`05_precision/{zero_cross_interpolation,robust_peak_to_peak}`、`04_dsp/mad` |
| 18 | 伪代码 | `platforms -> thresholds -> crossings -> interpolate -> R-F-R pair -> scalar metrics -> robust average` |
| 19 | MCU 调用 | duty/period/high/low 调用 `SignalDuty_Process`；rise/fall 仍只有 Recipe，不得伪造 API |
| 20 | 失败排查 | 查平台/削顶、阈值定义、方向顺序、event capacity、Fs、残缺边沿和振铃 |

```c
signal_duty_config_t cfg;
signal_duty_result_t measured;

SignalDuty_GetDefaultConfig(&cfg);
if (SignalDuty_Process(voltage_v, sample_count, sample_rate_hz,
        &cfg, &measured) == SIGNAL_ALGORITHM_OK) {
    duty_ratio = measured.duty_ratio;
    high_width_s = measured.high_width_s;
    period_s = measured.period_s;
}

/* rise_time_s/fall_time_s 仍需上文多阈值 Recipe；没有可调用的统一正式 API。 */
```
