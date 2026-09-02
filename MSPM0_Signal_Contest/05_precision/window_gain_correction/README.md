# WindowGainCorrection：为什么 Hann 后幅值要修正

> **LEVEL B / SIMPLE HELPER：** 这是无状态单函数 Helper。它被保留是因为实信号单边谱中 DC/Nyquist 与普通 bin 的倍数不同，重复手写容易产生固定 2 倍误差；没有 Init、context、result struct 或 SysConfig。

## 第一次使用 Window Gain Correction？从这里开始

目标：“把 FFT Magnitude 的 raw DFT 标度换成实信号单边峰值幅度，并补偿实际窗 coherent gain”。

### STEP 1：加入工程

链接 `MSPM0_Signal_Contest/05_precision/window_gain_correction/signal_window_gain_correction.c`；Include Path 加本目录和 `03_measurement/common`。

### STEP 2：include

```c
#include "signal_window_gain_correction.h"
```

### STEP 3：变量

```c
float magnitude[N / 2U + 1U];
signal_window_result_t window_result;
```

允许原地：输入输出都传 `magnitude`，不用第二个数组。

### STEP 4：参数

`bin_count` 必须等于 `N/2+1`；`fft_size=N`；`coherent_gain` 必须直接来自本帧 `SignalWindow_Apply` 的结果。gain 太小会把幅值放得过大；拿错窗的 gain 会产生系统比例误差。

### STEP 5：SysConfig

**【不需要 SysConfig】**。

### STEP 6：初始化

没有 Init；Magnitude 和 Window result 都 ready 后调用。

### STEP 7：调用

```c
signal_algorithm_status_t status = SignalWindowGainCorrection_Apply(
    magnitude, magnitude, N / 2U + 1U, N,
    window_result.coherent_gain);
```

### STEP 8：结果

输出是单边 peak amplitude：DC/Nyquist 不乘 2，其他 bin 乘 2，并除以 `N*coherent_gain`。若时域输入单位 V，接近 bin 中心的孤立正弦输出可解释为 Vpeak。

### STEP 9：连接

```c
(void)SignalWindowGainCorrection_Apply(magnitude, magnitude,
    mag_result.bin_count, N, window_result.coherent_gain);
(void)SignalPeakDetect_Process(magnitude, mag_result.bin_count,
    first_bin, last_bin, &peak);
```

第二条链：校正后 magnitude 可用于显示主要峰；THD 的公共比例会相消，但仍应保持同一窗/同一标度。

### STEP 10：Build

undefined symbol=未链接 `.c`；bin_count 错=必须 N/2+1；幅值约小一半=可能漏单边×2/漏本模块；幅值偏大=coherent gain/N 不匹配。

### STEP 11：验证

输入整数 bin、1 Vpeak 正弦，经过 Window→FFT→Magnitude→Correction 后，对应峰应接近 1 Vpeak。

### STEP 12：常见修改

1. Hann→Blackman：只需传新的 `window_result.coherent_gain`，不要写死 0.5。
2. 输出 Vpp：对正弦 Vpp≈2×Vpeak；不要对任意波形盲目使用。
3. 输出 Vrms：对正弦 Vrms≈Vpeak/√2；先确认题目幅值定义。
4. N 变化：bin_count 同步 N/2+1。

### STEP 13：完整最小示例

```c
#include "signal_window_gain_correction.h"
void Correct(float *mag, uint32_t n, float coherent_gain)
{
    (void)SignalWindowGainCorrection_Apply(
        mag, mag, n / 2U + 1U, n, coherent_gain);
}
```

下面是单边标度、边界 bin、验证证据和完整 API Reference。

## 1 这个算法是干什么的？

FFT raw magnitude 会随 N 变大，窗又把正弦平均权重压低。该模块把它换成单边峰值幅度。

## 2 一个最简单的例子

N=1024、Rectangular CG=1，某内部 bin raw magnitude=256，则 peak=`2*256/1024=0.5`。

## 3 原理

内部正频率 bin：`Apeak=2*M/(N*CG)`；DC 和 Nyquist 没有负频率镜像，所以不用乘 2。Hann 后幅值需要校正，因为 `mean(w)<1`，同一正弦被窗权重缩小。

## 4 比赛里什么时候用？

FFT 输出要报告正弦 Vpeak/幅度谱时。

## 5 输入

raw magnitude N/2+1、FFT N、实际 CG。

## 6 输出

单边 peak amplitude，若时域单位 V 则输出 Vpeak。

## 7 API怎么调用

```c
SignalWindowGainCorrection_Apply(raw,amp,bins,N,w.coherent_gain);
```

## 8 参数怎么改

CG 必须来自实际使用的同 N 窗；Rect=1。不要把 power_gain 填进来。

## 9 参数改大会怎样

