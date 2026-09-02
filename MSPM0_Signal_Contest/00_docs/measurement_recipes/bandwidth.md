# Cutoff Frequency / -3 dB Bandwidth / Unity-Gain Bandwidth

## 先定义题目中的“带宽”

- **截止频率**：相对参考增益下降到指定阈值的频率，常见为 -3 dB。
- **闭环 -3 dB 带宽**：闭环响应相对低频平台下降 3 dB。
- **0 dB / unity crossover**：增益曲线穿过 1 V/V（0 dB）的频率。
- **电赛口语“单位增益带宽”**：有时指电压跟随器闭环 -3 dB 带宽；有时指运放开环增益交越。两者测法不同，必须按题面定义。没有环路增益注入，不能凭闭环一条曲线声称测得真实开环 UGBW。

## 输入与输出

- 输入：Frequency Response Recipe 得到的按频率排序 `f[k]` 与 `gain_db[k]`/线性增益，及参考频段定义。
- 输出：`cutoff_frequency_hz`、`bandwidth_hz`、可选 `unity_crossover_hz`、用于插值的包围点和结果有效性。

## 逻辑链

```text
先完成频响扫频与thru校准
→ 在指定低频参考区求稳健参考增益 Gref（不是随便取第一个点）
→ target_db=Gref_db-3.0103 dB
→ 找到首次满足“前点高于、后点低于target”的相邻包围点
→ 在 log10(f)-gain_dB 平面线性插值
→ 得到 -3 dB cutoff
→ 若题目要0 dB交越：同法找 gain_db=0 的包围点
→ 峰化/多次穿越时报告全部穿越或按题目选定区间
```

### 为什么这些步骤存在

- 参考增益应来自平坦区的多点 Median/Mean，避免单个噪声点改变 -3 dB 门限。
- 必须先有包围点；没有跨过阈值不能外推一个“带宽”。
- Bode 曲线通常对 log-frequency 更接近局部直线，优先在 `log10(f)` 上插值；粗扫频只在 Hz 上线性插值会偏。
- 峰化、多极点或带通响应可能多次穿越，简单“第一个低于”不总是题目需要的截止点。

## 推荐、备选与失效

| 情况 | 默认 | 备选 | 失效条件 |
|---|---|---|---|
| 单调低通 | 对数频率+dB插值 | 局部二次拟合 | 扫频没有包围 -3 dB |
| 有噪声/点抖动 | 参考区稳健平均+局部重复测量 | 单调回归（PC后处理） | AGC/量程切换未校准造成台阶 |
| 共振/峰化 | 明确参考和第几个穿越 | 输出上下截止/多个穿越 | 用“第一个低于”硬套所有曲线 |
| 开环 unity crossover | 真实环路增益测量 | 题目允许的等效方法 | 仅闭环 follower 曲线却声称开环 UGBW |

## 采样与精度

- 每个频点遵守 Frequency Response Recipe；截止附近应缩小步长，至少有一高一低两个可靠点，建议自适应二分/对数中点补测 2～4 次。
- 多帧 gain_db 先做 Median/MAD；插值只降低扫频网格量化，不会修正前端频响、源幅值和 Fs 误差。
- 历史 `fuxian/24_A` 使用带 DC 正弦的 AC RMS 比值寻找 0.707 幅值门限，并在相邻频点插值；该逻辑说明“Mean 不能代替交流幅值”。

## MCU / RAM 与现有 Primitive

- 曲线搜索 O(K)、O(1)；保存 K 点约每个点 12～24 字节，亦可流式处理。
- 复用 Frequency Response、Gain、AC RMS/LockIn/SineFit3、Median/MAD。
- `target`、包围点和 log-frequency 线性插值是短 Recipe 逻辑，暂不单独建模块。

## 缺口

若多个 Application 都需要“带质量标志的阈值包围 + 线性/log 插值 + 多穿越”，可升级为 P1 `curve_crossing` Simple Helper，并配 Python golden model。当前 Recipe 状态：`DRAFT`。

## 20 项执行契约

| # | 字段 | 本 Recipe 的约束 |
|---|---|---|
| 1 | 用途 | -3 dB 截止、带宽、0 dB crossover、题面定义允许时 UGBW |
| 2 | 输入 | 有序 `f[k]`、`gain_db[k]`、参考频段和带宽定义 |
| 3 | 输出 | cutoff/bandwidth/crossover Hz、包围点、有效状态 |
| 4 | 完整逻辑链 | 频响/校准 -> 参考平台 -> 阈值 -> 包围点 -> log-f 插值 |
| 5 | 步骤原因 | 见“为什么这些步骤存在” |
| 6 | 默认算法 | 稳健参考 + 首次指定方向穿越 + log-frequency 线性插值 |
| 7 | 可选增强 | 截止附近二分补测、局部二次/单调回归、多穿越输出 |
| 8 | 适用条件 | 参考定义明确且扫频覆盖阈值两侧 |
| 9 | 不适用条件 | 无包围点、把闭环带宽冒充开环 UGBW、峰化却未定义穿越 |
| 10 | 采样率 | 由每个频点的 Frequency Response Recipe 决定 |
| 11 | 点数/周期数 | 每频点 5～20 周期；阈值附近至少一高一低可靠点并建议补测 |
| 12 | 抗噪 | 参考区/频点多帧 Median/MAD，坏点重测 |
| 13 | 精度增强 | 自适应缩步、log-f 插值、thru/源幅值/采样时钟校准 |
| 14 | 计算量 | O(K) |
| 15 | RAM | 流式 O(1) 或每点约 12～24 字节保存曲线 |
| 16 | Primitive | Gain、AC RMS/LockIn/SineFit3、Median/MAD；`curve_crossing` 尚缺 |
| 17 | 仓库路径 | 本 Recipe、`frequency_response.md`、`05_precision/{lock_in,sine_fit_3param}` |
| 18 | 伪代码 | `Gref -> target -> find bracket -> interpolate log10(f) -> validate crossing` |
| 19 | MCU 调用 | 下方为应用伪代码，无伪造库 API |
| 20 | 失败排查 | 查参考平台、dB/ratio 阈值、排序、包围方向、多穿越、量程台阶和校准 |

```c
/* APPLICATION PSEUDOCODE：curve_crossing 尚不是正式 API。 */
if (gain_db[k - 1U] >= target_db && gain_db[k] <= target_db) {
    float a = (target_db - gain_db[k - 1U]) / (gain_db[k] - gain_db[k - 1U]);
    cutoff_hz = powf(10.0f, log10f(f[k - 1U]) + a *
                     (log10f(f[k]) - log10f(f[k - 1U])));
}
```
