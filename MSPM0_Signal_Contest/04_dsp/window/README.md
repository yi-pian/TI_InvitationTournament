# Window：FFT 前为什么要“加窗”

> **LEVEL C / REAL ALGORITHM MODULE：** 统一入口同时生成样本权重、coherent gain 和 power gain；Hann/Hamming/Blackman/Rectangular 子目录只作兼容别名，不再单独选择。

## 比赛复制版：先看这里

**适合：** FFT 前减少记录首尾不连续造成的频谱泄漏。正常频谱测量默认先用 Hann；整周期严格相干采样才常用 Rectangular。

**复制到 `modules/`：** `signal_window.c`、`signal_window.h` 和 `03_measurement/common/signal_algorithm_status.h`。这是统一 Window 入口；不需要再复制 Hann/Hamming/Blackman 子目录。无 SysConfig/Pin。

```c
#include "signal_window.h"

static float windowed_v[N];
signal_window_result_t win;
signal_algorithm_status_t status = SignalWindow_Apply(
    centered_v, windowed_v, N, SIGNAL_WINDOW_HANN, &win);
if (status == SIGNAL_ALGORITHM_OK) {
    // ===== 这里写你自己的逻辑：windowed_v[] -> FFT =====
    // win.coherent_gain 留给幅值校正使用。
}
```

**输入 / 输出：** `float input[N]` -> `float output[N]`，并输出 coherent gain 和 power gain；允许原地覆盖。

| 需求 | 选择 |
|---|---|
| 日常 FFT/频率幅值 | Hann（默认） |
| 更低旁瓣但主瓣更宽 | Blackman |
| 兼顾旁瓣与幅值稳定 | Hamming |
| 严格整周期/不想加窗 | Rectangular |

**Build / 最小验证：** 全 1 输入加 Hann 后首尾接近 0，gain 为合理正值。隔离复制工程已 `SysConfig / Compile / Full Link PASS`，Flash 7288 B、SRAM（含栈）557 B。完整代码见 `README_MINIMAL_EXAMPLE.c`。

**连接：** `Remove DC -> Window -> FFT`。常见错误是把 `coherent_gain` 忘在幅值校正之外、N<2、或 Window 与后续 N 不一致。

> 下文保留窗口差异与详细 API；比赛 COPY 以本节为准。

## 你真的需要这个模块吗？

**已有一帧时域数据，准备做 FFT 且不能保证相干采样时需要。** 这是 C `ALGORITHM_MODULE`，只处理内存数据。

## 你应该已经有什么输入数据

`const float input[N]`；普通频谱默认 Hann，只有相干采样等明确条件才换窗。

## 最短接入步骤

1. **文件：** 正常应用复制统一入口 `signal_window.c/.h` 和顶部列出的 status 头；不要再复制 Hann/Hamming/Blackman/Rectangular 子实现。
2. **参数：** `N` 和 `signal_window_type_t`。
3. **Workspace / Result：** 准备 `float output[N]` 和 `signal_window_result_t result`。
4. **调用：** `SignalWindow_Apply(input, output, N, SIGNAL_WINDOW_HANN, &result)`。
5. **输出：** 加窗后的时域 buffer、coherent gain 和 power gain。
6. **连接下一步：** FFT；需要还原幅值时把 gain 交给 Window Gain Correction。
7. **Build / 最小验证：** Rectangular 输出应等于输入；Hann 两端应接近 0，gain 应为有限正数。

> 算法边界：不配置 Pin，不修改 SysConfig，不调用 DriverLib，也不需要 Platform Adapter。上游硬件变化时，只把真实 `Fs/N/VREF` 等事实同步到算法参数。

## 1 这个算法是干什么的？

FFT 会把当前 N 点记录当作首尾无缝重复。若真实记录首尾不连续，接缝像一次跳变，会把能量撒到许多频点，这叫频谱泄漏。窗函数把边缘压低，让接缝变平滑。

## 2 一个最简单的例子

8 点全 1 输入：矩形窗仍全 1；Hann 两端变 0，中间较大。它不是“删掉两点”，而是整段按平滑权重缩放。

## 3 原理

`y[n]=x[n]w[n]`。边缘越平滑，旁瓣通常越低，泄漏越少；但频域主瓣会更宽，相邻频率更难分开。Hann 能减泄漏正是因为它降低记录边界不连续，不是因为 FFT 本身改变了。

相干增益 `CG=mean(w)`。窗把同一正弦的峰值压小，所以幅值要除以 CG；否则“泄漏少了”却报告了错误电压。

## 4 比赛里什么时候用？

未知正弦频率、非相干采样的频谱/频率/THD。严格相干采样且要分辨很近频率时可用 Rectangular。

## 5 输入

float 数组、`count>=2`、window type。输入通常已 RemoveDC；若要看 DC 则保留。

## 6 输出

加窗数组（单位仍为 V）及 `coherent_gain=mean(w)`、`power_gain=mean(w²)`。两种 gain 不可混用。

## 7 API怎么调用

```c
signal_window_result_t wr;
SignalWindow_Apply(x,x,N,SIGNAL_WINDOW_HANN,&wr);
```

## 8 参数怎么改

改 type，不要凭“窗越高级越好”。比赛默认未知单音常选 Hann；弱小分量旁边有强音可看 Blackman；相干采样选 Rectangular。

## 9 参数改大会怎样

