# Multi-channel Delay Compensation：多通道延时补偿

## 输入与输出

- 输入：所有通道同时接到同一校准信号时的波形或多频相位，参考通道编号、`Fs`。
- 输出：每通道相对参考的固定 `delay_s`/`delay_samples`、残差与有效频率范围；正式测量中的补偿相位。

## 校准链 A：宽带/脉冲相关

```text
同一宽带信号送所有通道 → 同步采样 → 每路去DC/同带宽处理
→ reference与channel_i互相关 → 峰lag
→ 相关峰亚样本插值 → delay_i=lag_i/Fs
→ 多帧Median/MAD → 保存delay与相关系数
```

## 校准链 B：多频相位斜率

```text
同一正弦逐频点送所有通道
→ 每点测 phase_i-reference
→ 沿频率unwrap → 线性拟合 phase_rad(f)=phase0-2πf*delay
→ delay=-slope/(2π) → 检查残差
```

单频 `SignalChannelDelayCalibration_Compute` 简单快速，但相位 wrap 使延迟只在半周期先验内无歧义；多频斜率更稳健，也能发现通道前端存在非线性相位。

## 推荐、备选与适用条件

- 默认：校准信号为宽带脉冲/噪声且相关峰唯一时，使用相关链。
- 备选：只能逐频产生正弦时，使用多频 phase unwrap + 斜率；已知延迟小于半周期时可用现有单频 Calibration 快速完成。
- 适用：采样顺序固定、各通道连接与前端配置和正式测量一致。
- 不适用：周期正弦相关出现多个等价峰且没有 lag 先验，或前端群延迟随频率强烈变化却强行用一个固定 delay 表示。

## 正式测量如何应用

- 只补相位结果：`corrected_phase = measured_phase + 360*f*delay`，再 wrap。
- 需要对齐波形后再做相关/相减：整数 delay 可移动索引；小数 delay 需要经离线设计的 fractional-delay FIR。不能只改相位标量却声称时域数组已对齐。

## 采样、抗噪声与失效

- 相关法要求校准波形具有足够带宽和唯一相关峰；纯周期正弦会有多周期等价峰。
- 相位斜率至少 3 个频点，建议覆盖宽频段并在每点测 5～20 周期；phase unwrap 必须结合最大可能 delay。
- 每通道使用相同预处理；不同 IIR 会额外引入相位。
- ADC 轮询 skew 可能随配置改变；采样顺序/触发方式变化后必须重校。

## MCU / RAM 与 Primitive

- 单频补偿 O(1)，复用 `SignalChannelDelayCalibration_Compute/Apply`。
- 相关法 O(NL)，复用 Correlation；多频拟合 O(K)。
- 缺少相关峰亚样本插值、phase unwrap/line fit、fractional-delay FIR 设计/应用。前两项 P0/P1；fractional-delay 先 P2，避免未经验证的滤波器改变幅相。

Recipe 状态：`DRAFT`。

## 20 项执行契约

| # | 字段 | 本 Recipe 的约束 |
|---|---|---|
| 1 | 用途 | 多通道固定采样/前端 delay 校准与相位结果补偿 |
| 2 | 输入 | 同源宽带波形或多频相位、参考通道、Fs |
| 3 | 输出 | 每通道 `delay_s/samples`、残差、有效频带、corrected phase |
| 4 | 完整逻辑链 | 相关法或多频相位斜率；正式测量仅补标量或明确做波形对齐 |
| 5 | 步骤原因 | 单频存在整周歧义；相位斜率/宽带相关消歧并检查频率相关误差 |
| 6 | 默认算法 | 宽带事件用 Correlation；小先验单频用现有 ChannelDelayCalibration |
| 7 | 可选增强 | 相关峰亚样本插值、多频 unwrap+线性拟合、fractional-delay FIR |
| 8 | 适用条件 | 采样拓扑固定、同源信号、最大 delay/频带有先验 |
| 9 | 不适用条件 | 纯周期相关多峰未消歧、配置变更后沿用旧校准、非线性相位强 |
| 10 | 采样率 | delay 基础分辨率 `1/Fs`，校准频带覆盖应用频带 |
| 11 | 点数/周期数 | 相关记录覆盖最大 lag；多频至少 3 点、每点 5～20 周期 |
| 12 | 抗噪 | 多帧 delay Median/MAD、相关系数/拟合残差门 |
| 13 | 精度增强 | 峰插值、多频斜率、同步触发、每配置/温度重校 |
| 14 | 计算量 | 单频 O(1)，相关 O(NL)，多频拟合 O(K) |
| 15 | RAM | 相关输出 `4(2L+1)`；单频/多频结果 O(K) |
| 16 | Primitive | Correlation、ChannelDelayCalibration；phase unwrap/line fit/peak interpolation 缺失 |
| 17 | 仓库路径 | `04_dsp/correlation`、`05_precision/channel_delay_calibration` |
| 18 | 伪代码 | `common stimulus -> delay estimate -> robust combine -> bind config -> phase correction` |
| 19 | MCU 调用 | 见下方真实单频 Compute/Apply API |
| 20 | 失败排查 | 查相关 lag 符号、B-A 定义、wrap/整周、最大 lag、采样顺序和配置版本 |

```c
SignalChannelDelayCalibration_Compute(measured_phase_deg, expected_phase_deg,
                                      frequency_hz, &calibration);
SignalChannelDelayCalibration_Apply(measured_phase_deg, frequency_hz,
                                    &calibration, &corrected_phase_deg);
```
