# CMSIS-DSP 电赛 Cookbook

本页只使用本机 MSPM0 SDK 2.11.00.07 内置 CMSIS-DSP 1.16.2 的真实 API。比赛母版已配置 `arm_math.h`；以下代码不需要再复制普通 RMS、MinMax、FFT、FIR 或 IIR 核心源码。

所有函数都要求指针有效、`N > 0`；CMSIS API通常不返回参数错误，调用方自己保证长度和容量。

## 1. Max / Min / Vpp

作用：找最大、最小和峰峰值。输入输出单位与 `x[]` 相同。

```c
#include "arm_math.h"

float32_t maximum, minimum, vpp;
uint32_t max_index, min_index;
arm_max_f32(x, N, &maximum, &max_index);
arm_min_f32(x, N, &minimum, &min_index);
vpp = maximum - minimum;
```

比赛例子：`x` 为 ADC 转换后的 V 数组，则 `vpp` 单位是 V。孤立毛刺会直接污染结果；强毛刺改用 `robust_peak_to_peak`。

## 2. Mean / DC

```c
float32_t dc_v;
arm_mean_f32(voltage_v, N, &dc_v);
```

`dc_v` 是整帧平均电压。帧未覆盖整数周期时，交流分量也会泄漏进平均值。

## 3. RMS / AC RMS

总 RMS：

```c
float32_t rms_v;
arm_rms_f32(voltage_v, N, &rms_v);
```

AC RMS Recipe：

```c
float32_t dc_v, ac_rms_v;
arm_mean_f32(voltage_v, N, &dc_v);
arm_offset_f32(voltage_v, -dc_v, ac_workspace_v, N);
arm_rms_f32(ac_workspace_v, N, &ac_rms_v);
```

总 RMS 包含 DC；AC RMS 先去平均值。`ac_workspace_v` 可与输入分开，也可在确认后原地覆盖。

## 4. Power

`arm_power_f32` 输出平方和，不是平均功率：

```c
float32_t sum_of_squares;
float32_t mean_square;
arm_power_f32(x, N, &sum_of_squares);
mean_square = sum_of_squares / (float32_t) N;
```

若要 RMS，直接使用 `arm_rms_f32`。

## 5. Vector Offset / Remove DC

```c
float32_t mean;
arm_mean_f32(x, N, &mean);
arm_offset_f32(x, -mean, x_ac, N);
```

作用：每个样本都减去同一个 DC。它不是高通滤波器；缓慢漂移需要分帧更新或正式滤波策略。

## 6. Vector Scale

```c
arm_scale_f32(input, gain, output, N);
```

用于校准增益、单位换算或窗后幅值修正。带偏置校准用 `scale` 后再 `offset`：`corrected = raw * gain + offset`。

## 7. Dot Product

```c
float32_t dot;
arm_dot_prod_f32(a, b, N, &dot);
```

用于同步检波、投影和最小二乘的基础项。若要归一化相关系数，还要分别计算两路能量并处理零能量边界。

## 8. Complex Magnitude

输入必须是交错复数：`re0, im0, re1, im1, ...`。

```c
arm_cmplx_mag_f32(fft_interleaved, magnitude, N);
```

`magnitude[k] = sqrt(re[k]^2 + im[k]^2)`。实信号频谱通常只使用 `0 ... N/2`；幅值单位还要考虑 FFT 归一化、单边谱倍增和窗口 coherent gain。

## 9. Q15 CFFT（比赛默认候选）

```c
#include "arm_const_structs.h"
#include "arm_math.h"

q15_t fft_q15[2U * 256U] = {0};
q15_t magnitude_q15[256U];

/* fft_q15[2*n] = real[n], fft_q15[2*n+1] = imag[n] */
arm_cfft_q15(&arm_cfft_sR_q15_len256, fft_q15, 0U, 1U);
arm_cmplx_mag_q15(fft_q15, magnitude_q15, 256U);
```

当前 SDK 提供 16、32、64、128、256、512、1024、2048、4096 点常量实例。`ifftFlag=0` 是正变换，`bitReverseFlag=1` 让输出按正常 bin 顺序排列。Q15 内部缩放必须通过已知正弦标定恢复幅值。

## 10. F32 CFFT

```c
float32_t fft_f32[2U * 256U];
arm_cfft_f32(&arm_cfft_sR_f32_len256, fft_f32, 0U, 1U);
arm_cmplx_mag_f32(fft_f32, magnitude_f32, 256U);
```

F32 易理解，但 M0+ 无 FPU；长 FFT 必须用板测周期决定是否采用。真实输入也可使用 `arm_rfft_fast_f32`，但它的 packed 输出格式不同，不能直接当作 CFFT 交错复数；换接口前先写独立格式测试。

