# Harmonic：正确找 2~5 次谐波

> **LEVEL C / REAL ALGORITHM MODULE：** 谐波目标频率、Nyquist、邻 bin 积分和幅值/能量区别直接决定 THD 正确性，不能降成散落循环。

## 比赛复制版：先看这里

**适合：** 已有 FFT magnitude 和基波频率，要按 1～N 次谐波目标频率在邻近多个 bin 积分。不要只取单个 bin 来测非相干采样谐波。

**复制到 `modules/`：** `signal_harmonic.c`、`signal_harmonic.h`、`05_precision/multi_bin_energy/signal_multi_bin_energy.c`、`05_precision/multi_bin_energy/signal_multi_bin_energy.h`，以及 `03_measurement/common/signal_algorithm_status.h`。无 SysConfig/Pin。

```c
#include "signal_harmonic.h"

const signal_harmonic_config_t harmonic_config = {
    .fundamental_frequency_hz = fundamental_hz,
    .first_order = 1U,
    .last_order = 5U,
    .radius_bins = 2U
};
signal_harmonic_result_t harmonics;
signal_algorithm_status_t status = SignalHarmonic_Process(
    magnitude, N / 2U + 1U, Fs, N, &harmonic_config, &harmonics);
if (status == SIGNAL_ALGORITHM_OK) {
    // ===== 这里写你自己的逻辑 =====
    // harmonics.items[1] ... items[5]
}
```

**输入 / 输出：** magnitude + Fs/N + 已知基波 Hz -> 每阶目标频率、中心/起止 bin、energy 和 root-sum-square。最大阶数 10；超过 Nyquist 会返回错误。

| 题目变化 | 修改 |
|---|---|
| 要 H2～H5 | `first_order=1,last_order=5`，保留 H1 给 THD 分母 |
| Hann 泄漏较明显 | 从 `radius_bins=2` 起做合成验证 |
| 相邻谐波窗口重叠 | 减小 radius、增大 N 或调整 Fs；不能硬算 |
| 基波估计更精确 | 上游接 FFT Interpolation 后把 Hz 传入 |

**Build / 最小验证：** 人工在基波及整数倍附近布置已知 magnitude，检查每阶积分范围和 energy。隔离复制工程已 `SysConfig / Compile / Full Link PASS`，Flash 2936 B、SRAM（含栈）877 B。完整代码见 `README_MINIMAL_EXAMPLE.c`。

**连接：** `FFT Magnitude -> Peak/Interpolation -> Harmonic -> THD`。常见错误是漏复制 MultiBinEnergy、基波 Hz 不准、radius 窗口重叠、或高次谐波越过 Nyquist。

> 下文保留多 bin 原理和详细 API；比赛 COPY 以本节为准。

## 你真的需要这个模块吗？

**已有 magnitude 频谱和可靠基波频率，并且要整理 H1～H5 等谐波时需要。** 这是 C `ALGORITHM_MODULE`，只处理内存数据。

## 你应该已经有什么输入数据

非负频率 magnitude、`bin_count`、真实 `Fs`、FFT size 和精细基波频率。

## 最短接入步骤

1. **文件：** 复制顶部清单（含 Multi Bin Energy）到 `modules/`，include `signal_harmonic.h`；不需要算法仓库 Include Path。
2. **参数：** 基波频率、起止谐波阶数、积分半径 bins、`Fs` 和 FFT size。
3. **Workspace / Result：** 准备 `signal_harmonic_config_t config` 与 `signal_harmonic_result_t result`。
4. **调用：** `SignalHarmonic_Process(magnitude, bin_count, Fs, N, &config, &result)`。
5. **输出：** 每阶目标频率、bin 范围、energy 和 root-sum-square。
6. **连接下一步：** THD、谐波表显示、质量判限。
7. **Build / 最小验证：** 已知纯单音时 H1 应占主导；所有目标谐波必须低于 Nyquist，积分窗口不能重叠。

> 算法边界：不配置 Pin，不修改 SysConfig，不调用 DriverLib，也不需要 Platform Adapter。上游硬件变化时，只把真实 `Fs/N/VREF` 等事实同步到算法参数。

## 1 这个算法是干什么的？

已知基波频率 f0 后，计算每个 h*f0 在 FFT 的实际 fractional bin，取最近中心并积分邻 bin，输出各阶能量。

## 2 一个最简单的例子

Fs=102400、N=1024、f0=1000 Hz，bin 间隔100 Hz：H1=bin10、H2=20、H3=30。

## 3 原理

目标 bin=`h*f0*N/Fs`。不能简单写 `magnitude[2*k]`，因为 k 可能只是四舍五入后的基波 bin；非整 bin 时误差会随 h 放大。每阶用 MultiBinEnergy 收集主瓣。

## 4 比赛里什么时候用？

谐波表、THD、判断削顶/非线性失真；24_C 只用 FFT 插值基波计算 H1~H5，使用 Hann 后的邻近 bin 能量，不能让 Timer Capture 频率覆盖 `fundamental_frequency_hz`。

## 5 输入

非负频率 magnitude、bin_count=N/2+1、Fs、N、f0 Hz、阶数1~10、radius。

## 6 输出

每阶目标 Hz、fractional/center bin、实际积分范围、energy/RSS。

## 7 API怎么调用

```c
signal_harmonic_config_t c={f0,1U,5U,2U};
SignalHarmonic_Process(m,bins,Fs,N,&c,&r);
```

