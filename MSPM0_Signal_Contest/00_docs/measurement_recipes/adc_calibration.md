# ADC Calibration：ADC 增益、偏置与线性校准

## 输入与输出

- 输入：若干个可追溯参考电压 `true_v[k]`、ADC 测得的 `measured_v[k]` 或 raw code、量程/VREF/温度信息。
- 输出：`corrected=gain*measured+offset` 的参数、残差/最大误差、适用量程和版本；多点时可输出 LUT。

## 两点校准逻辑链

```text
预热并固定VREF/量程 → 接低参考点 → 多次采样 → 稳健平均 measured_low
→ 接高参考点 → 多次采样 → measured_high
→ Compute(gain, offset)
→ 用独立中间点验证，不用参与拟合的数据自证
→ 保存参数及温度/量程/VREF
→ 正式数据：raw转名义电压 → Apply校准 → 测量Recipe
```

低高两点分开增益和偏置；中间验证点用于发现非线性、参考错误或饱和。校准参数必须与具体 ADC 通道、参考、前端增益和量程绑定。

## 为什么采用这条链

多次采样先降低参考点随机噪声；两个相距足够远的点分别确定斜率和截距；独立中间点用于发现“模型不适用”，避免用参与拟合的数据验证自己。默认推荐两点 affine；残差超指标且映射仍单调时，备选才是多点 LUT。

## 多点/LUT 分支

当两点残差超过指标：

```text
多个递增参考点 → 每点多帧稳健统计 → 检查单调性
→ 保存 measured→true LUT → 区间内线性插值
→ 留出独立验证点 → 报最大残差与表外状态
```

LUT+线性插值比 MCU 上高阶多项式更可控、计算更轻；表外默认拒绝或夹紧并报警，不静默外推。

## 推荐、备选与失效

| 情况 | 默认 | 备选 | 失效条件 |
|---|---|---|---|
| 固定量程、误差近似线性 | 两点 gain/offset | 多点最小二乘 | 参考点太近、任一点削顶 |
| 可重复非线性 | 分段 LUT | 低阶多项式 | 非单调、温漂/迟滞明显 |
| 多量程 | 每档独立校准 | 共享模型+档位参数 | 一套参数跨所有档 |

## 采样、抗噪声与精度

- 每点至少数十到数百次样本，先检查稳定和异常；多帧 Median/MAD 后平均。
- 这是静态校准，不要求覆盖信号周期；建议每点 `N>=32` 起步，并按标准差/题目误差决定是否增加到 128～1024 点。
- 参考源精度必须优于目标；VREF/前端/分压均包含在校准链中时，应明确参数校准的是“ADC 引脚”还是“系统输入端”。
- 校准不能修复削顶、带宽不足、采样时钟误差或高频前端衰减；频率相关误差走 Frequency Response Compensation。
- 上电温度、供电和时间漂移超出校准条件时需要重校或温度补偿。

## MCU / RAM 与现有 Primitive

- 两点 Compute/Apply 为 O(1)/O(N)，无大型 workspace，复用 `SignalADCGainOffsetCalibration_Compute` 与 `SignalADCGainOffsetCalibration_Apply`。
- MAD/Statistics 用于参考点稳定性。
- 通用 LUT 插值目前缺失，列为 P0 `calibration_lut_1d` 候选；正式实现必须有单调性、边界、表外和 golden tests。

Recipe 状态：`DRAFT`；ADC gain/offset Primitive 已 `PC_VERIFIED`，没有板级校准证据时不能写 BOARD_VERIFIED。

## 20 项执行契约

| # | 字段 | 本 Recipe 的约束 |
|---|---|---|
| 1 | 用途 | 每通道/量程 ADC 系统增益、偏置和可重复非线性校准 |
| 2 | 输入 | 可追溯 `true_v[k]`、measured/raw、VREF/量程/温度 |
| 3 | 输出 | affine 参数或单调 LUT、残差、范围和版本 |
| 4 | 完整逻辑链 | 两点校准链；残差超标才进入多点/LUT 分支 |
| 5 | 步骤原因 | 低高点分离 gain/offset，独立中间点防止拟合自证 |
| 6 | 默认算法 | 两点 `corrected=gain*measured+offset` |
| 7 | 可选增强 | 多点线性、分段 LUT、低阶多项式/温度分区 |
| 8 | 适用条件 | 参考源更准、量程/VREF/前端固定、关系可重复 |
| 9 | 不适用条件 | 削顶、迟滞/非单调、温漂超标、用同一数据拟合又验证 |
| 10 | 采样率 | 静态校准按建立时间设置，无需追求最高 Fs |
| 11 | 点数/周期数 | 每参考点数十～数百样本；至少留 1 个独立验证点 |
| 12 | 抗噪 | 每点 Median/MAD 后平均，记录 stddev/稳定性 |
| 13 | 精度增强 | 扩展覆盖范围/温度、独立验证、LUT 密化而非盲目高阶 |
| 14 | 计算量 | Compute O(1)，Apply O(N)，LUT O(log K) |
| 15 | RAM | affine O(1)；LUT 约每点 8 字节 + 元数据 |
| 16 | Primitive | ADCGainOffsetCalibration、MAD、Statistics；通用 LUT 尚缺 |
| 17 | 仓库路径 | `05_precision/adc_gain_offset_calibration`、`04_dsp/mad`、`03_measurement/statistics` |
| 18 | 伪代码 | `stabilize -> collect low/high -> compute -> independent validate -> bind metadata -> apply` |
| 19 | MCU 调用 | 见下方真实 Compute/Apply API |
| 20 | 失败排查 | 查参考真值、点间距、单位、通道/档位/VREF 绑定、独立残差和表外 |

```c
SignalADCGainOffsetCalibration_Compute(measured_low_v, true_low_v,
                                       measured_high_v, true_high_v, &cal);
SignalADCGainOffsetCalibration_Apply(input_v, corrected_v, N, &cal);
```
