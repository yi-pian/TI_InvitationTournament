# Phase Difference / Time Delay：相位差与时间延迟

## 输入与输出

- 输入：同步或已知时序关系的两路 `A[N]`、`B[N]`，每通道 `Fs`，可选目标频率 `f0`。
- 输出：统一符号的 `phase_B_minus_A_deg`；时间延迟 `delay_B_relative_to_A_s` 为正表示 B 更晚。

## 相位差逻辑链

```text
双通道raw → 分通道电压换算/增益偏置校准 → 分别去DC
→ 先补偿固定通道时延
→ 按信号选择：
   纯正弦高SNR：两路同方向crossing插值 → 配对 → Phase_FromZeroCross
   已知目标频率：两路同窗FFT或SineFit3 → 目标频率相角相减
   波形相似但非正弦：互相关 → 相关峰lag → Phase_FromCorrelationLag
→ wrap到[-180°,180°) → 多帧圆周平均/稳定性检查
```

### 为什么每一步存在

- 双通道必须同一物理时刻对应；轮询 ADC 的固定 skew 会变成随频率线性增加的假相位。
- 两路用相同预处理、窗和 FFT 标度，才能让公共延迟和相位定义可比较。
- 相位是圆周量，179° 与 -179° 不能做普通算术平均；可先平均 `sin/cos` 再 `atan2`，这段小公式留在 Recipe。
- 前端滤波器本身有频率相关相位，必须用 thru 校准扣除，不能只校准 ADC 数字延迟。

## 时间延迟逻辑链

```text
宽带/非周期相似波形：RemoveDC → 可选限带 → CrossCorrelation
→ 整数lag峰 → 峰顶三点插值（候选Primitive） → delay=lag/Fs

单频正弦：相位差 → delay=-phase/(360*f)
→ 结合已知最大延迟处理整周期歧义
```

单频相位只能得到模一个周期的延迟；若可能超过半周期，必须用宽带相关、多个频点的相位斜率或外部先验消除歧义。

## 推荐与失效条件

| 方法 | 适用 | 优点 | 失效条件 |
|---|---|---|---|
| ZeroCross phase | 同频纯正弦、高 SNR | RAM/CPU 小 | 谐波移动过零点；边沿配对错误 |
| FFT phase | 已做频谱、关注某一频率 | 可隔离基波 | 目标 bin 泄漏/能量太小；两路时间窗不一致 |
| SineFit3 phase | f0 已知且单音 | 同时给幅值/DC/残差 | f0 错误、多音、削顶 |
| Cross-correlation | 宽带或非正弦、两路形状相似 | 直接得到 delay | O(NL)；周期波形有多个等价峰 |
| 多频相位斜率 | 校准固定通道延迟 | 抗单点 360° 歧义 | 未正确 unwrap、前端自身相位不线性 |

默认优先级是：纯正弦先 FFT/SineFit，相似非正弦先互相关。高 SNR 且只需最低资源时，ZeroCross 是备选；单频存在整周歧义时，多频相位斜率或宽带相关是备选。

## 采样、周期、抗噪声

- 正弦相位建议每周期至少约 20 点并覆盖 5～20 周期；FFT N 为 2 次幂，目标频率最好相干或用 Hann。
- 时间延迟相关法的时间量化为 `1/Fs`；相关峰插值可在高 SNR、峰形平滑时提高亚样本精度，但不能补回模拟带宽丢失。
- 相关前可做不会破坏相对延迟的同系数线性相位滤波；不同通道各自使用不同 IIR 会引入新相位差。
- 多帧先用相关系数/残差筛掉低质量帧，再做圆周平均或 delay 平均。

## MCU / RAM 与现有 Primitive

- ZeroCross：O(N)+O(E)。FFT：O(N log N)，典型约 10N 字节 FFT/magnitude 工作区。
- Correlation：O(NL)，输出 `4*(2L+1)` 字节；根据最大物理延迟限制 L。
- 复用 `SignalPhase_FromZeroCross`、`SignalPhase_FromFFTBin`、`SignalPhase_FromCorrelationLag`、`SignalCorrelation_Process`、ZeroCross/Interpolation、FFT、SineFit3、`SignalChannelDelayCalibration_Compute/Apply`。

## 校准边界与缺口

现有 ChannelDelayCalibration 用单频已知真值计算固定 delay，校准前延迟必须小于半周期或已知整周数。更稳健的“多频相位 unwrap + 斜率拟合”和“相关峰亚样本插值”尚无正式 Primitive，列入缺口。Recipe 状态：`DRAFT`。

## 20 项执行契约

| # | 字段 | 本 Recipe 的约束 |
|---|---|---|
| 1 | 用途 | B-A 相位差与 B 相对 A 时间延迟 |
| 2 | 输入 | 同步/已知时序的 A/B 波形、`Fs`、可选 `f0`/校准 delay |
| 3 | 输出 | `phase_B_minus_A_deg`、`delay_B_relative_to_A_s` 与质量 |
| 4 | 完整逻辑链 | 见相位和时间延迟两条链 |
| 5 | 步骤原因 | 见“为什么每一步存在”；符号、同步和 wrap 必须统一 |
| 6 | 默认算法 | 单音用同向 crossing 或同 bin FFT；宽带相似波形用相关 |
| 7 | 可选增强 | SineFit3、多频相位斜率、相关峰亚样本插值 |
| 8 | 适用条件 | 通道同步/固定 skew 可校准，波形共享可比较特征 |
| 9 | 不适用条件 | 独立时间窗、未知整周期歧义、目标 bin 能量不足、周期相关多峰未消歧 |
| 10 | 采样率 | 正弦建议约 20 点/周期；delay 分辨率基础为 `1/Fs` |
| 11 | 点数/周期数 | 正弦 5～20 周期；相关记录覆盖事件和最大 lag |
| 12 | 抗噪 | 同预处理、相关系数/拟合残差门限、多帧圆周平均 |
| 13 | 精度增强 | 通道 delay/前端相位校准、crossing/相关峰插值、多频 unwrap |
| 14 | 计算量 | crossing O(N)，FFT O(N log N)，相关 O(NL) |
| 15 | RAM | FFT 约每路 `8N` 复谱；相关输出 `4(2L+1)`；可串行复用需先保存结果 |
| 16 | Primitive | Phase adapters、Correlation、ZeroCross/Interpolation、FFT、SineFit3、ChannelDelayCalibration |
| 17 | 仓库路径 | `03_measurement/phase`、`04_dsp/correlation`、`05_precision/channel_delay_calibration` |
| 18 | 伪代码 | `synchronize/calibrate -> common feature -> B-A -> wrap or /Fs -> quality gate` |
| 19 | MCU 调用 | 见下方真实 zero-cross phase API |
| 20 | 失败排查 | 查 B-A 符号、轮询 skew、频率/周期、wrap、bin、相关 lag 符号和前端相位 |

```c
signal_phase_result_t result;
signal_algorithm_status_t status = SignalPhase_FromZeroCross(
    crossing_a_samples, crossing_b_samples, period_samples, &result);
```
