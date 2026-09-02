# Integration Glue 使用说明

代码分类：本 README 中的局部调用块均为 `【ILLUSTRATIVE SNIPPET】`；完整可链接调用以对应 Round 1 Application 源码与 projectspec 为准。

## 第一次使用 Integration Glue？从这里开始

目标：“不在 `main.c` 重复写 raw→voltage、基础测量、频谱、THD 或双相位的长调用链”。它只是应用层接线板，不是第二套算法。

### STEP 1：加入工程

- 唯一正式源码/头文件：`MSPM0_Signal_Contest/08_applications/common/signal_integration.c/.h`
- Include Path：`08_applications/common`、算法 `03_measurement/common`，以及你使用链涉及的每个正式算法目录。
- 维护现有 Application：除了 `signal_integration.c`，还必须保留目标 projectspec 中列出的正式算法 linked source。
- 新比赛母版：按相近 Build-verified Application 的 source manifest，把 `signal_integration.c/.h` 与实际调用链需要的算法文件冻结复制到 `modules/`；不要把整库或未使用算法一起复制。

`signal_integration.c` 包含多条链。维护旧工程时可参考相近 Application projectspec 的 `<file ... action="link">`；新比赛母版则把这些条目当作“必要源文件清单”转换为冻结复制，不照搬根变量。只调用一个简单算法时，直接用该算法/Recipe 通常比引入全部 Glue 依赖更清楚。

### STEP 2：include

```c
#include "signal_integration.h"
```

上游硬件仍 include 自己的头文件，例如 `signal_adc_dma.h`。

### STEP 3：准备变量 / Workspace / Result

以 Signal Meter 为例：

```c
uint16_t raw[N];
float voltage_workspace[N];
signal_zero_cross_event_t events[N];
float crossing_positions[N];
signal_meter_result_t result;
```

所有 workspace 由 Application 创建。Glue 不做动态分配；频率未启用时仍可按你的调用方式省掉事件 workspace，具体以公开函数参数为准。

### STEP 4：第一次真正要改的参数

| 参数 | 作用 | 调大/调小或写错后的现象 | SysConfig |
|---|---|---|---|
| `count=N` | 帧长、FFT N | RAM/CPU/频率分辨率变化；所有容量同步 | 否 |
| `adc_bits/VREF/scale/offset` | raw→V | 所有电压结果比例或偏置错误 | 硬件 ADC 变化时是 |
| `sample_rate_hz` | 过零/FFT 的 Hz 换算 | 写错会使频率、相位按比例错 | 真实 Fs 变化时是 |
| `measurement_mask` | 开关基础测量 | 只组合 `SIGNAL_METER_MEASURE_*` 位 | 否 |
| `expected_min/max_hz` | FFT 主峰搜索范围 | 太窄漏峰，太宽可能选干扰 | 否 |
| `harmonic_bin_radius` | 谐波积分半径 | 大会纳入噪声/邻峰，小会漏泄漏能量 | 否 |
| `maximum_lag_samples` | 相关搜索范围 | workspace=`2L+1`，L 必须<N | 否 |

### STEP 5：需要修改 SysConfig 吗？

Glue 本身：**【不需要 SysConfig】**。上游硬件按链选择 P01 单 ADC、P02 双 ADC、P03 DAC、P04 ADC+DAC。Glue 不拥有 ADC/DMA/Timer/IRQ。

### STEP 6：初始化

Glue 没有初始化函数。先初始化硬件、采满一帧，再直接调用相应公开 Glue API。

### STEP 7：真正调用模块

```c
uint32_t mask = SIGNAL_METER_MEASURE_DC |
                SIGNAL_METER_MEASURE_VPP |
                SIGNAL_METER_MEASURE_RMS |
                SIGNAL_METER_MEASURE_FREQUENCY;

signal_algorithm_status_t status = SignalIntegration_SignalMeter(
    raw, N, 12U, 3.3f, 1.0f, 0.0f,
    actual_fs_hz, mask, 0.005f,
    voltage_workspace, N,
    events, N, crossing_positions, N,
    &result);
```

