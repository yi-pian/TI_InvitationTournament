# FFTParabolicInterpolation：峰值在两个 bin 之间怎么办

> **LEVEL C / REAL ALGORITHM MODULE：** 左中右邻点、局部最大、退化分母、offset 范围和 Fs/N 换算都有边界条件，继续作为正式精度模块。

## 比赛复制版：先看这里

**适合：** Peak 已给出内部峰 bin，想用它和左右邻点把频率细化到 fractional bin。它需要线性 magnitude，且峰不能在数组两端。

**复制到 `modules/`：** `signal_fft_parabolic_interpolation.c`、`signal_fft_parabolic_interpolation.h` 和 `03_measurement/common/signal_algorithm_status.h`。不需要 SysConfig/Pin。

```c
#include "signal_fft_parabolic_interpolation.h"

signal_fft_parabolic_result_t fine;
signal_algorithm_status_t status = SignalFFTParabolicInterpolation_Process(
    magnitude, N / 2U + 1U, peak.peak_index, Fs, N, &fine);
if (status == SIGNAL_ALGORITHM_OK) {
    float frequency_hz = fine.frequency_hz;
    // ===== 这里写你自己的逻辑 =====
}
```

**输入 / 输出：** magnitude、bin 数、整数峰 index、Fs、FFT N -> `bin_offset`、`fractional_bin`、`frequency_hz`、插值峰值。

| 题目变化 | 修改 |
|---|---|
| Fs/N 变化 | 同步传真实 Fs 和同一 FFT N |
| 峰在边界 | 调整 Peak 搜索范围，不能对 0 或最后 bin 插值 |
| 使用 log-magnitude 插值 | 本模块不是 log-parabolic，改用对应正式模块 |

**Build / 最小验证：** 构造中心 bin 明显大于左右邻点，检查 offset 在 [-0.5,0.5] 附近且 Hz=`fractional_bin*Fs/N`。隔离复制工程已 `SysConfig / Compile / Full Link PASS`，Flash 1944 B、SRAM（含栈）529 B。完整代码见 `README_MINIMAL_EXAMPLE.c`。

**连接：** `FFT Magnitude -> Peak -> FFT Parabolic Interpolation -> frequency_hz`。常见错误是传 dB 数据、峰不是局部最大、Fs/N 与上游不一致。

> 下文保留精度边界与详细 API；比赛 COPY 以本节为准。

## 你真的需要这个模块吗？

**已有整数 peak bin 及左右相邻 magnitude，并且要获得 fractional bin/更细频率时需要。** 这是 C `ALGORITHM_MODULE`，只处理内存数据。

## 你应该已经有什么输入数据

至少包含 `peak_bin-1`、`peak_bin`、`peak_bin+1` 的 magnitude，真实 `Fs` 和 FFT size。

## 最短接入步骤

1. **文件：** 复制顶部清单到 `modules/`，include `signal_fft_parabolic_interpolation.h`；无需另加算法仓库 Include Path。
2. **参数：** peak bin、`Fs`、FFT size 和数组容量。
3. **Workspace / Result：** 准备 `signal_fft_parabolic_result_t result`；不需要大 workspace。
4. **调用：** `SignalFFTParabolicInterpolation_Process(magnitude, bin_count, peak_index, Fs, N, &result)`。
5. **输出：** fractional bin 与插值频率。
6. **连接下一步：** Frequency result、Harmonic 的基波位置或频谱显示。
7. **Build / 最小验证：** 构造左右近似对称峰时 fractional offset 应接近 0；边界 bin 不可插值。

> 算法边界：不配置 Pin，不修改 SysConfig，不调用 DriverLib，也不需要 Platform Adapter。上游硬件变化时，只把真实 `Fs/N/VREF` 等事实同步到算法参数。

## 1 这个算法是干什么的？

FFT 最大 bin 只给整数格点。三点插值用峰左、峰、峰右拟合一条抛物线，估计真正峰顶在中心 bin 左右多少。

