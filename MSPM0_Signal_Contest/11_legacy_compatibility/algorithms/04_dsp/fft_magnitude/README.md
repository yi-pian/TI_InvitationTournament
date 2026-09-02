# FFTMagnitude：复频谱变成非负频率强度

> 新比赛工程默认：CMSIS DIRECT。交错复数使用 `arm_cmplx_mag_f32/q15/q31`。本目录旧 API 只为兼容；单边谱、FFT 缩放和 coherent gain 仍由 Recipe 处理。

> **CMSIS DIRECT / FROZEN_COMPATIBILITY：** 普通复数幅值不再作为竞赛专用模块；本目录旧 API 只维护既有 Application。单边谱、固定点缩放和 coherent gain 属于 Recipe，不是另写 magnitude 核心的理由。

## 新比赛工程：先看这里

**适合：** FFT 已产生 `signal_complex_f32_t spectrum[N]`，现在要得到可找峰的非负 `magnitude[N/2+1]`。

**不复制本目录源码。** 对 CMSIS 交错复数数组直接调用同数据类型的 magnitude API：

```c
#include "arm_math.h"

static float32_t magnitude[N];
arm_cmplx_mag_f32(spectrum_interleaved, magnitude, N);
```

**输入 / 输出：** 交错 complex FFT N 点（`2N` 个标量）-> 线性 magnitude N 点。实输入通常只消费 `0..N/2`。它尚未完成单边系数、FFT 固定点缩放或 Window coherent-gain 校正，不能直接宣称为 Vpeak。

| 题目变化 | 修改 |
|---|---|
| FFT N 变化 | 同步调整 spectrum 和 magnitude 容量 |
| 只找频率 | 原始 magnitude 可直接接 Peak |
| 要物理幅值 | 后续做 N、单边和 Window gain 校正 |

**Build / 最小验证：** 人工构造 `{real=1, imag=0}` 的 bin，应得 magnitude=1。隔离复制工程已 `SysConfig / Compile / Full Link PASS`，Flash 2368 B、SRAM（含栈）541 B。完整代码见 `README_MINIMAL_EXAMPLE.c`。

**连接：** `FFT -> FFT Magnitude -> Peak / FFT Interpolation / Harmonic`。常见错误是输出容量只有 N/2、把原始 magnitude 当实际电压、或误传时域数组。

> 下文旧 `SignalFFTMagnitude_Process` API 只供维护既有 Application；新工程以上面的 CMSIS 调用为准。

## 你真的需要这个模块吗？

**已有 FFT 复数/packed 输出，并且要得到非负 magnitude 频谱时需要。** 这是 C `ALGORITHM_MODULE`，只处理内存数据。

## 你应该已经有什么输入数据

符合 `signal_fft_magnitude.h` 约定的 FFT 输出、FFT size 和输出 bin 容量。

## 最短接入步骤

1. **文件：** 新工程不复制本目录源码；include `arm_math.h`。只有维护旧 Application 才读取下文兼容 API。
2. **参数：** FFT size、输出 bin count，以及 API 要求的标度信息。
3. **Workspace / Result：** 准备 `float magnitude[N/2+1]` 和 `signal_fft_magnitude_result_t result`。
4. **调用：** `SignalFFTMagnitude_Process(spectrum, N, magnitude, N / 2U + 1U, &result)`。
5. **输出：** 非负频率线性 magnitude 数组。
6. **连接下一步：** Window Gain Correction、Peak Detect、Harmonic、SNR/SFDR。
7. **Build / 最小验证：** 对已知单音 FFT 输出，最大 magnitude bin 应与输入频率一致。

> 算法边界：不配置 Pin，不修改 SysConfig，不调用 DriverLib，也不需要 Platform Adapter。上游硬件变化时，只把真实 `Fs/N/VREF` 等事实同步到算法参数。

## 1 这个算法是干什么的？

FFT 每个 bin 有 real/imag；Magnitude 计算它们到原点的长度，方便找峰。

