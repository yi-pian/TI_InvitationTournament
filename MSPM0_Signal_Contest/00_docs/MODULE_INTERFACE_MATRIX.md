# 模块接口兼容矩阵

> 本矩阵描述数据能否连接，不代表每一项都值得复制成模块。标为 Level A 的旧 API 仍保留兼容，但新工程使用 [Cookbook Direct Recipe](SIGNAL_ALGORITHM_COOKBOOK.md)；Level B/C 才按 README 复制 `.c/.h`。完整分级见 [ALGORITHM_SIMPLIFICATION_REPORT.md](ALGORITHM_SIMPLIFICATION_REPORT.md)。

`PC_VERIFIED` 表示本独立库已在 PC 用严格 C 编译并运行真值测试；不表示硬件接口存在，也不表示 BOARD_VERIFIED。算法只接收数组、count、Fs/config，不访问寄存器。

## 1 采集与基础物理量

| 模块 | 输入 | 输出 | 单位/原地 | 当前状态 |
|---|---|---|---|---|
| ADC_DMA（Expected Hardware） | Hardware | `const uint16_t *raw`、count、Fs | ADC code；由外设任务维护 | 不在本库实现 |
| DualADC（Expected Hardware） | Hardware | 两路 RAW、共同/各自 Fs | ADC code | 不在本库实现 |
| ADC_ToVoltage | `uint16_t raw` + conversion config | `float voltage_v[]` | code→V；非原地 | PC_VERIFIED |
| ADC Gain/Offset Calibration | float V + 两点校准 | corrected float V | V→V；可原地 | PC_VERIFIED |
| Mean | const float | scalar mean | 保持输入单位 | PC_VERIFIED |
| Statistics | const float | mean/min/max/variance/stddev | 输入单位、平方单位 | PC_VERIFIED |
| MinMax | const float | min/max/index | 保持输入单位 | PC_VERIFIED |
| Vpp | float V | amplitude_vpp_v | V→V | PC_VERIFIED |
| RMS | float V | rms_v | V→V | PC_VERIFIED |
| AC_RMS | float V | mean_v + ac_rms_v | V→V | PC_VERIFIED |
| RemoveDC | float V | centered float V + removed mean | V→V；可原地 | PC_VERIFIED |
| ClippingDetect | float V + rails/tolerance | count/ratio/flag | V→count/ratio | PC_VERIFIED |
| RobustVPP | float V + quantiles + N-workspace | quantile limits + Vpp V | 输入只读 | PC_VERIFIED |
| RobustRMS | float V + quantiles + N-workspace | limits/mean/RMS/count | 输入只读 | PC_VERIFIED |

## 2 时域频率与相位

| 模块 | 输入 | 输出 | 单位/原地 | 当前状态 |
|---|---|---|---|---|
| ZeroCross | float V + threshold/hysteresis/direction | event index pairs | V→sample index | PC_VERIFIED |
| ZeroCrossInterpolation | float V + events + same threshold | fractional positions | sample | PC_VERIFIED |
| MultiCycleAverage | same-direction positions + Fs | frequency/period/time/cycles | sample→Hz/s | PC_VERIFIED |
| Phase adapters | two crossing positions / two complex bins / lag | phase_B-phase_A | deg/rad | PC_VERIFIED |
| Correlation | two float arrays + max lag | coefficients + best lag | dimensionless/sample | PC_VERIFIED |
| Autocorrelation | float array + max lag | coefficients/period/frequency | dimensionless/sample/Hz | PC_VERIFIED |
| ChannelDelayCalibration | measured/expected B-A phase + f | delay_s/corrected phase | deg+Hz→s/deg | PC_VERIFIED |

## 3 调理与滤波

| 模块 | 输入 | 输出/状态 | 原地/工作区 | 当前状态 |
|---|---|---|---|---|
| MovingAverage | float + window | filtered float | 非原地 | PC_VERIFIED |
| Median | float + odd W | filtered float | 非原地；W-float workspace | PC_VERIFIED |
| MAD | float | median/MAD/scaled MAD | N-float workspace | PC_VERIFIED |
| Hampel | float + W/threshold/min scale | filtered + replaced count | 非原地；W workspace | PC_VERIFIED |
| FIR | float + external taps/state | filtered float | 可原地；T-1 state | PC_VERIFIED |
| IIR Biquad SOS | float + external SOS/state | filtered float | 可原地；2S state | PC_VERIFIED |

## 4 FFT 完整链

