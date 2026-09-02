# Vpp：峰峰值与稳健峰峰值

## 输入与输出

- 输入：校准后的 `voltage_v[N]`。
- 输出：普通 `vpp_v=max-min`，或明确标注为分位数定义的 `robust_vpp_v`。

## 默认逻辑链

```text
raw → ADC转电压 → 增益/偏置校准 → 削顶检查
→ 确认记录覆盖峰和谷 → Min/Max → Vpp=max-min
```

每一步分别防止“码值被当伏特”“比例错误”“满量程假峰”和“记录太短没有采到峰谷”。普通 Vpp 是物理记录中的最大跨度，不应先 RemoveDC；减去常量虽然理论上不改变 Vpp，却会多一次数组处理且可能掩盖偏置诊断。

## 强干扰/毛刺分支

```text
确认孤立尖峰不是被测对象
→ 方案A：上下分位数 → robust_vpp
→ 方案B：Hampel替换孤立毛刺 → 普通Vpp，并记录 replaced_count
→ 多帧Vpp → Median/MAD剔除异常帧 → 对有效帧平均
```

- 默认仍是普通 Vpp。
- 少量 ADC 错码时优先 `SignalRobustPeakToPeak_Process`，例如 5%/95% 分位数；输出必须叫 robust Vpp，因为它主动忽略真实尾部。
- 需要保留真实过冲、尖峰或窄脉冲时，禁止 Robust/Hampel/Median，应提高模拟带宽、Fs 和记录长度。

## 采样要求与失效条件

- 周期波形至少覆盖 1 个完整周期，建议 3～10 周期；起止位置随机时，多周期更可能覆盖峰谷。
- 正弦峰值误差取决于每周期点数和相位；建议约 20 点/周期，或改用 SineFit/FFT 幅值。
- 方波平台 Vpp 还应保证高低平台各有足够点。削顶、前端钳位、采样保持未建立会直接使结果无效。
- 慢漂移会把 Vpp 混入基线变化；若题目要短期交流跨度，应先分段或去趋势，但当前没有正式 Detrend Primitive。

## 抗噪声与精度增强

- 量化/白噪声会系统性抬高 max、压低 min；增加 N 反而可能让极值更极端，这时用正弦拟合、AC RMS 换算或分位数，而不是盲目加点。
- 前端/ADC gain-offset 校准后再报伏特值。
- 自动量程应让有效波形占 ADC 满量程的约 20%～80%，同时保留过冲余量。

## MCU / RAM 与 Primitive

- 普通 Vpp：O(N)、O(1)，直接使用 [Vpp Direct Recipe](../recipes/vpp.md)，不必复制兼容 `.c/.h`。
- Robust Vpp：平均 O(N)，需要 `float workspace[N]`，复用 `SignalRobustPeakToPeak_Process`。
- 可选 `SignalHampel_Process`、`SignalMAD_Process` 和 Clipping Detect Direct Recipe。

Recipe 状态：`DRAFT`；现有 Robust VPP Primitive 为 `PC_VERIFIED`，不代表模拟前端或板级 Vpp 已验证。

## 20 项执行契约

| # | 字段 | 本 Recipe 的约束 |
|---|---|---|
| 1 | 用途 | 普通 Vpp、强噪声/孤立错误码下的 robust Vpp |
| 2 | 输入 | 校准 `voltage_v[N]` |
| 3 | 输出 | `vpp_v` 或明确命名的 `robust_vpp_v`，单位 V |
| 4 | 完整逻辑链 | 见默认链和强干扰分支 |
| 5 | 步骤原因 | 换算、校准、削顶和完整周期分别排除四类假结果 |
| 6 | 默认算法 | 普通 MinMax/Vpp |
| 7 | 可选增强 | 分位数 Robust VPP、Hampel 后 Vpp、多帧 MAD |
| 8 | 适用条件 | 记录覆盖真实峰谷且量程未切换 |
| 9 | 不适用条件 | 削顶；要测真实尖峰却使用 Robust/Hampel；不足完整周期 |
| 10 | 采样率 | 正弦峰值建议约 20 点/周期；窄峰按峰宽至少 5 点 |
| 11 | 点数/周期数 | 至少 1 个完整周期，建议 3～10 周期 |
| 12 | 抗噪 | 多帧 Vpp 做 Median/MAD；孤立坏点才使用分位数 |
| 13 | 精度增强 | 增加每周期点数、正弦改用拟合/AC RMS、ADC/前端校准 |
| 14 | 计算量 | 普通 O(N)，Robust 平均 O(N) |
| 15 | RAM | 普通 O(1)，Robust 需 `float workspace[N]` |
| 16 | Primitive | Vpp Direct Recipe、RobustPeakToPeak、Hampel、MAD、Clipping |
| 17 | 仓库路径 | `00_docs/recipes/vpp.md`、`05_precision/robust_peak_to_peak`、`04_dsp/{hampel_filter,mad}` |
| 18 | 伪代码 | `validate -> ordinary max-min OR confirmed-non-target-tail quantiles -> report definition` |
| 19 | MCU 调用 | 见下方真实 Robust API；普通 Vpp 调用 Direct Copy 的 `recipe_vpp` |
| 20 | 失败排查 | 查削顶、量程、完整周期、尖峰是否目标、workspace 容量与分位数配置 |

```c
signal_robust_peak_to_peak_config_t cfg = {0.05f, 0.95f};
signal_robust_peak_to_peak_result_t result;
signal_algorithm_status_t status = SignalRobustPeakToPeak_Process(
    voltage_v, N, &cfg, workspace, N, &result);
```
