# Harmonic Amplitude / THD / SNR / SINAD：频谱质量指标

## 输入与输出

- 输入：校准后的 `voltage_v[N]`、`Fs`、预期基波范围、最高谐波阶数和分析带宽。
- 输出：`fundamental_frequency_hz`、H1～Hm 幅值/能量、`thd_percent`、`snr_db`、`sinad_db`，可选 SFDR。

## 统一前半链

```text
电压帧 → 削顶检查 → RemoveDC（DC不属于目标时）
→ 相干性判断
   相干：Rectangular（或题目规定窗）
   非相干：Hann
→ FFT → Magnitude
→ 排除DC/限定频段找基波峰
→ 峰值插值得到更准 f0
→ 以真实 f0 定位 H1...Hm
→ 每阶按窗主瓣宽度做 MultiBinEnergy
```

### 为什么不能只写“做 FFT”

- 非相干采样会把一条谱线泄漏到多个 bin，单 bin 谐波会漏能量。
- 用整数峰 bin 的倍数定位谐波会累积 f0 量化误差；应先细化基波频率，再用 `h*f0` 定位。
- 绝对谐波电压需要 N、单边和 coherent gain 修正；THD 的同标度能量比可消去公共比例。
- 窗函数改变噪声等效带宽。只做相对能量比与报告绝对噪声密度是两件事。

## THD Recipe

```text
Harmonic energies E1...Em
→ THD=sqrt((E2+...+Em)/E1)
→ THD%=100*THD
```

推荐复用 `SignalHarmonic_Process` 和 `SignalTHD_Process`。Hann 非相干谱可从 `radius_bins=2` 起做 PC/板级扫测；各谐波积分带不能重叠，也不能越过 Nyquist。

## 谐波幅值 Recipe

1. 用 Harmonic 给出每阶 `root_sum_square`。
2. 相干单 bin 可对 raw magnitude 使用 WindowGainCorrection 得到 Vpeak。
3. 非相干多 bin 的 RMS 合成还需一致的窗/能量标定；优先用已知正弦实测建立幅值校准因子，不把 RSS 直接无条件叫 Vpeak。

## SNR Recipe

```text
定义 signal band（基波主瓣）
→ 定义 analysis band
→ 从 noise 中排除 DC、基波 band、各次谐波 band、已知杂散
→ SNR=10log10(Psignal/Pnoise)
```

现有 `SignalSNR_Process` 支持 signal band、analysis band 和 excluded ranges。若没有排除谐波，结果不是严格 SNR。

## SINAD Recipe

```text
同一分析带内：
Psignal = 基波band能量
Pnoise_plus_distortion = 除DC和基波band外的所有有效能量
SINAD = 10log10(Psignal/Pnoise_plus_distortion)
```

可用现有 SNR Primitive，但 `excluded_ranges` 只排除 DC，不排除谐波，并把结果字段在 Application 中明确重命名为 SINAD；不要修改 Primitive 或把 `snr_db` 不加说明地直接显示为 SINAD。

## 推荐、备选与失效

| 场景 | 推荐 | 备选 | 失效条件 |
|---|---|---|---|
| 单音谐波/THD | Hann+多 bin harmonic | 相干 Rectangular+单 bin | Hm 超 Nyquist、削顶、邻频重叠 |
| 高精度绝对谐波幅值 | 相干采样+coherent gain | 标准源幅值校准 | 前端频响未补偿 |
| 严格 SNR | 明确排除谐波/杂散 | 时域残差（已知模型） | 分析带/窗 ENBW 未定义 |
| SINAD | 除 DC/基波外全部计入 | SineFit residual | 把谐波错误排除后仍叫 SINAD |

## 采样与记录要求

- `Fs > 2*m*f0` 只是数学下限；还要留抗混叠滤波过渡带。建议记录至少 5～20 个基波周期。
- N 决定 bin 宽 `Fs/N`；要区分相邻谐波/杂散，主瓣不能重叠。
- 严格动态性能测试优先相干采样；无法相干时固定窗、N、积分半径和分析带，使不同测量可比较。