## 11. FIR

```c
#define BLOCK_SIZE 64U
#define NUM_TAPS 5U

static const float32_t coeffs[NUM_TAPS] = {0.1F, 0.2F, 0.4F, 0.2F, 0.1F};
static float32_t state[NUM_TAPS + BLOCK_SIZE - 1U];
static arm_fir_instance_f32 fir;

arm_fir_init_f32(&fir, NUM_TAPS, coeffs, state, BLOCK_SIZE);
arm_fir_f32(&fir, input, output, BLOCK_SIZE);
```

`state` 长度必须是 `NUM_TAPS + BLOCK_SIZE - 1`。系数顺序必须按当前 CMSIS FIR 约定准备；系数设计不由 CMSIS 代替。

## 12. Biquad / IIR

```c
#define STAGES 1U
static const float32_t sos[5U * STAGES] = {
    b0, b1, b2, a1_cmsis, a2_cmsis
};
static float32_t state[4U * STAGES];
static arm_biquad_casd_df1_inst_f32 biquad;

arm_biquad_cascade_df1_init_f32(&biquad, STAGES, sos, state);
arm_biquad_cascade_df1_f32(&biquad, input, output, N);
```

每节 5 个系数、4 个状态。不同设计工具对反馈系数符号的定义可能相反；必须用冲激/阶跃或 Python golden 检查后再上板。

## 13. Correlation

```c
float32_t corr[2U * N - 1U];
arm_correlate_f32(a, N, b, N, corr);
```

输出长度为 `2*max(lenA,lenB)-1`。这是普通未归一化相关；若要时间延迟/相位，还需要 Contest 层完成去 DC、归一化、搜索 lag 范围、峰值插值、符号约定和 `delay_s = lag/Fs`。

## 14. Variance / Standard Deviation

```c
float32_t variance;
float32_t standard_deviation;
arm_var_f32(x, N, &variance);
arm_std_f32(x, N, &standard_deviation);
```

二者输出单位分别是输入单位的平方和输入单位本身。不要把方差当噪声功率，除非你已经定义了 DC、信号和噪声的分离方法；SNR/SINAD 仍需完整测量 Recipe。

## 15. Convolution

```c
float32_t convolution[NA + NB - 1U];
arm_conv_f32(a, NA, b, NB, convolution);
```

输出长度必须是 `NA + NB - 1`。它适合离线/块卷积验证或短序列处理；实时连续 FIR 默认用 `arm_fir_*` 管理跨块 state，不要每帧用 full convolution 重算历史。

## 16. Matrix Operations

```c
arm_matrix_instance_f32 A, B, C;
arm_mat_init_f32(&A, rows_a, cols_a, a_data);
arm_mat_init_f32(&B, rows_b, cols_b, b_data);
arm_mat_init_f32(&C, rows_a, cols_b, c_data);
arm_status status = arm_mat_mult_f32(&A, &B, &C);
```

当前 SDK 还提供矩阵加、减、转置等 API。矩阵维度和 `c_data` 容量必须由 Application 保证，并检查 `arm_status`。M0+ 上大型 F32 矩阵可能很慢且占 RAM；它主要作为 Sine Fit/Calibration 等 Contest 算法的基础算子，不代表应把所有标量公式改成矩阵。

## 17. Q15 / Q31 对应关系

本页优先用 F32 展示语义；大量数组或 FFT 再按数据类型指南切换。当前 1.16.2 头文件中，统计、scale/offset、dot product、complex magnitude、FFT、FIR、correlation 等均有相应 Q15/Q31 API（个别函数的累加结果类型和缩放不同）。切换时必须重新核对当前 `arm_math.h` 原型、Q 格式、饱和和输出标度，不能只把函数名后缀机械替换。

## 组合链

```text
普通 Vpp     : CMSIS Max + CMSIS Min + subtract
普通 AC RMS  : CMSIS Mean + CMSIS Offset + CMSIS RMS
普通频谱     : Remove DC Recipe + Window + CMSIS FFT + CMSIS Magnitude
高精度频率   : 上述频谱 + Contest Peak + Quinn/Jacobsen/Macleod
THD          : CMSIS FFT/Magnitude + Contest Multi-bin/Harmonic/THD
时间延迟     : CMSIS Correlation + Contest peak interpolation/calibration
```

## 工程前提

母版 projectspec 必须有 `-DARM_MATH_CM0`、CMSIS Core/DSP include；母版 `.syscfg` 必须有 `ProjectConfig.genLibCMSIS = true`。正常情况下不要手工添加静态库路径，SysConfig 生成的 `device.cmd.genlibs` 负责选择 SDK 自带 M0+ TI Clang 库。
