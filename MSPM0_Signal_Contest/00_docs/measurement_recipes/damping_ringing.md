# Damping / Ringing：过冲、振铃与阻尼

## 输入与输出

- 输入：已触发的阶跃响应 `voltage_v[N]`、`Fs`，阶跃前后稳态区间。
- 输出：初值/终值、过冲百分比、峰值时间、振铃频率、衰减包络、log decrement、二阶近似阻尼比，可选 settling time。

## 逻辑链

```text
触发捕获并保留前后数据
→ 阶跃前后稳态区稳健估计 y0/y∞
→ 归一化 e[n]=(y[n]-y∞)/(y∞-y0)
→ 保留过冲，不能Hampel/Median
→ 时域局部峰/谷检测 + prominence + 最小间距
→ 峰时间差平均 → damped ringing frequency
→ 同极性峰包络 Ak=|peak-y∞|
→ 对多个峰计算 δ=(1/m)ln(Ak/Ak+m)
→ 二阶近似 ζ=δ/sqrt((2π)^2+δ^2)
→ 多峰稳健回归/质量检查
```

### 每一步为什么存在

- 稳态估计提供基线和阶跃幅值；用整帧 Mean 会把过渡段混入。
- 峰谷必须相对最终值计算，不能相对 0 V。
- 同极性峰相隔一个振铃周期；相邻峰/谷只隔半周期，不能混算。
- 多峰 log-envelope 回归比只用两个峰抗噪声，但二阶公式只在单一欠阻尼主模态下有物理意义。

## 推荐与备选

| 情况 | 默认 | 备选 | 失效条件 |
|---|---|---|---|
| 清楚单模态振铃 | 局部峰+log decrement | 包络线性回归 | 多模态拍频、强非线性 |
| 峰很密/有噪声 | 先物理合理限带，再找峰 | Hilbert envelope（当前无 Primitive） | 滤波改变真实阻尼/频率 |
| 只有一次过冲 | 报 overshoot 与 settling | 不强算 ζ | 峰数不足仍输出阻尼比 |

## 采样与记录要求

- 振铃每周期建议至少约 20 点；至少得到 3 个同极性峰，推荐 5 个以上。
- 记录必须覆盖阶跃前基线、完整过渡和足够长最终稳态；否则 y∞ 与 settling time 无法定义。
- ADC/前端不能削顶，模拟带宽要高于振铃频率并保留过冲。

## 抗噪声与精度

- 用稳态区 MAD 设置峰 prominence；对 `δ_k` 或回归残差做离群筛选，不删除原始过冲。
- 峰顶三点插值可提高峰时间/幅值；平顶、饱和或边界峰拒绝插值。
- 前端频响和探头负载会改变振铃，需 thru/fixture 校准或至少作为限制记录。

## MCU / RAM 与 Primitive

- O(N+P)，峰列表 O(P)；若做最小二乘 log-envelope 仍是 O(P)、O(1) 小矩阵。
- 可复用 MAD、Statistics；当前频谱 PeakDetect 不能替代时域峰列表。
- 缺失 `time_peak_list` 和 `ringdown_fit`；前者 P1 通用，后者先保持 Recipe，等多题复用后再升级。

Recipe 状态：`DRAFT`。

## 20 项执行契约

| # | 字段 | 本 Recipe 的约束 |
|---|---|---|
| 1 | 用途 | 过冲/欠冲、振铃频率、衰减、阻尼比、settling time |
| 2 | 输入 | 触发阶跃响应 `voltage_v[N]`、Fs、前后稳态区间 |
| 3 | 输出 | y0/y∞、overshoot%、峰时间、ring frequency、δ、ζ、settling |
| 4 | 完整逻辑链 | 见“逻辑链”；稳态、峰列、包络和二阶模型质量依次处理 |
| 5 | 步骤原因 | 见四条原因；峰相对 y∞ 且同极性配对 |
| 6 | 默认算法 | 时域局部峰列 + prominence/distance + log decrement |
| 7 | 可选增强 | 峰顶插值、多峰 log-envelope 回归、物理合理限带 |
| 8 | 适用条件 | 单一欠阻尼主模态、稳态和至少 3 个同极性峰可见 |
| 9 | 不适用条件 | 多模态拍频、削顶、记录无最终稳态、峰不足却强算 ζ |
| 10 | 采样率 | 振铃每周期建议至少约 20 点，前端带宽高于振铃频率 |
| 11 | 点数/周期数 | 至少 3 个同极性峰，推荐 5 个以上并覆盖 settling |
| 12 | 抗噪 | 稳态 MAD 定 prominence；对 δ/回归残差筛离群，不删原始过冲 |
| 13 | 精度增强 | 峰顶三点插值、多峰回归、fixture/探头校准 |
| 14 | 计算量 | O(N+P) |
| 15 | RAM | 帧 + O(P) 峰列表；回归为 O(1) 小矩阵 |
| 16 | Primitive | MAD、Statistics；时域 `time_peak_list`/`ringdown_fit` 尚缺 |
| 17 | 仓库路径 | `04_dsp/mad`、`03_measurement/statistics`；不可误用频谱 PeakDetect |
| 18 | 伪代码 | `baseline -> normalize -> peaks -> same-polarity periods/amplitudes -> δ/ζ -> quality` |
| 19 | MCU 调用 | 下方是 Application 公式片段，不是伪造 API |
| 20 | 失败排查 | 查稳态区、削顶、峰方向/间距、峰数、二阶假设、探头负载和前端带宽 |

```c
/* APPLICATION RECIPE：peak_amp[] 已由时域局部峰逻辑得到。 */
float delta = logf(peak_amp[k] / peak_amp[k + 1U]);
float zeta = delta / sqrtf(4.0f * PI_F * PI_F + delta * delta);
```