参数顺序按头文件：raw/count、ADC 换算、Fs/mask/滞回、三个 workspace 及容量、result。不要只复制调用而漏容量参数。

### STEP 8：结果去哪里拿

成功后读取 `result.dc_v/vpp_v/rms_v/frequency_hz` 等字段；只有 mask 开启的字段有效。频率还要检查 `frequency_valid`。Spectrum/THD/Phase 分别使用头文件中的专用 result struct，单位已体现在字段名 `_v/_hz/_deg/_percent`。

### STEP 9：怎么接下一个模块

硬件到 Glue：

```c
const uint16_t *frame = SignalADC_GetBuffer();
size_t count = SignalADC_GetSampleCount();
float fs = (float)SignalADC_GetConfiguredTriggerRate();
```

Spectrum 链使用 `SignalIntegration_Spectrum(voltage_workspace, count, fs, ...)`，需要 `complex[N]` 和 `magnitude[N/2+1]`。Dual Phase 链先把两块 raw 分别转成 `channel_a_v/channel_b_v`，再调用 `SignalIntegration_DualPhase(...)`；Glue 输出还能直接交给 UART/TFT 应用显示，不需要再改算法。

### STEP 10：第一次 Build

- 某个真实头文件找不到（例如 `signal_fft.h`）：漏了该正式算法的 Include Path。
- 某个真实符号未定义（例如 `SignalFFT_ForwardReal`）：projectspec 没链接该正式 `.c` 或其依赖。
- CMSIS symbol/library 错：FFT backend 与 projectspec 的 CMSIS Include/archive 不一致。
- RAM overflow：检查 voltage、complex、magnitude、events、correlation workspace 是否全部同时保留。
- SysConfig conflict：来自上游硬件 Profile，不是 Glue 本身。

### STEP 11：最小验证

先对 RawToVoltage 用 `{0,4095}` 真值；再对 Signal Meter 输入已知 DC+正弦，检查 DC/Vpp/RMS/frequency。频谱输入单音看主峰；双相位先用相同两路看结果接近 0°。

### STEP 12：最常见修改

1. **关闭某项测量**：只改 measurement mask；无需删算法源码前先 Build，确定链接死代码消除结果。
2. **N 512→1024**：同步 raw/voltage/events/positions/FFT/magnitude 容量；FFT N 保持 2 次幂；full link 看 `.map`。
3. **Fs 100 k→200 k**：硬件 Timer 和 Glue 的 `sample_rate_hz` 同步。
4. **Basic 改 Spectrum**：增加 complex/magnitude workspace 和正式 FFT 链依赖；每加一个模块先 Build。
5. **只需要 VPP**：可直接调用 ADC To Voltage+VPP，避免把整套 Glue 当大型框架。

### STEP 13：从头到尾最小示例

```c
#include "signal_integration.h"
#define N 128U
static uint16_t raw[N];
static float voltage[N];
static signal_zero_cross_event_t events[N];
static float positions[N];

void ProcessFrame(void)
{
    signal_meter_result_t r;
    uint32_t mask = SIGNAL_METER_MEASURE_DC | SIGNAL_METER_MEASURE_VPP;
    if (SignalIntegration_SignalMeter(raw, N, 12U, 3.3f, 1.0f, 0.0f,
            100000.0f, mask, 0.005f,
            voltage, N, events, N, positions, N, &r) ==
        SIGNAL_ALGORITHM_OK) {
        float vpp_v = r.vpp_v;
        (void)vpp_v;
    }
}
```

下面是每条 Glue API 的完整逐参数说明、Buffer 规则、资源和验证证据。

> 正式源码：`../signal_integration.c`、`../signal_integration.h`。本目录只放说明文档，不复制源码。

## 1. What It Does