这里没有连续大小参数。选择更低旁瓣窗通常降低远端泄漏，但主瓣更宽、频率分辨细节变差，CG 也不同。

## 10 这个算法的代价是什么

Benefits：控制边界泄漏；返回实际增益避免硬写 0.5。

Trade-offs：幅值衰减、主瓣变宽、噪声等效带宽改变；本 RAM-saving 实现运行 `cosf`，CPU 比查表高。

## 11 什么时候不要用

时域 DC/Vpp/RMS 没有明确窗定义时；单次瞬态需要保留边缘时；以为窗能把 `Fs/N` 变成更细真实分辨率时。

## 12 怎么和前一个模块接

```text
ADC_ToVoltage -> RemoveDC -> Window
```

## 13 怎么和后一个模块接

```text
┌──── Window ────┐
│ y=x*w          │
│ CG=mean(w)     │
└──────┬─────────┘
       ↓
FFT -> Magnitude -> WindowGainCorrection(CG)
```

## 14 最小Demo

```c
signal_window_result_t w;
(void)SignalWindow_Apply(samples,samples,N,SIGNAL_WINDOW_HANN,&w);
```

## 15 PC测试

四种窗 8 点端点和解析 CG 全部匹配；Hann+1024 FFT 对 0.5 Vpeak 正弦修正后为 0.4999988 V，PASS。

排查：幅值偏小检查是否漏掉 GainCorrection；峰变宽是窗的正常代价；旁瓣异常先确认 RemoveDC、记录长度和窗定义分母 N-1。

## 16 MCU资源

O(N)，内部 O(1)。不占 `4N` 系数表 RAM；但 Hann/Hamming 每点一次 `cosf`，Blackman 两次。频繁重复同 N 时，可未来增加 const/调用者表版本并做 RAM/CPU权衡。

## 17 验证状态

PC_VERIFIED：2026-08-07；四窗解析测试和完整 FFT 幅值链通过，未实板计时。

## 17. 统一 API 教程（已按当前头文件核对）

本节由当前公开头文件、实现中实际出现的状态码和正式模块注册表生成。它补充前文的场景教程；函数签名变化时必须重新运行 `tools/upgrade_formal_beginner_docs.ps1`，不要手工保留旧 API。

遵循仓库的 [Beginner README 标准](../../00_docs/BEGINNER_README_STANDARD.md)：先用最小示例完成一次正常数据流，再按需要阅读全功能示例和本节 API 细节。

### 什么时候用 / 什么时候不要用

当题目需要“window”目录对应的公开功能，并且输入数据、单位和硬件资源满足前文约束时使用本模块。若只需要更简单的上游功能、输入尚未准备好，或需要不同的数据模型/外设资源，应先选择相邻模块而不是强行调用本 API。

### 输入 / 输出

输入由各 API 的只读数组、配置、频率/阈值和平台对象组成；输出写入 result/output/buffer 参数或由 Get API 返回。调用者负责数组容量、生命周期和物理单位；失败返回时输出不是有效结果。

### 调用顺序

```text
SignalWindow_Apply
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

### `signal_algorithm_status_t SignalWindow_Apply(const float *input_samples, float *output_samples, uint32_t count, signal_window_type_t type, signal_window_result_t *result);`

**它做什么：** 生成并逐点应用指定窗，同时返回相干增益和功率增益。

**什么时候调用：** 对调用者提供的数据执行一次同步计算或生成，并在成功后写入输出对象/数组。

| 参数 | 类型 | 初学者解释 |
|---|---|---|
| `input_samples` | `const float *` | 输入数组，只读，单位任意。 |
| `output_samples` | `float *` | 输出数组，容量至少 count；允许与输入为同一数组。 |
| `count` | `uint32_t` | 点数，至少为 2。 |
| `type` | `signal_window_type_t` | Rectangular/Hann/Hamming/Blackman。 |
| `result` | `signal_window_result_t *` | 输出 `mean(w)` 相干增益和 `mean(w^2)` 功率增益。 |

**返回：** 成功返回 SIGNAL_ALGORITHM_OK；参数或数值非法返回错误码。

**最小调用形状：** `SignalWindow_Apply(...);`。可直接从 README_MINIMAL_EXAMPLE.c 复制正常流程；README_FULL_EXAMPLE.c 展示全部公开 API，其中取消类 API 会以 #if 0 隔离。

**注意：** 所有指针和数组都由调用者拥有；先检查返回值。异步采集、DMA 或回调还在使用 buffer 时，不能读取结果或改写该 buffer。

### 示例、模块链与验收

- 最小入门：`README_MINIMAL_EXAMPLE.c`，只保留正常入口和结果读取。
- 全功能：`README_FULL_EXAMPLE.c`，以正确顺序展示当前头文件全部公开 API；`Stop` 等非常规路径不会默认执行。
- 模块链：先由上游提供单位、采样率和有效数据，再调用本模块；成功后将输出交给显示、控制、测量或下一步 DSP。硬件资源仍以 SysConfig 合约为唯一来源。
- 文档验收：README/API、两份示例和头文件会由 `tools/validate_beginner_documentation.ps1` 覆盖检查；这只表示文档与源码签名一致，不代替未进行的实板验证。

### 模块链

`上游采集/配置 -> 本模块 -> 检查返回值和结果 -> 测量、显示、控制或下一步 DSP`。只有确认本模块的输出单位和有效状态后，才交给下一模块。