## 8 参数怎么改

BASIC：相干采样 radius=0。COMPETITION：Hann、插值 f0、radius=2 起步，再用频偏扫频确认。last_order 不得让 h*f0 超 Nyquist。

## 9 参数改大会怎样

阶数大覆盖更多失真但靠近 Nyquist；radius 大收能量更多也收噪声，且低 f0 时谐波窗口可能重叠并被拒绝。

## 10 这个算法的代价是什么

Benefits：不假设整 bin；结果可诊断；固定 RAM。

Trade-offs：f0 偏差传给全部谐波；仅看预定谐波，不发现非谐波杂散；前端频响未校正时高阶偏小。

## 11 什么时候不要用

基波未知/误检、谐波超过 Nyquist、窗带重叠、多音互相占据谐波位置。

## 12 怎么和前一个模块接

`Hann -> FFT -> Magnitude + PeakInterpolation(f0) -> Harmonic`

## 13 怎么和后一个模块接

`Harmonic -> THD / harmonic table`

## 14 最小Demo

```c
signal_harmonic_config_t c={1000,1,5,2};
signal_harmonic_result_t r;
(void)SignalHarmonic_Process(m,bins,Fs,N,&c,&r);
```

## 15 PC测试

A1=0.5、A2=0.05、A3=0.025：相干单 bin 找到10/20/30；1037 Hz Hann+radius2 完整链通过。未伪造高阶未测结果。

排查：H2/H3 很低先查前端/窗/radius；bin 错查 f0、Fs、N；OUT_OF_RANGE 查 Nyquist或窗口重叠。

## 16 MCU资源

最多 11 个固定 item；计算 O(H·(2R+1))，无大型工作区。

## 17 验证状态

PC_VERIFIED：BASIC 与 COMPETITION 合成真值通过；未实板频响校正。

## 17. 统一 API 教程（已按当前头文件核对）

本节由当前公开头文件、实现中实际出现的状态码和正式模块注册表生成。它补充前文的场景教程；函数签名变化时必须重新运行 `tools/upgrade_formal_beginner_docs.ps1`，不要手工保留旧 API。

遵循仓库的 [Beginner README 标准](../../00_docs/BEGINNER_README_STANDARD.md)：先用最小示例完成一次正常数据流，再按需要阅读全功能示例和本节 API 细节。

### 什么时候用 / 什么时候不要用

当题目需要“harmonic”目录对应的公开功能，并且输入数据、单位和硬件资源满足前文约束时使用本模块。若只需要更简单的上游功能、输入尚未准备好，或需要不同的数据模型/外设资源，应先选择相邻模块而不是强行调用本 API。

### 输入 / 输出

输入由各 API 的只读数组、配置、频率/阈值和平台对象组成；输出写入 result/output/buffer 参数或由 Get API 返回。调用者负责数组容量、生命周期和物理单位；失败返回时输出不是有效结果。

### 调用顺序

```text
SignalHarmonic_Process
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

### `signal_algorithm_status_t SignalHarmonic_Process(const float *magnitude, uint32_t bin_count, float sample_rate_hz, uint32_t fft_size, const signal_harmonic_config_t *config, signal_harmonic_result_t *result);`

**它做什么：** 按已知基波频率定位各次谐波，并对每次谐波附近多个 bin 积分。

**什么时候调用：** 对调用者提供的数据执行一次同步计算或生成，并在成功后写入输出对象/数组。

| 参数 | 类型 | 初学者解释 |
|---|---|---|
| `magnitude` | `const float *` | FFT 非负频率线性 magnitude。 |
| `bin_count` | `uint32_t` | 通常为 fft_size/2+1。 |
| `sample_rate_hz` | `float` | 采样率，Hz。 |
| `fft_size` | `uint32_t` | FFT 点数。 |
| `config` | `const signal_harmonic_config_t *` | 基波频率、阶数范围 1~10、bin 半径。 |
| `result` | `signal_harmonic_result_t *` | 输出每阶目标频率、中心/范围和能量。 |

**返回：** 成功返回 SIGNAL_ALGORITHM_OK；超 Nyquist、窗口重叠或参数非法返回错误码。

**最小调用形状：** `SignalHarmonic_Process(...);`。可直接从 README_MINIMAL_EXAMPLE.c 复制正常流程；README_FULL_EXAMPLE.c 展示全部公开 API，其中取消类 API 会以 #if 0 隔离。

**注意：** 所有指针和数组都由调用者拥有；先检查返回值。异步采集、DMA 或回调还在使用 buffer 时，不能读取结果或改写该 buffer。

### 示例、模块链与验收

- 最小入门：`README_MINIMAL_EXAMPLE.c`，只保留正常入口和结果读取。
- 全功能：`README_FULL_EXAMPLE.c`，以正确顺序展示当前头文件全部公开 API；`Stop` 等非常规路径不会默认执行。
- 模块链：先由上游提供单位、采样率和有效数据，再调用本模块；成功后将输出交给显示、控制、测量或下一步 DSP。硬件资源仍以 SysConfig 合约为唯一来源。
- 文档验收：README/API、两份示例和头文件会由 `tools/validate_beginner_documentation.ps1` 覆盖检查；这只表示文档与源码签名一致，不代替未进行的实板验证。

### 模块链

`上游采集/配置 -> 本模块 -> 检查返回值和结果 -> 测量、显示、控制或下一步 DSP`。只有确认本模块的输出单位和有效状态后，才交给下一模块。