把已经存在的采集转换、基础测量、过零测频、FFT、谐波、THD 和双通道相位 API 按已验证顺序连接起来。

小白理解：它是应用层“接线板”，不是第二套算法。你给它 raw/float buffer、点数和 workspace，它替你调用正式算法模块并汇总结果。

## 2. When To Use It

适合 Signal Meter、Spectrum、THD、双通道 Phase 等已经与现有函数签名一致的组合。只需单独调用一个算法、需要更换处理顺序，或需要保留中间结果时，直接调用各正式算法更清楚。

## 3. Where It Sits In The Signal Chain

```text
ADC DMA -> raw[N] -> Integration Glue -> meter / spectrum / THD result
Dual ADC -> A[N],B[N] -> RawToVoltage -> DualPhase -> phase result
```

## 4. Inputs / Outputs

- raw 输入：调用者拥有的 `const uint16_t raw[count]`，单位 ADC code。
- voltage workspace：调用者拥有的 `float[count]`；部分链会原地去 DC/加窗，调用后不再保留原电压。
- FFT workspace：`signal_complex_f32_t[count]`，至少 `count` 项。
- magnitude workspace：`float[count/2+1]`。
- 过零事件/位置：分别为 `signal_zero_cross_event_t[event_capacity]` 与 `float[position_capacity]`。
- 输出：头文件中四种 result struct，标量单位在字段名中用 `_v`、`_hz`、`_deg` 标明。

## 5. Dependencies

必须链接你实际调用链所用的正式算法源码；`signal_integration.c` 的直接依赖包括 ADC To Voltage、Mean、MinMax、VPP、RMS、AC RMS、Remove DC、Zero Cross、Zero Cross Interpolation、Multi Cycle Average、Hann、FFT、FFT Magnitude、Window Gain Correction、Peak Detect、FFT Parabolic Interpolation、Harmonic、THD、Correlation 和 Phase。完整清单以 `../signal_integration.c` 的 `#include` 和目标 `.projectspec` 为准。

本模块不直接依赖 DriverLib 或 SysConfig；上游 ADC/Dual ADC 才依赖硬件配置。

## 6. Public API Reference

### `SignalIntegration_RawToVoltage(...)`

作用：构造真实 `signal_adc_to_voltage_config_t` 并调用 `SignalADCToVoltage_Process`。

| 参数 | 类型/单位 | 含义与要求 |
|---|---|---|
| `raw` | `const uint16_t *`，code | 非空，至少 `count` 项 |
| `count` | `size_t`，sample | `1..UINT32_MAX` |
| `adc_bits` | `uint8_t`，bit | `1..16`；最大码由 `(1UL<<adc_bits)-1` 得到 |
| `reference_voltage_v` | `float`，V | ADC 换算参考电压；合法性由 ADC To Voltage 校验 |
| `input_scale` | `float` | 前端比例系数 |
| `offset_voltage_v` | `float`，V | 换算后加到结果上的偏置 |
| `voltage_v` | `float *`，V | 输出，至少 `count` 项 |
| `voltage_capacity` | `size_t`，sample | 必须不小于 `count` |

成功返回 `SIGNAL_ALGORITHM_OK`；空指针、位数或容量错误返回参数错误，下游可继续返回数值/范围错误。输入输出不能原地复用，因为元素类型不同。

### `SignalIntegration_FrequencyTime(...)`

作用：在 `voltage_v` 上原地 Remove DC，再执行 Zero Cross → Linear Interpolation → Multi Cycle Average。

| 参数 | 类型/单位 | 含义与要求 |
|---|---|---|
| `voltage_v` | `float *`，V | 输入/工作区，至少 `count` 项；会被去 DC 覆盖 |
| `count` | `size_t` | 至少能产生两个同方向过零；API 还要求不超过 `UINT32_MAX` |
| `sample_rate_hz` | `float`，Hz | 必须大于 0，使用真实配置采样率 |
| `hysteresis_v` | `float`，V | 过零滞回；非负性由 Zero Cross 校验 |
| `events` / `event_capacity` | 事件数组/项数 | 保存相邻夹点；容量不足会失败 |
| `crossing_positions` / `position_capacity` | `float[]`/项数 | 保存小数 sample 位置，容量至少有效事件数 |
| `frequency_hz` | `float *`，Hz | 成功时写平均频率 |