## 2 一个最简单的例子

构造峰顶在 2.25 的抛物线，三个点为 8.4375、9.9375、9.4375；算法返回 offset=+0.25。

## 3 原理

`delta=0.5*(L-R)/(L-2C+R)`，frequency=`(k+delta)*Fs/N`。它能得到 bin 间位置，是因为主瓣顶端在小范围内近似光滑曲线。

## 4 比赛里什么时候用？

FFT 测单音频率、峰不在 bin 中心时。Hann 常用，但线性 magnitude 抛物线有偏差；高精度可再比较 log-parabolic。

## 5 输入

线性 magnitude、bin_count、非边界局部峰、Fs Hz、FFT size。

## 6 输出

offset（-0.5~0.5 bin）、fractional_bin、frequency_hz、插值 magnitude（仍是输入标度）。

## 7 API怎么调用

```c
SignalFFTParabolicInterpolation_Process(m,bins,peak,Fs,N,&r);
```

## 8 参数怎么改

没有调参；关键是窗、N、Fs 和正确 peak。不要人为夹出想要频率。

## 9 参数改大会怎样

N 大使每 bin Hz 更小，但 RAM/时间/记录更长；插值偏差也与窗和 fractional offset 相关。

## 10 这个算法的代价是什么

Benefits：O(1)、比整数 bin 精细。Trade-offs：模型近似；多音重叠/低 SNR/平顶会偏。

## 11 什么时候不要用

peak=0/Nyquist、左右点不存在、中心不是局部最大、两个强音主瓣重叠。

## 12 怎么和前一个模块接

`Magnitude -> PeakDetect -> Parabolic`

## 13 怎么和后一个模块接

`frequency_hz -> display/control/harmonic frequency seed`

## 14 最小Demo

```c
signal_fft_parabolic_result_t r;
(void)SignalFFTParabolicInterpolation_Process(m,bins,k,Fs,N,&r);
```

## 15 PC测试

解析抛物线 offset 0.25 精确通过；1024/Hann/Fs102400 的 1037 Hz 测得 1032.1666 Hz，误差 4.833 Hz，真实记录并未伪装为零误差。

排查：偏差数 Hz 可能是线性插值固有偏差；错误数百 Hz 先查 Fs/N/peak；NO_FEATURE 查平顶或非局部最大。

## 16 MCU资源

O(1)，少量加减乘除，无数组工作区。

## 17 验证状态

PC_VERIFIED（已记录真实偏差）；未实板。

## 17. 统一 API 教程（已按当前头文件核对）

本节由当前公开头文件、实现中实际出现的状态码和正式模块注册表生成。它补充前文的场景教程；函数签名变化时必须重新运行 `tools/upgrade_formal_beginner_docs.ps1`，不要手工保留旧 API。

遵循仓库的 [Beginner README 标准](../../00_docs/BEGINNER_README_STANDARD.md)：先用最小示例完成一次正常数据流，再按需要阅读全功能示例和本节 API 细节。

### 什么时候用 / 什么时候不要用

当题目需要“fft_parabolic_interpolation”目录对应的公开功能，并且输入数据、单位和硬件资源满足前文约束时使用本模块。若只需要更简单的上游功能、输入尚未准备好，或需要不同的数据模型/外设资源，应先选择相邻模块而不是强行调用本 API。

### 输入 / 输出

输入由各 API 的只读数组、配置、频率/阈值和平台对象组成；输出写入 result/output/buffer 参数或由 Get API 返回。调用者负责数组容量、生命周期和物理单位；失败返回时输出不是有效结果。

### 调用顺序

```text
SignalFFTParabolicInterpolation_Process
```

`Init` 一般仅一次；`Set/Configure` 仅在参数变化且模块空闲时执行；`Start/Process/Generate` 是每帧或每次任务入口；`Get/Is` 用于读取已完成的结果；`Stop` 只在需要取消时调用。若本模块没有其中某类 API，以实际列出的函数为准。

