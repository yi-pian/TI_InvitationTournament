# FFT：把时域拆成频率 bin

> 新比赛工程底层统一 CMSIS-DSP。普通 FFT 直接用 `arm_cfft_*`/`arm_rfft_*`；当前 `SignalFFT_*` 只作为已有 Application 的薄兼容 Glue。默认候选为 Q15，最终 Q15/Q31/F32 仍由 RAM、误差和板上周期决定；Reference C 仅用于 PC truth。

> **LEVEL C / REAL ALGORITHM MODULE：** Radix-2、复数格式、Backend、缩放和大 buffer 都不是比赛现场应该重写的十几行公式。

## 新比赛工程：先看这里

**适合：** 已有 N 点去 DC/加窗后的时域 `float input[N]`，要得到复数频谱。FFT 不直接输出 V 或主频，后面还要接 Magnitude/Peak。

**普通 FFT 不复制本目录核心。** 母版已经配置 CMSIS-DSP；Q15 CFFT 最小代码见 `00_docs/recipes/cmsis_fft_spectrum.md`，Q15/Q31/F32 选择见 `00_docs/CMSIS_DATATYPE_SELECTION_GUIDE.md`。本目录 `SignalFFT_*` 仅是维护既有 Application 的兼容 Glue。无 SysConfig/Pin。

```c
#include "arm_const_structs.h"
#include "arm_math.h"

// ===== 你需要根据题目修改；N 必须是 2 的幂 =====
#define N  (1024U)
static q15_t fft_q15[2U * N];
arm_cfft_q15(&arm_cfft_sR_q15_len1024, fft_q15, 0U, 1U);
```

**输入 / 输出：** 此 Q15 例子输入输出都是交错复数 `q15_t[2N]`，偶数下标为实部、奇数下标为虚部；实信号后续通常只看 0～N/2。输入归一化、饱和、FFT 缩放和幅值修正必须按 CMSIS Recipe 处理。

| 题目变化 | 修改/影响 |
|---|---|
| 频率分辨率要高 | 减小 `Fs/N`，常见做法是增大 N；观察时间/RAM/CPU 增加 |
| 最高频率要高 | 增大 Fs，同时确认 Nyquist 和前端带宽 |
| RAM 紧张 | 减小 N，或改用复数原地入口；严禁大数组放栈上 |
| backend 优化 | 改 backend config/构建选项，不改公开调用 API |

FFT complex buffer 单独占 `8N` 字节；N=1024 时为 8192 B。隔离复制工程（N=8 最小例）已 `SysConfig / Compile / Full Link PASS`，Flash 9496 B、SRAM（含栈）581 B。完整代码见 `README_MINIMAL_EXAMPLE.c`。

**最小验证：** 输入已知整数 bin 正弦，Magnitude 后主峰应落在该 bin。常见错误是 N 非 2 次幂、忘了 Remove DC/Window、把 complex 当 magnitude、Fs/N 与采集事实不一致。

**连接：** `Remove DC -> Window -> FFT -> FFT Magnitude -> Peak / Harmonic`。

> 下文旧 `SignalFFT_*` API 只供维护既有 Application；新工程以上面的 CMSIS 路径为准。

## 你真的需要这个模块吗？

**已有 N 点时域 buffer，并且要得到频率 bin/频谱时需要。** 这是 C `ALGORITHM_MODULE`，只处理内存数据。

## 你应该已经有什么输入数据

`const float input[N]`；通常先 Remove DC 和 Window；`N` 必须满足当前 backend 的约束。

## 最短接入步骤

1. **文件：** 新工程优先直接 include `arm_math.h`；母版已配置 CMSIS include/link。只有维护现有 `SignalFFT_*` Application 时才复制本目录兼容 Glue。
2. **参数：** `N`、输入容量、输出容量和构建时 backend。
3. **Workspace / Result：** 准备 `float input[N]` 与 `signal_complex_f32_t spectrum[N]`；大数组放静态区并先算 SRAM。
4. **调用：** `SignalFFT_ForwardReal(input, spectrum, N, N)`。
5. **输出：** 复频谱，不是电压幅值。
6. **连接下一步：** FFT Magnitude → Peak/Interpolation/Harmonic。
7. **Build / 最小验证：** 输入整数 bin 的已知正弦，主能量应出现在对应 bin；必须做 full link。