方向固定为上升沿，阈值固定为去 DC 后的 `0 V`。未找到足够事件、容量不足或样本顺序错误都会返回下游错误码。

### `SignalIntegration_SignalMeter(...)`

作用：先 raw→voltage，再按 `measurement_mask` 计算 DC、Min/Max、VPP、总 RMS、AC RMS 和过零频率。

- ADC 换算参数和 workspace 规则同 `RawToVoltage`。
- `measurement_mask` 只能由头文件中的 `SIGNAL_METER_MEASURE_*` 位组合；未开启的结果字段被置 0。
- 开启频率时必须提供 events、positions、正采样率和滞回；不开频率时这些 workspace 不会被使用。
- `result` 非空；成功时 `frequency_valid=1` 表示频率字段有效。

### `SignalIntegration_Spectrum(...)`

作用：对 `voltage_workspace` 原地 Remove DC、Hann，然后 FFT、Magnitude、窗增益校正、主峰和最多 8 个主要峰。

- `count` 必须满足 FFT 的 2 次幂要求；`fft_capacity>=count`。
- `magnitude_capacity>=count/2+1`。
- `sample_rate_hz>0`，且 `expected_max_hz>expected_min_hz`。
- `requested_peak_count<=SIGNAL_INTEGRATION_MAX_PEAKS`（当前为 8）。
- 输出主峰频率、fractional bin、幅值、coherent gain 和主要峰数组。该函数会改写 voltage/FFT/magnitude workspace。

### `SignalIntegration_THD(...)`

作用：复用 Spectrum 链找基波，再按 `harmonic_bin_radius` 计算 H1..H5 和 THD。

workspace 与 Spectrum 相同；`harmonic_bin_radius` 是每阶中心 bin 两侧纳入能量的半径。输入带宽必须允许 H5 不越过 Nyquist，否则 Harmonic 返回错误。输出含基波频率/幅值、`harmonic_amplitude_v[1..5]` 和 `thd_percent`。

### `SignalIntegration_DualPhase(...)`

作用：两路分别原地去 DC，以 FFT bin 相角和归一化互相关两种方法输出 B-A 相位。

- `channel_a_v`、`channel_b_v`：各 `count` 个 float，会被原地修改。
- `sample_rate_hz`、`signal_frequency_hz`：必须为正；后者用于选择目标 bin 并把 lag 换相位。
- `maximum_lag_samples<count`。
- `fft_a`、`fft_b` 容量均至少 `count`；`correlation_workspace` 容量至少 `2*maximum_lag_samples+1`。
- 输出 `fft_phase_deg`、`correlation_phase_deg`、相关系数和 lag。相位符号统一为 B-A。

## 7. Call Sequence

```text
硬件采集完成
  -> RawToVoltage（若所选组合入口不自带换算）
  -> 选择一个 SignalMeter / FrequencyTime / Spectrum / THD / DualPhase
  -> 仅在返回 SIGNAL_ALGORITHM_OK 后读取 result
  -> 下一帧前重新填充被覆盖的 workspace
```

所有公开函数都是同步函数，不需要单独 Init。不要在 DMA 仍写 buffer 时调用。

## 8. Minimal Example

```c
signal_meter_result_t result;
signal_algorithm_status_t status = SignalIntegration_SignalMeter(
    raw, N, 12U, 3.3f, 1.0f, 0.0f, actual_fs_hz,
    SIGNAL_METER_MEASURE_VPP | SIGNAL_METER_MEASURE_RMS,
    0.01f, voltage, N, events, EVENT_CAPACITY,
    crossing_positions, EVENT_CAPACITY, &result);
if (status == SIGNAL_ALGORITHM_OK) {
    /* result.vpp_v 和 result.rms_v 有效 */
}
```

