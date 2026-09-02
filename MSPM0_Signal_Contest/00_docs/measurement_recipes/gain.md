# Gain / Gain dB：增益与分贝

## 输入与输出

- 输入：同一频率、同一稳态条件下的输入与输出波形 `Vin[N]`、`Vout[N]`，每通道 `Fs`，目标频率可已知或先估计。
- 输出：线性幅值增益 `gain=|Vout|/|Vin|`、`gain_db=20*log10(gain)`，可选相位差。

## 逻辑链

```text
双通道同步采样 → 两路独立 ADC/前端校准 → 削顶/噪声底检查
→ 用同一种方法测输入、输出目标分量幅值
   已知单音：SineFit3 或 LockIn
   未知单音：同窗FFT + 同一频率bin/多bin能量
   任意重复波形总体：AC RMS（题目明确时）
→ gain=Aout/Ain → gain_db=20log10(gain)
→ 可选 Phase Recipe 得到相位
```

### 为什么必须同一种幅值定义

输入用 Vpp、输出用 RMS 会产生定义错误；即使都叫“幅值”，FFT 单 bin、基波幅值和整波 RMS 也不等价。若目标是传递函数，应测同一频率分量，而不是让输出谐波混入 AC RMS。

## 推荐与备选

| 场景 | 默认 | 备选 | 失效条件 |
|---|---|---|---|
| DDS 频率已知、弱输入 | 两路 LockIn 或 SineFit3 | 相干 FFT | 参考不同步、频率设定不等于实际值 |
| 未知单音 | FFT 找 f0，再两路同频幅值 | SineFit4 窄带细化 | 两路各自找不同峰 |
| 宽带重复波形总体比例 | AC RMS | Vpp | DUT 改变波形形状却仍称频率增益 |

## 采样与记录

- 两路应同步；轮询采样会影响相位但对慢变化幅值影响较小，仍应记录通道时序。
- 至少覆盖 5～20 周期；LockIn/FFT 最好整数周期。若测到 Hn，还要满足 Nyquist 和抗混叠。
- `Ain` 必须显著高于噪声底且不接近 0；输入接近 0 时增益比会爆炸，应返回无效而不是饱和值。

## 抗噪声与精度

- 用残差 RMS、SNR 或多帧离散度做质量门限。
- 两通道若量程不同，分别做 ADC gain/offset 和前端增益标定；比值只能消去共同误差，不能消去通道间误差。
- 频率扫频中应固定或记录每个量程/VGA 状态；改变增益后等待建立再采样。
- `20*log10` 是标量 Recipe；M0+ 上若只需显示，可在扫频完成后计算，或用校准 LUT+线性插值减少频繁 `log10f`。

## MCU / RAM 与 Primitive

- AC RMS：每路 O(N)、O(1)。LockIn/SineFit3：每路 O(N)、O(1)。FFT：每路 O(N log N)，可串行复用一套 FFT workspace，前提是保留所需复数相位/幅值结果。
- 复用 LockIn、SineFit3、AC RMS Direct Recipe、Window/FFT/Magnitude/MultiBinEnergy/WindowGainCorrection、Phase 和 ADC calibration。

Recipe 状态：`DRAFT`。

## 20 项执行契约

| # | 字段 | 本 Recipe 的约束 |
|---|---|---|
| 1 | 用途 | 线性电压增益与 `gain_db` |
| 2 | 输入 | 同稳态 Vin/Vout、每通道 Fs、目标频率/幅值定义 |
| 3 | 输出 | `gain=Aout/Ain`、`20log10(gain)`、可选相位 |
| 4 | 完整逻辑链 | 见“逻辑链”；两路必须同一种幅值定义 |
| 5 | 步骤原因 | 通道校准、削顶/噪声门、同频分量避免定义错配 |
| 6 | 默认算法 | 已知单音两路 LockIn/SineFit3；总体波形按题意 AC RMS |
| 7 | 可选增强 | 同窗 FFT/多 bin、SineFit4、Phase Recipe |
| 8 | 适用条件 | 同步或幅值可比、Ain 明显高于噪声、稳态 |
| 9 | 不适用条件 | Ain≈0、任一路削顶、两路各找不同峰、量程未校准 |
| 10 | 采样率 | 覆盖目标及谐波带宽；幅相同测建议约 20 点/周期 |
| 11 | 点数/周期数 | 建议 5～20 周期；每扫频点 2～3 稳态帧 |
| 12 | 抗噪 | 残差/SNR 门、多帧 gain Median/MAD、弱单音 LockIn |
| 13 | 精度增强 | 两通道独立校准、同时采样、频响/thru 补偿 |
| 14 | 计算量 | AC RMS/LockIn/SineFit O(N)，FFT O(N log N) |
| 15 | RAM | 基础/拟合 O(1)；FFT 可串行复用约 `10N` 工作区 |
| 16 | Primitive | LockIn、SineFit3、AC RMS、FFT/Magnitude/MultiBin/WindowGain、Phase、ADC Calibration |
| 17 | 仓库路径 | `05_precision/{lock_in,sine_fit_3param}`、`00_docs/recipes/ac_rms.md` |
| 18 | 伪代码 | `measure Ain/Aout identically -> reject invalid -> divide -> log10 -> optional phase` |
| 19 | MCU 调用 | 下方示例使用真实 SineFit3 API；dB 换算留在 Application |
| 20 | 失败排查 | 查幅值定义、Ain 下限、通道校准、量程、同步、f0、残差与 log 输入 |

```c
SignalSineFit3Param_Process(vin_v, N, &fit_cfg, &in_fit);
SignalSineFit3Param_Process(vout_v, N, &fit_cfg, &out_fit);
gain = out_fit.amplitude_peak_v / in_fit.amplitude_peak_v;
gain_db = 20.0f * log10f(gain); /* 应用标量公式 */
```