## 2 一个最简单的例子

`3+j4` 的 magnitude 是 5。

## 3 原理

`M[k]=sqrt(real²+imag²)`。实信号负频率镜像，所以只输出 `N/2+1` 个 bin。

## 4 比赛里什么时候用？

找基波、谐波、杂散和幅度谱。

## 5 输入

N 点 complex FFT、fft_size。

## 6 输出

N/2+1 个 raw DFT magnitude，没有 /N、单边 x2、窗 CG 修正。

## 7 API怎么调用

```c
SignalFFTMagnitude_Process(spectrum,N,mag,N/2+1,&r);
```

## 8 参数怎么改

只有 N/capacity，必须与 FFT 一致。

## 9 参数改大会怎样

N 大输出约 2N 字节，计算增大。

## 10 这个算法的代价是什么

Benefits：易找峰。Trade-offs：丢失相位、需额外 buffer、每 bin 开方。

## 11 什么时候不要用

需要 FFT phase/correlation 时保留 complex；RAM 极紧可边算边消费，需另写安全接口。

## 12 怎么和前一个模块接

`FFT complex -> Magnitude`

## 13 怎么和后一个模块接

`Magnitude -> Peak / GainCorrection / Harmonic`

## 14 最小Demo

```c
signal_fft_magnitude_result_t r;
(void)SignalFFTMagnitude_Process(X,N,m,N/2+1,&r);
```

## 15 PC测试

冲激 FFT 所有非负 bin Expected=1，全部 PASS。

## 16 MCU资源

输出 `4*(N/2+1)`，O(N/2) hypotf，内部 O(1)。

## 17 验证状态

PC_VERIFIED；未实板。

## 18. 完整 API、调用顺序与 Buffer 规则

唯一公开函数 `SignalFFTMagnitude_Process(spectrum, fft_size, magnitude, magnitude_capacity, result)`：输入只读 `signal_complex_f32_t[fft_size]`；N>=2 且为偶数；输出至少 `N/2+1` 个 float；result 非空。成功返回 OK 并写 `bin_count=N/2+1`；容量不足或参数/数值非法返回对应状态。

```text
FFT complex[N] -> Magnitude -> raw magnitude[N/2+1]
                            -> Window Gain Correction -> Peak/Harmonic
```

输入输出类型不同且不能原地。模块无 workspace/Init；输出 RAM=`4*(N/2+1)`。

```c
signal_fft_magnitude_result_t mag;
status = SignalFFTMagnitude_Process(
    spectrum, N, magnitude, N / 2U + 1U, &mag);
```

## 19. Result Meaning / Algorithm Scope

`magnitude[k]=sqrt(real²+imag²)`，k 对应 `k*Fs/N`。它仍是未除 N 的 raw DFT magnitude，没有自动做单边 x2、DC/Nyquist 特例或窗相干增益修正，不能直接称为 Vpeak。

常见错误：输出容量只分 N/2 而漏 Nyquist、把 byte 当元素数、fft_size 与 spectrum 不一致、把 raw magnitude 当 V、再次对 magnitude 开方、读取 `result.bin_count` 之外数据。

## 20. Quick Modify Table

| 我想改什么 | 去哪里 | 改什么 | 影响 | 是否需同步上游 |
|---|---|---|---|---|
| N | FFT/Application buffers | `fft_size`/capacity | RAM/bin数 | 否 |
| 物理幅值 | 后续 Gain Correction | N/coherent gain | raw→单边幅值 | 否 |
| 频率 | 后续换算 | `k*Fs/N` | bin→Hz | 改真实Fs时是 |
| 峰搜索范围 | Peak Detect | start/end | 排除DC/限定频带 | 否 |

## API Reference

`SignalFFTMagnitude_Process(spectrum, fft_size, magnitude, magnitude_capacity, result)`：输出 `N/2+1` 个原始 DFT magnitude。