CG 填大输出变小，填小输出变大；N 错会等比例错并触发 bin_count 检查。

## 10 这个算法的代价是什么

Benefits：标度和单边规则统一。Trade-offs：不能补偿非整 bin 的 scalloping；噪声功率需用 power/ENBW 方法。

## 11 什么时候不要用

宽带噪声 PSD、multi-bin energy 或严重泄漏却只读单 bin 时，不应把结果当完整能量。

## 12 怎么和前一个模块接

`Window result CG + FFTMagnitude -> GainCorrection`

## 13 怎么和后一个模块接

`corrected amplitude -> Peak/display`；THD multi-bin 可直接用统一 raw 能量比，标度会相消。

## 14 最小Demo

```c
(void)SignalWindowGainCorrection_Apply(m,m,N/2+1,N,cg);
```

## 15 PC测试

解析 256→0.5；Hann 1024 精确 bin 0.5 Vpeak 恢复为 0.4999988 V，PASS。

排查：约一半/两倍检查单边规则；约 CG 倍检查是否重复或漏校正；非整 bin 偏小检查多 bin/插值而非乱改 CG。

## 16 MCU资源

O(N/2)，内部 O(1)，支持原地；每 bin 一次乘法。

## 17 验证状态

PC_VERIFIED；未实板。

## 17. 统一 API 教程（已按当前头文件核对）

本节由当前公开头文件、实现中实际出现的状态码和正式模块注册表生成。它补充前文的场景教程；函数签名变化时必须重新运行 `tools/upgrade_formal_beginner_docs.ps1`，不要手工保留旧 API。

遵循仓库的 [Beginner README 标准](../../00_docs/BEGINNER_README_STANDARD.md)：先用最小示例完成一次正常数据流，再按需要阅读全功能示例和本节 API 细节。

### 什么时候用 / 什么时候不要用

当题目需要“window_gain_correction”目录对应的公开功能，并且输入数据、单位和硬件资源满足前文约束时使用本模块。若只需要更简单的上游功能、输入尚未准备好，或需要不同的数据模型/外设资源，应先选择相邻模块而不是强行调用本 API。

### 输入 / 输出

输入由各 API 的只读数组、配置、频率/阈值和平台对象组成；输出写入 result/output/buffer 参数或由 Get API 返回。调用者负责数组容量、生命周期和物理单位；失败返回时输出不是有效结果。

### 调用顺序

```text
SignalWindowGainCorrection_Apply
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

### `signal_algorithm_status_t SignalWindowGainCorrection_Apply(const float *raw_magnitude, float *amplitude_peak, uint32_t bin_count, uint32_t fft_size, float coherent_gain);`

**它做什么：** 把实信号非负频率的原始 DFT magnitude 换算为单边峰值幅度。

**什么时候调用：** 对调用者提供的数据执行一次同步计算或生成，并在成功后写入输出对象/数组。

| 参数 | 类型 | 初学者解释 |
|---|---|---|
| `raw_magnitude` | `const float *` | 输入 N/2+1 个原始 magnitude。 |
| `amplitude_peak` | `float *` | 输出峰值幅度；允许与输入为同一数组。 |
| `bin_count` | `uint32_t` | 必须等于 fft_size/2+1。 |
| `fft_size` | `uint32_t` | FFT 点数，必须为偶数且至少 2。 |
| `coherent_gain` | `float` | 实际窗相干增益 mean(w)，必须为正。 |

**返回：** 成功返回 SIGNAL_ALGORITHM_OK。

**最小调用形状：** `SignalWindowGainCorrection_Apply(...);`。可直接从 README_MINIMAL_EXAMPLE.c 复制正常流程；README_FULL_EXAMPLE.c 展示全部公开 API，其中取消类 API 会以 #if 0 隔离。

**注意：** 所有指针和数组都由调用者拥有；先检查返回值。异步采集、DMA 或回调还在使用 buffer 时，不能读取结果或改写该 buffer。

### 示例、模块链与验收

- 最小入门：`README_MINIMAL_EXAMPLE.c`，只保留正常入口和结果读取。
- 全功能：`README_FULL_EXAMPLE.c`，以正确顺序展示当前头文件全部公开 API；`Stop` 等非常规路径不会默认执行。
- 模块链：先由上游提供单位、采样率和有效数据，再调用本模块；成功后将输出交给显示、控制、测量或下一步 DSP。硬件资源仍以 SysConfig 合约为唯一来源。
- 文档验收：README/API、两份示例和头文件会由 `tools/validate_beginner_documentation.ps1` 覆盖检查；这只表示文档与源码签名一致，不代替未进行的实板验证。

### 模块链

`上游采集/配置 -> 本模块 -> 检查返回值和结果 -> 测量、显示、控制或下一步 DSP`。只有确认本模块的输出单位和有效状态后，才交给下一模块。