> 算法边界：不配置 Pin，不修改 SysConfig，不调用 DriverLib，也不需要 Platform Adapter。上游硬件变化时，只把真实 `Fs/N/VREF` 等事实同步到算法参数。

## 1 这个算法是干什么的？

FFT 高效计算 DFT，告诉你记录中各离散频率的复数强度。它不是黑盒分析器：后面仍需 Magnitude、Peak、插值和增益修正。

## 2 一个最简单的例子

8 点冲激 `1,0,0...` 的每个 FFT bin 都是 `1+j0`，所以所有 magnitude 都是 1。这是最基础冲激测试。

## 3 原理

DFT 把信号投影到 N 个复指数。FFT 利用 N 为 2 次幂时的对称性，以蝶形从 O(N²) 降到 O(NlogN)。频率网格是 `bin_frequency=Fs/N`；这解释了为什么最大 bin 只能给到网格点。

频谱泄漏来自记录首尾不连续；FFT 没有“算错”。Hann 通过平滑边界降低泄漏，但主瓣更宽。

## 4 比赛里什么时候用？

有噪声频率估计、多音/谐波/THD/SFDR、严重失真周期信号基波搜索。

## 5 输入

N 点 float 或 `{real,imag}`，N>=2 且为 2 次幂。通常先 RemoveDC 和 Window。

## 6 输出

N 点未归一化复频谱。实信号非负频率只需 0~N/2，但当前 FFT 工作区仍为完整 N complex。

## 7 API怎么调用

```c
SignalFFT_ForwardReal(windowed_v, spectrum, N, N);
/* RAM-saving: 先填 complex buffer，再原地 */
SignalFFT_ForwardComplexInPlace(spectrum, N);
```

## 8 参数怎么改

只改 N 并同步所有 buffer/采样记录；N 必须 512/1024/2048/4096 等 2 次幂。Fs 由采集层传递，不进入 FFT 数值执行。

## 9 参数改大会怎样

N 大：`Fs/N` 更小、观察时间更长，但 `8N` FFT RAM 与 O(NlogN) CPU 上升。信号在长记录内变化时，频谱会混合变化，不是永远越大越好。

## 10 这个算法的代价是什么

Benefits：完整频谱、可接多种测量、无 twiddle RAM 表。

Trade-offs：1024 complex 已 8192 字节；M0+ 无 FPU；当前不是 optimized real FFT；不自动校正窗和单边幅值。

## 11 什么时候不要用

方波干净边沿可 Timer Capture；只测 DC/Vpp/RMS；N/RAM 未预算；单次瞬态却要稳态频率。

## 12 怎么和前一个模块接

```text
ADC_ToVoltage -> RemoveDC -> Hann -> FFT
```

## 13 怎么和后一个模块接

```text
┌──── FFT ────┐
│ complex[N]  │
└──────┬──────┘
       ↓
Magnitude -> Peak -> ParabolicInterpolation
          -> Harmonic / THD
```

## 14 最小Demo

```c
static signal_complex_f32_t spectrum[1024];
(void)SignalFFT_ForwardReal(x, spectrum, 1024U, 1024U);
```

## 15 PC测试

8 点冲激的 5 个非负 bin magnitude 全为 1；1024 点 Hann 链对精确 bin 1000 Hz 误差 0.000122 Hz，对 1037 Hz 线性抛物线误差 4.833 Hz（在声明容差内）。PASS。

排查：频率比例错查 Fs；峰散开查相干/Window；幅值大约 N/2 倍说明还没归一化；N 非 2 次幂返回 NOT_SUPPORTED。

## 16 MCU资源

