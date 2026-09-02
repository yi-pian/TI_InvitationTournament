# Amplitude：幅值测量决策 Recipe

“幅值”必须先定义成 Vpeak、Vpp、总 RMS、AC RMS、基波峰值或某一已知频率分量的幅值；这些量不能混称。

## 输入与输出

- 输入：校准后的 `voltage_v[N]`、`Fs`，以及可选已知频率 `f0`。
- 输出：明确单位和定义的 `amplitude_peak_v`、`vpp_v`、`rms_v`、`ac_rms_v` 或目标频率幅值。

## 逻辑链

```text
ADC raw → 电压换算 → ADC/前端增益偏置校准 → 削顶检查
→ 根据“幅值定义”和波形选择：
   任意时域波形：Min/Max → Vpp
   只要能量：RMS 或 AC RMS
   已知频率单音：SineFit3 或 LockIn
   未知单音/频谱：RemoveDC → Window → FFT → Magnitude
                    → Peak/Interpolation → coherent-gain幅值修正
```

### 为什么这样分支

- Vpp 对波形形状没有正弦假设，但非常依赖是否采到真实峰谷。
- RMS 衡量整段能量；总 RMS 包含 DC，AC RMS 去掉均值。
- 正弦的 `Vpeak=Vpp/2=sqrt(2)*AC_RMS` 只对没有明显失真的正弦成立。
- SineFit3/LockIn 把无关频率当残差或积分掉，适合弱单音；普通 Vpp 会把宽带噪声也当幅值。
- FFT 的原始 magnitude 不是电压峰值，必须按 N、单边规则和 Window coherent gain 修正。

## 推荐与备选

| 场景 | 默认 | 备选 | 不应使用 |
|---|---|---|---|
| 任意周期波形绝对跨度 | Vpp | Robust Vpp（仅毛刺不是目标） | 用正弦 RMS 公式反推 |
| 包含 DC 的功率相关幅值 | RMS | Statistics 的方差+均值交叉检查 | RemoveDC 后再声称总 RMS |
| 只要交流能量 | AC RMS | RemoveDC→RMS | Mean 代表交流幅值 |
| 已知频率弱正弦 | LockIn | SineFit3 | 全带宽 Vpp |
| 未知正弦 | FFT+插值+增益修正 | SineFit4 窄带细化 | 单 bin 不做窗/增益修正 |

## 条件、采样与失效

- 时域幅值至少覆盖完整峰和谷；建议 3～10 周期。非整数周期的 RMS/均值会产生端点偏差，增加周期数或做相干截取。
- 最高目标频率至少保留约 10～20 点/周期以可靠重建峰值；只满足 Nyquist 不等于能准确测峰。
- FFT 幅值建议相干采样；否则用 Hann + 多 bin 能量或拟合。窗修正不能补偿 ADC/前端频响。
- 输入削顶、低于噪声底、AGC 尚未稳定、前端带宽不足时结果无效。

## 抗噪声与精度

- 偶发错误码：先诊断硬件；确认不是目标尖峰后使用 Hampel、RobustVPP 或 RobustRMS，并报告替换/钳位数量。
- 弱已知单音：增加相干积分时间，用 LockIn；参考不同步时会衰减并产生相位漂移。
- 多帧结果用 Median/MAD 剔除整帧异常，再平均；不要对原始谐波波形盲目平滑。
- 应用 ADC gain/offset、分压/放大比例和频率响应校准。

## MCU / RAM 与现有 Primitive

- Vpp/RMS/AC RMS：O(N)，额外 O(1)；Direct Recipe 优先。
- LockIn/SineFit3：O(N)，O(1) workspace，软件三角运算有 CPU 成本。
- FFT：O(N log N)，RAM 约 `complex 8N + magnitude 2N` 字节。
- 复用：ADC To Voltage Recipe 或兼容 API、Clipping Detect Recipe、`SignalRobustPeakToPeak_Process`、`SignalRobustRMS_Process`、`SignalLockIn_Process`、`SignalSineFit3Param_Process`、Window/FFT/Magnitude/Peak/Interpolation/WindowGainCorrection。

Recipe 状态：`DRAFT`。具体 Vpp、RMS、AC RMS、DC 链见相邻 Recipe。

## 20 项执行契约

| # | 字段 | 本 Recipe 的约束 |
|---|---|---|
| 1 | 用途 | 在 Vpeak/Vpp/RMS/AC RMS/基波幅值中选择正确幅值定义 |
| 2 | 输入 | 校准 `voltage_v[N]`、`Fs`、可选 `f0` 与定义 |
| 3 | 输出 | 带 Vpeak/Vpp/RMS 等明确语义和 V 单位的值 |
| 4 | 完整逻辑链 | 见“逻辑链”分支 |
| 5 | 步骤原因 | 见“为什么这样分支” |
| 6 | 默认算法 | 任意波形 Vpp/AC RMS；已知弱单音 LockIn；未知单音 FFT 链 |
| 7 | 可选增强 | Robust VPP/RMS、SineFit3/4、多 bin 能量 |
| 8 | 适用条件 | 幅值定义、测量带宽和波形条件明确 |
| 9 | 不适用条件 | 削顶、AGC 未锁定、低于噪声底、用正弦公式处理非正弦 |
| 10 | 采样率 | 时域峰值建议目标最高频率有 10～20 点/周期 |
| 11 | 点数/周期数 | 建议覆盖 3～10 周期；弱信号 5～20 周期或更长相干积分 |
| 12 | 抗噪 | 多帧 Median/MAD；确认毛刺非目标后才 Robust/Hampel |
| 13 | 精度增强 | ADC/前端/频响校准，相干采样，coherent-gain 修正 |
| 14 | 计算量 | Vpp/RMS O(N)，LockIn/SineFit O(N)，FFT O(N log N) |
| 15 | RAM | 基础链 O(1)；Robust 需 `4N` workspace；FFT 约 `10N` 字节工作区 |
| 16 | Primitive | Clipping、RobustPeakToPeak、RobustRMS、LockIn、SineFit3、Window/FFT/Magnitude/Peak/Interpolation |
| 17 | 仓库路径 | `00_docs/recipes/{vpp,rms,ac_rms}.md`、`05_precision/{robust_peak_to_peak,robust_rms,lock_in,sine_fit_3param}` |
| 18 | 伪代码 | `validate -> choose amplitude definition -> measure with one consistent method -> calibrate/report` |
| 19 | MCU 调用 | 见下方弱单音真实 API 示例；普通量使用相邻 Direct Recipe |
| 20 | 失败排查 | 依次查定义、单位、削顶、覆盖周期、窗/FFT 标度、量程和校准 |

```c
signal_lock_in_config_t cfg = {f0_hz, sample_rate_hz, 0.0f, 1U};
signal_lock_in_result_t result;
signal_algorithm_status_t status = SignalLockIn_Process(voltage_v, N, &cfg, &result);
/* status==SIGNAL_ALGORITHM_OK 后，result.amplitude_peak_v 才有效。 */
```
