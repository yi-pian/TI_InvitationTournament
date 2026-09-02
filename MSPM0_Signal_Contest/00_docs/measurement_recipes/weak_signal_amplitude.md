# Weak Signal Amplitude：弱信号幅值

## 输入与输出

- 输入：`voltage_v[N]`、`Fs`，通常还需要已知或可靠粗估的目标频率和参考相位。
- 输出：目标频率 `amplitude_peak_v`、相位、DC、残差 RMS 或检测置信度。

## 逻辑链

```text
ADC/前端低噪声与增益校准 → 削顶检查
→ 目标频率已知且参考同步：LockIn I/Q积分
→ 频率已知但参考相位未知：SineFit3
→ 只有可靠粗频率：FFT粗定位 → 窄范围SineFit4
→ 多帧质量门限（残差/SNR/幅值） → I/Q或幅值平均
```

LockIn 通过与目标正交参考相乘并积分，把测量带宽压到约观察时间的倒数；SineFit3 用已知频率最小二乘同时估计正弦、余弦和 DC。两者都比全带宽 Vpp 更适合弱单音。

## 推荐与失效

| 方法 | USE WHEN | DON'T USE WHEN |
|---|---|---|
| LockIn | 参考频率/相位与采样同步，可延长积分 | 独立时钟明显漂移、目标频率未知 |
| SineFit3 | f0 已知，想同时得到幅值/相位/DC/残差 | 多音、削顶、f0 错误 |
| FFT→SineFit4 | 只有可靠粗频率且单音 | 搜索范围跨多个主瓣；当前 SineFit4 验证边界较窄 |

默认有同步参考就用 LockIn；准确频率已知但参考相位不可靠时，SineFit3 是备选；只有粗频率时，FFT 粗定位后再用窄范围 SineFit4，不能把 SineFit4 当全带搜索器。

## 采样、抗噪声与精度

- 最好覆盖整数周期，至少 5～20 周期；更长积分提高窄带 SNR，但会抹去幅值/频率随时间变化。
- 前端增益先把弱信号提高到 ADC 有效码范围，但必须保留 DC/过冲余量；AGC 稳定后才采测量帧。
- 对 I/Q 做平均比对幅值直接平均更合理；独立时钟漂移时应分短块估计相位旋转，当前无正式跟踪 Primitive。
- ADC 增益/偏置、前端频响和参考相位延迟都需校准。

## MCU / RAM 与 Primitive

- LockIn、SineFit3：O(N)、O(1)；M0+ 软件 `sin/cos` 有成本，但不需 FFT 大数组。
- FFT+SineFit4：O(N log N + I*N)，高 CPU。
- 复用 `SignalLockIn_Process`、`SignalSineFit3Param_Process`、`SignalSineFit4Param_Process`、FFT 链、ADC Calibration、Channel Delay Calibration。

Recipe 状态：`DRAFT`。

## 20 项执行契约

| # | 字段 | 本 Recipe 的约束 |
|---|---|---|
| 1 | 用途 | 已知/粗知频率的弱正弦幅值、相位和 DC |
| 2 | 输入 | `voltage_v[N]`、`Fs`、参考或粗频率、可选参考相位 |
| 3 | 输出 | 目标 `amplitude_peak_v`、phase、DC、残差/置信度 |
| 4 | 完整逻辑链 | 见“逻辑链” |
| 5 | 步骤原因 | 窄带相干积分或拟合排除无关带宽噪声 |
| 6 | 默认算法 | 同步参考用 LockIn；只知准确频率用 SineFit3 |
| 7 | 可选增强 | FFT 粗定位 + 窄范围 SineFit4、分块相位跟踪 |
| 8 | 适用条件 | 近单音、频率证据可靠、可延长观察时间 |
| 9 | 不适用条件 | 多音、削顶、参考漂移明显、SineFit4 搜索范围过宽 |
| 10 | 采样率 | 满足模拟带宽与目标频率；不靠超高 Fs 代替积分时间 |
| 11 | 点数/周期数 | 最好整数周期，至少 5～20 周期；更弱信号延长相干积分 |
| 12 | 抗噪 | 窄带 LockIn、残差/SNR 质量门、多帧 I/Q 平均 |
| 13 | 精度增强 | 参考/采样同步、前端增益和频响校准、通道相位校准 |
| 14 | 计算量 | LockIn/SineFit3 O(N)，FFT+SineFit4 O(N log N+I·N) |
| 15 | RAM | LockIn/SineFit3 O(1)，FFT 约 `10N` 字节工作区 |
| 16 | Primitive | LockIn、SineFit3、SineFit4、FFT 链、ADC/Delay Calibration |
| 17 | 仓库路径 | `05_precision/{lock_in,sine_fit_3param,sine_fit_4param}` |
| 18 | 伪代码 | `validate -> choose known-reference method -> integrate/fit -> quality gate -> average I/Q` |
| 19 | MCU 调用 | 见下方真实 LockIn API |
| 20 | 失败排查 | 查参考频率/相位、整数周期、时钟漂移、前端噪声/增益、残差和削顶 |

```c
signal_lock_in_config_t cfg = {reference_hz, sample_rate_hz, reference_phase_rad, 1U};
signal_lock_in_result_t result;
signal_algorithm_status_t status = SignalLockIn_Process(voltage_v, N, &cfg, &result);
```