## 抗噪声与精度

- 不要在 THD 前加会削弱 H2～Hm 的低通或 Hampel；那会改变被测失真。
- 多帧对能量求和/平均后再开方；对 dB 直接平均会引入不同统计含义。
- 做 ADC/DAC/前端频响校准，尤其各谐波跨越大频率范围时。

## MCU / RAM 与 Primitive

- FFT O(N log N)，Magnitude O(N/2)，Harmonic/THD O(m*radius)，SNR O(bins)。
- RAM 典型 `complex 8N + magnitude 2N` 字节；Harmonic result 为固定小结构。
- 复用 Window、FFT、Magnitude、Peak、两种峰插值、MultiBinEnergy、Harmonic、THD、SNR、SFDR、WindowGainCorrection。

## 缺口

SINAD 公式很短，当前保持 Recipe-local；若未来多个 Application 反复需要统一的 DC/基波/谐波 band 规划与指标质量标志，再升级为 `spectral_quality_plan` Primitive。Recipe 状态：`DRAFT`。

## 20 项执行契约

| # | 字段 | 本 Recipe 的约束 |
|---|---|---|
| 1 | 用途 | H1～Hm、THD、SNR、SINAD、SFDR、波形失真 |
| 2 | 输入 | 校准 `voltage_v[N]`、Fs、f0 范围、谐波阶数、分析带 |
| 3 | 输出 | f0、各阶能量/幅值、THD%、SNR/SINAD/SFDR dB |
| 4 | 完整逻辑链 | 统一前半链后按 THD/SNR/SINAD 的频带定义分支 |
| 5 | 步骤原因 | 见“为什么不能只写做 FFT”及各指标定义 |
| 6 | 默认算法 | 非相干 Hann + FFT + 主峰插值 + 多 bin harmonic energy |
| 7 | 可选增强 | 相干 Rectangular、SineFit residual、频响校准、多帧能量平均 |
| 8 | 适用条件 | 稳态单音为主、分析带和排除范围明确 |
| 9 | 不适用条件 | 削顶、谐波越 Nyquist、主瓣重叠、把未排谐波结果叫严格 SNR |
| 10 | 采样率 | `Fs > 2*m*f0` 且留抗混叠过渡带 |
| 11 | 点数/周期数 | 至少 5～20 基波周期；N 使主瓣/邻频可分离 |
| 12 | 抗噪 | 固定分析带、能量域多帧平均、质量门；不在 THD 前删谐波 |
| 13 | 精度增强 | f0 插值、多 bin 半径扫测、窗增益/ENBW、各谐波频响校准 |
| 14 | 计算量 | O(N log N + bins + m·radius) |
| 15 | RAM | 典型复谱 `8N` + magnitude `≈2N` 字节 + 时域/窗 buffer |
| 16 | Primitive | Window、FFT、Magnitude、Peak/Interpolation、MultiBinEnergy、Harmonic、THD、SNR、SFDR |
| 17 | 仓库路径 | `04_dsp/{window,fft,fft_magnitude,peak_detect,harmonic,thd,snr,sfdr}`、`05_precision/multi_bin_energy` |
| 18 | 伪代码 | `validate -> spectrum -> f0 -> harmonic bands -> metric-specific include/exclude -> quality` |
| 19 | MCU 调用 | 见下方真实 Harmonic/THD API；SINAD 用 SNR API 时必须重命名语义 |
| 20 | 失败排查 | 查削顶、窗/N 标度、f0、radius 重叠、Nyquist、DC/谐波排除、噪声分析带 |

```c
SignalHarmonic_Process(magnitude, bin_count, sample_rate_hz, N,
                       &harmonic_cfg, &harmonics);
SignalTHD_Process(&harmonics, &thd);
SignalSNR_Process(magnitude, bin_count, &snr_cfg, &snr);
```
