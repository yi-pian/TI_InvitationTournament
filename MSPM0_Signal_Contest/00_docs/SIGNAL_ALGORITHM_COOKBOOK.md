# Signal Algorithm Cookbook

> 2026-08-13 更新：比赛母版已默认配置 CMSIS-DSP 1.16.2。普通 Mean/MinMax/Vpp/RMS/AC RMS/Remove DC/FFT/Magnitude/FIR/IIR/Correlation 的首选代码改见 [CMSIS-DSP 电赛 Cookbook](CMSIS_DSP_CONTEST_COOKBOOK.md)；本页旧直接循环仍可用于理解或 PC 对照，不是比赛默认核心。

这是比赛现场的算法入口。先找“我要做什么”，再按推荐方式执行。简单计算直接复制 Recipe；只有真正复杂的算法才复制正式模块。题目要求的是压摆率、带宽、SINAD、频响、自动量程或校准等完整测量结果时，先查 [Measurement Recipe Index](MEASUREMENT_RECIPE_INDEX.md)，再回到本页选其中的 Direct Recipe 或正式 Primitive。

| 我要做什么 | 推荐方式 | 去哪里 |
|---|---|---|
| 算平均值 / DC | **CMSIS Recipe** | [Mean](recipes/mean.md) |
| 找最大值、最小值 | **CMSIS Direct** | [Min/Max](recipes/minmax.md) |
| 算 Vpp | **CMSIS Recipe** | [Vpp](recipes/vpp.md) |
| 算普通 RMS（包含 DC） | **CMSIS Direct** | [RMS](recipes/rms.md) |
| 算 AC RMS（去掉 DC） | **CMSIS Recipe** | [AC RMS](recipes/ac_rms.md) |
| ADC code 转电压 | **直接代码** | [ADC To Voltage](recipes/adc_to_voltage.md) |
| 去直流 | **CMSIS Recipe** | [Remove DC](recipes/remove_dc.md) |
| 乘比例 | **CMSIS Direct** | [Scaling](recipes/scaling.md) |
| 加/减固定偏移 | **CMSIS Direct** | [Offset Correction](recipes/offset_correction.md) |
| 归一化到指定峰值 | **CMSIS Recipe** | [Normalize](recipes/normalize.md) |
| 简单阈值判断 | **直接代码** | [Threshold](recipes/threshold.md) |
| 判断削顶点数量 | **直接代码** | [Clipping Detect](recipes/clipping_detect.md) |
| 在范围内找主峰 | **直接代码** | [Peak Detect](recipes/peak_detect.md) |
| 多个同向过零求平均频率 | **直接代码** | [Multi-Cycle Average](recipes/multi_cycle_average.md) |
| 滑动平均 | **Simple Helper** | [`04_dsp/moving_average/README.md`](../04_dsp/moving_average/README.md) |
| FFT 单边幅值和窗增益换算 | **Simple Helper** | [`05_precision/window_gain_correction/README.md`](../05_precision/window_gain_correction/README.md) |
| 过零事件/滞回 | **正式模块** | [`frequency_zero_cross/README.md`](../03_measurement/frequency_zero_cross/README.md) |
| FFT / 看频谱 | **CMSIS Direct + Recipe** | [CMSIS FFT](recipes/cmsis_fft_spectrum.md) |
| FFT magnitude | **CMSIS Direct** | [CMSIS Complex Magnitude](CMSIS_DSP_CONTEST_COOKBOOK.md) |
| FFT 峰值插值 | **正式模块** | [`fft_parabolic_interpolation/README.md`](../05_precision/fft_parabolic_interpolation/README.md) |
| FIR / IIR | **CMSIS DIRECT + Recipe** | [CMSIS FIR/Biquad](CMSIS_DSP_CONTEST_COOKBOOK.md) |
| 去孤立毛刺 | **正式模块** | [`hampel_filter/README.md`](../04_dsp/hampel_filter/README.md) |
| 相关 / 自相关 | **正式模块** | [`correlation/README.md`](../04_dsp/correlation/README.md) / [`autocorrelation/README.md`](../04_dsp/autocorrelation/README.md) |
| 测谐波 / THD | **正式模块** | [`harmonic/README.md`](../04_dsp/harmonic/README.md) / [`thd/README.md`](../04_dsp/thd/README.md) |
| 测相位 | **正式模块** | [`phase/README.md`](../03_measurement/phase/README.md) |
| 拟合单音 / 弱信号锁相 | **正式模块** | [`sine_fit_3param/README.md`](../05_precision/sine_fit_3param/README.md) / [`lock_in/README.md`](../05_precision/lock_in/README.md) |
| 组合完整测量逻辑链 | **Measurement Recipe** | [测量逻辑链总索引](MEASUREMENT_RECIPE_INDEX.md) |

## 最短使用流程

1. 找到上表中的功能。
2. 标为“CMSIS Direct/Recipe”或“直接代码”：打开 Recipe，只复制“比赛现场直接复制这一段”。
3. 标为“Simple Helper/正式模块”：打开对应 README，按文件清单复制 `.c/.h`。
4. 每加入一项就 Build 一次。
5. 最后核对 `Fs`、`N`、单位、VREF、阈值、窗口和 Backend。

算法 Recipe 和模块都不配置 ADC/Timer/DMA/Pin；真实采样率和电压参数来自 Application/SysConfig，但必须作为数值正确传进算法。

## 三个最常用的直接链

```text
ADC raw[] -> ADC To Voltage Recipe -> Vpp / RMS / Mean Recipe
```

```text
voltage[] -> CMSIS Remove DC Recipe -> Window Recipe/Table -> CMSIS FFT
```

```text
同向过零位置[] -> Multi-Cycle Average Recipe -> frequency_hz
```

## 何时不要用直接 Recipe

- 输入可能包含 NaN/Inf，而你的应用没有先验证；
- 数据长度、容量和边界来自不可信外部输入；
- 需要跨帧状态、动态窗口或大 workspace；
- 算法有多种 Backend/定点缩放；
- 对异常点、泄漏、相位、数值退化的处理会直接影响题目精度。

此时回到 [ALGORITHM_SIMPLIFICATION_REPORT.md](ALGORITHM_SIMPLIFICATION_REPORT.md) 选择 Level B/C，而不是继续往 `main.c` 塞复杂实现。