| 模块 | 输入 | 输出 | 标度/原地 | 当前状态 |
|---|---|---|---|---|
| Rectangular/Hann/Hamming/Blackman | float samples | windowed float + coherent gain | V→V；可原地 | PC_VERIFIED |
| FFT Real | N float，N为2次幂 | N complex | 未归一化 DFT | PC_VERIFIED |
| FFT Complex In-place | N complex | N complex spectrum | `exp(-j2πkn/N)`；原地 | PC_VERIFIED |
| Magnitude | N complex | N/2+1 float | raw DFT magnitude | PC_VERIFIED |
| WindowGainCorrection | magnitude + N + coherent gain | 单边 peak amplitude | magnitude→输入峰值单位；可原地 | PC_VERIFIED |
| PeakDetect | magnitude + bin range | peak index/value | 保持谱单位 | PC_VERIFIED |
| Linear Parabolic | 3 linear magnitude bins + Fs/N | offset/fractional bin/Hz | bin→Hz | PC_VERIFIED |
| Log Parabolic | 3 positive magnitude bins + Fs/N | offset/fractional bin/Hz | bin→Hz | PC_VERIFIED |

## 5 谱指标

| 模块 | 输入 | 输出 | 单位 | 当前状态 |
|---|---|---|---|---|
| MultiBinEnergy | linear magnitude + center/radius | sum magnitude² + RSS | 谱标度²/谱标度 | PC_VERIFIED |
| Harmonic | magnitude + true f0 + order/radius | H1..H10 bin ranges/energy | Hz/bin/谱能量 | PC_VERIFIED |
| THD | Harmonic result | ratio/percent | 无量纲/% | PC_VERIFIED |
| SNR | magnitude + signal/exclude ranges | ratio/dB/energies | 无量纲/dB | PC_VERIFIED |
| SFDR | magnitude + fundamental/exclude ranges | spur/fund ratio/dBc | dBc | PC_VERIFIED |

## 6 模型与同步检测

| 模块 | 输入 | 输出 | 单位 | 当前状态 |
|---|---|---|---|---|
| SineFit3 | float V + known f/Fs | A/phase/DC/residual | V/deg/rad | PC_VERIFIED |
| SineFit4 | float V + initial±width/Fs/iterations | f + SineFit3 result | Hz/V/deg | PC_VERIFIED（窄带干净单音） |
| LockIn | float V + reference f/Fs/phase | mean/I/Q/A/phase | V/deg/rad | PC_VERIFIED（相干整数周期） |

## 7 可以直接连接

| 上游 | 下游 | 必须满足 |
|---|---|---|
| ADC_DMA RAW | ADC_ToVoltage | code 范围、Vref、input_scale 正确 |
| ADC_ToVoltage V | Calibration/Mean/Vpp/RMS/RemoveDC/滤波 | count 一致，V 单位明确 |
| RemoveDC centered V | ZeroCross/Window/FFT/SineFit | 任务不需要保留 DC |
| ZeroCross events | ZeroCrossInterpolation | 同一原数组和 threshold，event 容量足够 |
| Interpolated same-direction crossings | MultiCycleAverage | 不混上升/下降，否则频率翻倍 |
| Windowed float | FFT Real | N 是 2 次幂；窗 coherent gain 被保存 |
| FFT complex | Magnitude/FFT Phase | 相同 FFT 符号和 bin |
| Magnitude | GainCorrection/Peak/Harmonic/SNR/SFDR | 明确当前是 raw 还是 corrected 标度 |
| Peak index + magnitude | Parabolic/LogParabolic | 峰不是边界且有左右邻点 |
| Harmonic | THD | 必须含 H1 和至少 H2，半径一致不重叠 |
| Correlation lag | Phase adapter | 已知 period_samples，正 lag 表示 B 更晚 |

## 8 不能直接连接

| 错误连接 | 为什么错 | 正确做法 |
|---|---|---|
| RAW `uint16_t` -> float 算法 | 类型/单位错误 | 先 ADC_ToVoltage |
| RemoveDC -> DC Mean | 被测 DC 已删 | Mean 接原始 voltage |
| FFT complex -> THD | 缺 magnitude、基波和谐波积分 | Magnitude→Harmonic→THD |
| Hann raw peak -> Vpeak | 窗相干增益未校正 | WindowGainCorrection |
| 上升+下降 crossings -> MultiCycleAverage | 间隔只有半周期 | ZeroCross 只选一个方向 |
| IIR/FIR 后相位直接当原信号相位 | 滤波引入幅相响应 | 双路同处理并做频响/延迟校准 |
| Hampel/Median -> 脉冲测量 | 有效尖峰会被删除 | 保留原始链，改采样/触发 |
| SineFit4 宽频全局搜索 | 算法只保证窄区间局部单峰 | 先 FFT/过零取得粗频率 |

## 9 Expected Hardware 连接片段

```c
/* 这些 Get 函数名只是契约示例；以硬件任务最终公开 API 为准。 */
const uint16_t *raw = SignalADC_GetBuffer();
uint32_t count = SignalADC_GetSampleCount();
float fs_hz = SignalADC_GetSampleRateHz();

if (SignalADCToVoltage_Process(raw, voltage_v, count, &convert_cfg)
        == SIGNAL_ALGORITHM_OK) {
    (void)fs_hz;
    SignalRemoveDC_Process(voltage_v, voltage_v, count, &remove_result);
}
```

算法层不需要 DMA channel、Timer instance、ADC 寄存器或中断细节。