## 9. Connecting To Other Modules

```text
SignalADC_GetBuffer() + SignalADC_GetSampleCount()
    -> SignalIntegration_SignalMeter(...)

raw -> RawToVoltage -> Spectrum -> result.frequency_hz / peak_amplitudes_v[]

DualADC A/B raw -> 两次 RawToVoltage -> DualPhase -> FFT/Correlation phase
```

真实调用可参考 `signal_meter`、`frequency_meter`、`spectrum_analyzer`、`harmonic_thd_analyzer`、`dual_channel_phase_meter` 与 `signal_analyzer`。

## 10. Parameter Guide

| 参数 | 作用 | 增大后的主要影响 | RAM/速度 | SysConfig |
|---|---|---|---|---|
| `count` | 帧长/FFT N | 观察更久、FFT 网格更细 | 所有 workspace 线性增大，CPU 增大 | 否；但上游 DMA 长度同步 |
| `sample_rate_hz` | sample→Hz 换算 | Nyquist 提高，固定 N 的网格变粗 | 算法 RAM 不变 | 算法否；改变真实采样需改硬件配置 |
| `hysteresis_v` | 抑制阈值附近重复过零 | 抗噪更强，也可能漏掉小信号 | 不影响 RAM | 否 |
| `expected_min/max_hz` | 限制主峰搜索 | 搜索范围改变 | RAM 不变 | 否 |
| `harmonic_bin_radius` | 每阶积分半径 | 纳入更多泄漏，也可能混入邻频 | RAM 不变、CPU略增 | 否 |
| `maximum_lag_samples` | 互相关搜索范围 | 可容纳更大延迟 | workspace=`2L+1`，CPU约 O(NL) | 否 |

## 11. Common Modification Tasks

- 改测量项：只改 `measurement_mask`，不改 SysConfig。
- 改 N：同步 raw、voltage、FFT、magnitude 和事件 workspace；Spectrum/THD/FFT Phase 要保持 2 次幂。
- 改 ADC 通道/VREF：通道和参考源改上游 `.syscfg`，数值换算同步改 `reference_voltage_v`。
- 保留原电压：不要直接把唯一副本传给会原地处理的 Frequency/Spectrum/THD/DualPhase；先复制到 workspace。

## 12. Config vs SysConfig

CONFIG ONLY：测量 mask、N、频率范围、滞回、峰数、谐波半径、最大 lag、各 workspace 容量。

SYSCONFIG REQUIRED：真实 ADC 通道/引脚、参考源、Timer/Event/DMA、双 ADC 路由。Glue 本身没有 SysConfig 实例。

## 13. SysConfig Setup

本模块直接配置：Not Applicable。上游单通道采集参考 `PROFILE_01_ADC_CAPTURE`，双通道参考 `PROFILE_02_DUAL_ADC` 或 `PROFILE_06_FULL_SIGNAL`；必须使用其对应应用中实际生成的宏名，不能手改 `ti_msp_dl_config.*`。

## 14. Resources / Memory

无独占 ADC/Timer/DMA/IRQ。主要资源由调用者 workspace 决定：raw `2N` bytes、voltage `4N`、complex FFT 每路 `8N`、magnitude `4*(N/2+1)`、相关 workspace `4*(2L+1)`，再加事件数组。以最终 `.map` 为准。

## 15. Buffer Rules

- 所有 buffer 由调用者创建和长期持有；模块不动态分配。
- raw 只读；RawToVoltage 输出不能与 raw 重叠。
- FrequencyTime、Spectrum、THD 会改写 voltage workspace。
- DualPhase 会改写两路 voltage 和全部 workspace。
- 容量参数按“元素数”而不是 byte；失败后不要把 result 当有效值。