### SysConfig 边界

本模块是纯软件/算法模块，**不需要 SysConfig**。ADC、DAC、Timer、DMA、引脚和时钟由上游模块配置；调用时只把真实的采样率、数组长度、单位等事实传入。

### 参数分级

- 【比赛必须会】输入/输出数组、`count/length/capacity`、采样率/频率、阈值/增益以及本 README 前文标出的 pin。它们直接影响题目范围、RAM、时间轴或物理单位。
- 【出问题再理解】Timer 时钟、DMA 通道、Event 路由、参考源和 IRQ。它们属于硬件链路，必须与 SysConfig 生成结果一致。
- 【以后进阶】多缓冲、运行时重配置、回调调度和 ISR 优化。先用最小示例完成一帧闭环，再处理吞吐或延迟。

### 常见错误 FAQ

- 参数错误：先检查指针非空、count/capacity 的单位是元素数、频率/阈值单位与上游一致。
- 硬件无结果：不要修改生成文件；回到 SysConfig 核对 pin、instance、时钟、Timer、DMA 和 Event 的完整链路。
- 结果异常：确认上一轮异步采集已经完成，真实 Fs/N/参考电压已传到算法，且没有在 DMA 使用期间改写 buffer。

### `signal_algorithm_status_t SignalFFTParabolicInterpolation_Process(const float *magnitude, uint32_t bin_count, uint32_t peak_index, float sample_rate_hz, uint32_t fft_size, signal_fft_parabolic_result_t *result);`

**它做什么：** 用峰 bin 及左右相邻 magnitude 做三点抛物线插值。

**什么时候调用：** 对调用者提供的数据执行一次同步计算或生成，并在成功后写入输出对象/数组。

| 参数 | 类型 | 初学者解释 |
|---|---|---|
| `magnitude` | `const float *` | 原始或统一标度的线性 magnitude 数组。 |
| `bin_count` | `uint32_t` | 数组长度。 |
| `peak_index` | `uint32_t` | 整数峰索引，必须有左右邻点且自身为局部最大。 |
| `sample_rate_hz` | `float` | 采样率，Hz。 |
| `fft_size` | `uint32_t` | FFT 点数。 |
| `result` | `signal_fft_parabolic_result_t *` | 输出偏移、fractional bin、频率 Hz 和插值峰值。 |

**返回：** 成功返回 SIGNAL_ALGORITHM_OK；平顶/非局部峰或参数错误返回对应码。

**最小调用形状：** `SignalFFTParabolicInterpolation_Process(...);`。可直接从 README_MINIMAL_EXAMPLE.c 复制正常流程；README_FULL_EXAMPLE.c 展示全部公开 API，其中取消类 API 会以 #if 0 隔离。

**注意：** 所有指针和数组都由调用者拥有；先检查返回值。异步采集、DMA 或回调还在使用 buffer 时，不能读取结果或改写该 buffer。

### 示例、模块链与验收

- 最小入门：`README_MINIMAL_EXAMPLE.c`，只保留正常入口和结果读取。
- 全功能：`README_FULL_EXAMPLE.c`，以正确顺序展示当前头文件全部公开 API；`Stop` 等非常规路径不会默认执行。
- 模块链：先由上游提供单位、采样率和有效数据，再调用本模块；成功后将输出交给显示、控制、测量或下一步 DSP。硬件资源仍以 SysConfig 合约为唯一来源。
- 文档验收：README/API、两份示例和头文件会由 `tools/validate_beginner_documentation.ps1` 覆盖检查；这只表示文档与源码签名一致，不代替未进行的实板验证。

### 模块链

`上游采集/配置 -> 本模块 -> 检查返回值和结果 -> 测量、显示、控制或下一步 DSP`。只有确认本模块的输出单位和有效状态后，才交给下一模块。