complex `8N` 字节，内部 O(1)，O(NlogN)。N=4096 单 complex buffer 就 32768 字节，已经占满 MSPM0G3507 标称 32 KB RAM，实际不可用；详见 FFT_MEMORY_BUDGET。

## 17 验证状态

PC_VERIFIED：2026-08-07，严格编译、冲激和完整正弦链通过；未 TI Arm Clang/板端性能验证。

## 18. 完整 Public API Reference

### `SignalFFT_ForwardComplexInPlace(data, count)`

`data` 为可写 `signal_complex_f32_t[count]`，输入时是复时域、返回时被未归一化复频谱覆盖；N 必须是至少 2 的 2 次幂。成功 OK；非法 N/数值返回对应错误。最省 RAM，但调用者要先把实数填到 `.real` 并把 `.imag=0`。

### `SignalFFT_ForwardReal(input_samples, spectrum, count, spectrum_capacity)`

小白优先入口：只读 `float[count]` 实时域，输出 `signal_complex_f32_t[count]`；capacity 至少 count；N 同样必须为 2 次幂。内部先复制实部/清零虚部，再调用原地 FFT。失败时不要消费 spectrum。

两个函数均同步、每帧一次、无 Init/动态内存。输出采用 `exp(-j2πkn/N)` 且不除 N。

```text
RemoveDC -> Window -> ForwardReal -> complex spectrum[N]
                                 -> FFT Magnitude -> Peak/Harmonic
```

## 19. Buffer Rules / Realistic Example

real input=`4N` bytes，complex output=`8N` bytes；`ForwardReal` 不支持把 float 输入与 complex 输出重叠。`ForwardComplexInPlace` 只允许在自己的 complex 数组内原地。N=4096 时单 complex 已 32768 bytes，不能只看该数组而忽略栈/其他 buffer。

```c
signal_remove_dc_result_t dc;
signal_window_result_t win;
SignalRemoveDC_Process(voltage, voltage, N, &dc);
SignalWindow_Apply(voltage, voltage, N, SIGNAL_WINDOW_HANN, &win);
status = SignalFFT_ForwardReal(voltage, spectrum, N, N);
if (status == SIGNAL_ALGORITHM_OK) {
    status = SignalFFTMagnitude_Process(
        spectrum, N, magnitude, N / 2U + 1U, &mag_result);
}
```

## 20. Parameter / Backend / Algorithm Scope

公开 FFT API 只有 N，没有 Fs/window/backend 参数。Fs 只在后续 `k*Fs/N` 换算；window 在前一个模块；Backend 由构建/后端配置层决定，不是这两个函数的运行参数。N 增大：bin 间隔减小、观察时间/RAM/O(NlogN) 增加。

真实采样率由上游提供，本模块只接收 buffer 和 N。常见错误：N 非 2 次幂、容量不足、未去 DC/加窗、把 complex 当 magnitude、忘记未归一化、在栈上放大数组、Fs/N 不一致。

## 21. Result Meaning

`spectrum[k].real/imag` 是复 DFT 系数，不是 V；实输入的非负频率看 `k=0..N/2`，但当前 buffer 仍需 N complex。必须再接 FFT Magnitude；要物理单边峰值还需 N、单边因子和 coherent gain 校正。

## 22. Quick Modify Table

| 我想改什么 | 去哪里 | 改什么 | 影响 | 是否需同步上游 |
|---|---|---|---|---|
| FFT N | Application config + 全部 buffer | count/capacity | RAM、CPU、`Fs/N` | 否 |
| Fs | Acquisition + Hz换算 | actual sample rate | Nyquist/bin Hz | 改硬件Fs时是 |
| 窗 | Window config | type | 泄漏/幅值校正 | 否 |
| Backend | 构建/算法后端配置 | backend selection | 性能/数值格式 | 否 |
| 输出幅值 | 后续链 | Magnitude + GainCorrection | 物理量含义 | 否 |

## API Reference

- `SignalFFT_ForwardReal(input_samples, spectrum, count, spectrum_capacity)`
- `SignalFFT_ForwardComplexInPlace(data, count)`