## 16. Result Meaning

频率为 Hz；电压字段为 V；THD 为百分数；`fractional_bin*Fs/N` 对应频率；相位均为 `B-A`。Spectrum 输出幅值经过当前 Hann coherent-gain/单边校正链，但仍应以应用校准和真实前端比例验证。

## 17. Common Mistakes

- 把 `capacity` 写成字节数。
- Spectrum/THD 用非 2 次幂 N。
- 把请求 Fs 而非 `GetConfigured...Rate()` 传入。
- 同一 voltage buffer 先跑 Spectrum 后又当原电压做 RMS。
- 事件或 correlation workspace 太小。
- H5 已超过 Nyquist 仍要求 THD 到 5 次。
- 忘记链接 `signal_integration.c` 的全部正式算法依赖。

## 18. Verification

先用现有各应用的 PC/完整链接证据验证状态码和结果，再上板输入已知正弦：VPP/RMS 与真值比较，频率看误差，双通道用已知延迟检查 B-A 符号。没有新实板证据时不得写 BOARD_VERIFIED。

## 19. Realistic Example

```c
/* 完整频谱链：raw -> V -> RemoveDC/Hann/FFT/Magnitude/Peak */
status = SignalIntegration_RawToVoltage(raw, N, 12U, 3.3f,
    front_end_scale, front_end_offset_v, voltage, N);
if (status == SIGNAL_ALGORITHM_OK) {
    status = SignalIntegration_Spectrum(voltage, N, actual_fs_hz,
        min_hz, max_hz, fft, N, magnitude, N / 2U + 1U,
        4U, &spectrum_result);
}
```

## 20. Quick Modify Table

| 我想改什么 | 去哪里 | 改什么 | 影响 | SysConfig? |
|---|---|---|---|---|
| 测量项 | Application config/call site | `measurement_mask` | 运行时间和所需过零 workspace | 否 |
| N | Application config/buffer declarations | `count` 与全部容量 | RAM、时间、FFT 分辨率 | 否 |
| 真实 Fs | 采集配置 + 调用参数 | 上游 Timer 与 `sample_rate_hz` | 频率刻度 | 改硬件率时是 |
| ADC 通道 | 上游 `.syscfg` | ADC channel/pin | 输入来源 | 是 |
| FFT 搜索范围 | call site | `expected_min/max_hz` | 主峰候选范围 | 否 |
| THD 积分宽度 | call site | `harmonic_bin_radius` | 泄漏/邻频能量 | 否 |
| 相位搜索范围 | call site | `maximum_lag_samples` | RAM/CPU/最大延迟 | 否 |

## Integration Closure

- 本模块是纯软件的正式组合入口，不拥有 DriverLib callback、ISR 或 SysConfig 资源。
- 它调用独立算法仓库的唯一正式源码；Application 不复制 ADC→Voltage、FFT→Magnitude、THD 或 Phase 内部循环。
- 第一轮 Signal Meter、Frequency、Spectrum、THD、Phase 应用已完成 full link 回归；当前是 `BUILD_VERIFIED`，未做开发板验证。

## Copy Into Target Project

链接 `../signal_integration.c/.h`，并按照实际调用的 API 链接其正式算法依赖；最稳妥的可复制源清单以对应已验证 Application 的 `.projectspec` 为准。Include Path 使用稳定的库根变量，不复制算法源码到 Application。

## Hardware / Platform Binding

- Platform：Not Applicable。本模块是纯软件 Integration Glue，不拥有 callback、ISR 或 DriverLib peripheral。
- 唯一实现：`08_applications/common/signal_integration.c/.h`。
- 硬件由上游 ADC/Dual ADC 模块各自 README 绑定；本模块只消费 raw/float buffer。
- 真实应用示例：Signal Meter、Spectrum Analyzer、THD Analyzer、Phase Meter 的 Round 1 projectspec/full-link 回归。
