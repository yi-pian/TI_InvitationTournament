# ADC_ToVoltage：把 ADC 数字翻译成电压

> **LEVEL A / COMPATIBILITY_API：** 新比赛工程请直接使用详细的 [ADC To Voltage Recipe](../../00_docs/recipes/adc_to_voltage.md)，并把 VREF、最大码、前端比例和偏移集中放在 Application 配置中。旧 `.c/.h` 只兼容现有工程。

## 比赛复制版：先看这里

**适合：** 你已经有 `uint16_t raw[N]`，下一步算法需要单位为 V 的 `float voltage_v[N]`。

**30 秒路线：** 把 `signal_adc_to_voltage.c`、`signal_adc_to_voltage.h`、`03_measurement/common/signal_algorithm_status.h` 复制到母版 `modules/`；不需要 SysConfig、Pin、Platform 或其他文件。母版已包含 `modules` Include Path，Refresh 后确认 `.c` 参与 Build。

**输入 / 输出：** `uint16_t raw[N]` -> `float voltage_v[N]`。输出公式是 `raw/max_code*VREF*input_scale+offset`。

把下面放到 `main.c` 顶部：

```c
#include "signal_adc_to_voltage.h"

#define N  (1024U)
static uint16_t raw[N];
static float voltage_v[N];

// ===== 你需要根据题目/前端修改 =====
static const signal_adc_to_voltage_config_t adc_scale = {
    4095U, 3.3f, 1.0f, 0.0f
};
```

在 ADC DMA 完成后调用：

```c
signal_algorithm_status_t status = SignalADCToVoltage_Process(
    raw, voltage_v, N, &adc_scale);
if (status == SIGNAL_ALGORITHM_OK) {
    // ===== 这里写你自己的逻辑 =====
    // voltage_v[] 可交给 VPP/RMS/Remove DC。
}
```

| 题目变化 | 修改 |
|---|---|
| ADC 分辨率变化 | `adc_max_code` |
| VREF 变化 | `reference_voltage_v`；硬件 VREF 仍在 SysConfig/电路改 |
| 前端分压/增益变化 | `input_scale` |
| 前端引入固定偏置 | `offset_voltage_v` |

**Build / 最小验证：** 用 `{0, 1024, 2048, 4095}` 检查单调性、0 V 和满量程。隔离复制工程已 `SysConfig / Compile / Full Link PASS`，Flash 1896 B、SRAM（含栈）529 B，未新增板测。完整可链接代码在 `README_MINIMAL_EXAMPLE.c`。

**下一步：** `voltage_v -> VPP / RMS / AC RMS / Remove DC`。常见错误是把 ADC code 直接当 V、VREF 写错、前端比例方向写反或两个数组容量不足。

> 下文保留算法原理、资源和详细 API；若旧的“链接源码”措辞与本节冲突，比赛使用以本节的 COPY 路线为准。

## 你真的需要这个模块吗？

**已有 ADC raw code，并且结果必须用 V 表示时需要。** 这是 C `ALGORITHM_MODULE`，只处理内存数据。

## 你应该已经有什么输入数据

`const uint16_t raw[N]`；真实 ADC full-scale code、VREF，以及前端比例/校准偏置。

## 最短接入步骤

1. **文件：** 复制本节顶部清单到母版 `modules/`，include `signal_adc_to_voltage.h`；母版无需另加算法仓库 Include Path。
2. **参数：** `N`、`adc_max_code`、`reference_voltage_v`、`input_scale`、`offset_voltage_v`。
3. **Workspace / Result：** 调用者准备 `float voltage_v[N]` 和 config；模块不分配内存。
4. **调用：** `SignalADCToVoltage_Process(raw, voltage_v, N, &config)`。
5. **输出：** `voltage_v[0..N-1]`，单位 V。
6. **连接下一步：** Mean、VPP、RMS、AC RMS、Remove DC。
7. **Build / 最小验证：** 12 bit/3.3 V 时用 `{0, 4095}`，应约得 `{0.0, 3.3}` V。

> 算法边界：不配置 Pin，不修改 SysConfig，不调用 DriverLib，也不需要 Platform Adapter。上游硬件变化时，只把真实 `Fs/N/VREF` 等事实同步到算法参数。

## 1 这个算法是干什么的？

ADC 给你的 `2048` 不是 2048 V，而是一个原始码值。这个 Adapter 根据 ADC 满量程和参考电压，把每个 `uint16_t` code 换成单位为 V 的 `float`，让后续算法知道自己处理的是真实电压。

## 2 一个最简单的例子

12 位 ADC、参考 3.3 V、没有额外缩放：

```text
RAW:      0       2048       4095
Voltage:  0 V    约1.6504 V   3.3 V
```

## 3 原理

```text
voltage_v = raw_code / adc_max_code
            * reference_voltage_v
            * input_scale
            + offset_voltage_v
```

12 位最大可出现码值是 4095。`input_scale` 用来还原外部分压/放大，例如前端把 0~6.6 V 缩小一半送入 ADC，则可设 2.0。`offset_voltage_v` 是最后加到结果上的校正量，不是“去直流”。

## 4 比赛里什么时候用？

几乎所有以 ADC RAW 开始、最后要输出 V 的链都先用它：DC、Vpp、RMS、FFT 幅值、THD 幅值。只测 code 是否撞满量程时可以直接看 RAW，不必换算。

## 5 输入

- `const uint16_t *raw_codes`：ADC RAW，单位 code。
- `uint32_t count`：元素个数，必须大于 0。
- `adc_max_code`：最大码值；12 位通常 4095。
- `reference_voltage_v`：本次换算采用的参考电压，单位 V。
- `input_scale`：物理输入 / ADC 引脚电压，无量纲，不能为 0。
- `offset_voltage_v`：最终加法校正，单位 V。

## 6 输出

`float voltage_v[count]`，单位 V。调用者必须提前准备至少 `4*count` 字节。若发现 RAW 超过 `adc_max_code`，函数返回越界且不写任何输出。

## 7 API怎么调用

```c
signal_adc_to_voltage_config_t cfg = {
    .adc_max_code = 4095U,
    .reference_voltage_v = 3.3f,
    .input_scale = 1.0f,
    .offset_voltage_v = 0.0f
};

signal_algorithm_status_t status = SignalADCToVoltage_Process(
    raw, voltage_v, count, &cfg);
```

## 8 参数怎么改

改 ADC 分辨率时改 `adc_max_code`；VREF 变化时改 `reference_voltage_v`；前端分压/放大比变化时改 `input_scale`；做已知零偏校准时改 `offset_voltage_v`。

## 9 参数改大会怎样

- `reference_voltage_v` 或 `input_scale` 变大：所有输出电压同比变大。
- `offset_voltage_v` 变大：所有点整体向上平移，不改变 Vpp。
- `adc_max_code` 误设过大：换算电压整体偏小；误设过小还可能触发越界。

## 10 这个算法的代价是什么

Benefits：单位统一，后续代码不重复换算，校准参数集中。

Trade-offs：1024 点 `float` 输出占 4096 字节；Cortex‑M0+ 没有 FPU；VREF、分压电阻和运放增益误差会直接进入结果。

## 11 什么时候不要用

- 输入本来就是 `float` V；
- 只需要 RAW 码域快速限幅报警；
- 不知道 VREF/前端比例却要求绝对电压精度。此时先校准参数。

## 12 怎么和前一个模块接

```text
┌──────────── ADC_DMA ────────────┐
│ const uint16_t *raw + count     │
└────────────────┬───────────────┘
                 ↓
┌────────── ADC_ToVoltage ────────┐
│ code -> V；不访问 ADC 寄存器     │
└─────────────────────────────────┘
```

## 13 怎么和后一个模块接

```text
ADC_ToVoltage
      ├──> Mean / DC
      ├──> Vpp / RMS / AC_RMS
      ├──> ClippingDetect
      └──> RemoveDC -> ZeroCross / FFT
```

## 14 最小Demo

```c
const uint16_t raw[3] = {0U, 2048U, 4095U};
float voltage_v[3];
signal_adc_to_voltage_config_t cfg = {4095U, 3.3f, 1.0f, 0.0f};

if (SignalADCToVoltage_Process(raw, voltage_v, 3U, &cfg)
        == SIGNAL_ALGORITHM_OK) {
    /* voltage_v[2] 约为 3.3 V */
}
```

## 15 PC测试

测试输入 `{0, 2048, 4095}`，理论值 `{0, 2048*3.3/4095, 3.3}`；同时测试 4096 越界且输出保持不变。已实际运行，全部 PASS。

排查：整体比例错先查 VREF、`adc_max_code`、`input_scale`；整体平移查 `offset_voltage_v`；偶发越界查 DMA 数据对齐与 ADC 位宽。

## 16 MCU资源

时间 O(N)，内部 RAM O(1)，无动态内存。输出为 `4*N` 字节。软件浮点吞吐需在最终采样率下实测；离线处理 512/1024 点通常比逐点 ISR 处理更合适。

## 17 验证状态

PC_VERIFIED：2026-08-07，GCC C11 严格警告编译，真值与边界测试通过。尚未在 LP‑MSPM0G3507 上做 BOARD_VERIFIED。

## 18. 完整 API、调用顺序与 Buffer 规则

唯一公开函数是 `SignalADCToVoltage_Process(raw_codes, voltage_v, count, config)`：每帧 ADC DMA 完成后调用一次，无 Init。`raw_codes` 为只读 `uint16_t[count]`；`voltage_v` 为调用者可写 `float[count]`；`count>0`；`config` 四个字段必须有效。成功返回 `SIGNAL_ALGORITHM_OK`；空指针/零长度/无效配置返回 `INVALID_ARGUMENT`，任一 raw 超 `adc_max_code` 返回 `OUT_OF_RANGE`。实现会先校验全部 raw，再写输出，因此越界时输出保持未写状态。

输入输出元素类型/宽度不同，不支持原地；raw 由采集层拥有，voltage 由应用拥有，可在成功后接 VPP/RMS/RemoveDC。模块无 workspace 和动态内存。

```text
ADC DMA DONE -> Process -> 检查 OK -> voltage_v[] -> Measurement/DSP
```

## 19. Realistic Example / Common Modification Tasks

```c
const signal_adc_to_voltage_config_t adc_scale = {
    .adc_max_code = 4095U,
    .reference_voltage_v = 3.3f,
    .input_scale = 2.0f,       /* 前端 1:2 分压，恢复到输入端 */
    .offset_voltage_v = 0.0f
};
status = SignalADCToVoltage_Process(
    SignalADC_GetBuffer(), voltage_v, SignalADC_GetSampleCount(), &adc_scale);
if (status == SIGNAL_ALGORITHM_OK) {
    status = SignalVPP_Process(voltage_v, SignalADC_GetSampleCount(), &vpp);
}
```

CONFIG ONLY：`adc_max_code`、VREF 数值、前端 scale/offset、N。SYSCONFIG REQUIRED：真实 ADC 分辨率、参考源、通道/pin；硬件变化后必须同步本 config。

## 20. Quick Modify Table

| 我想改什么 | 去哪里 | 改什么 | 影响 | SysConfig? |
|---|---|---|---|---|
| ADC 位数 | Application config | `adc_max_code` | code→V 比例/越界 | 硬件位数变化时是 |
| VREF | `.syscfg` + config | reference source、`reference_voltage_v` | 全部电压比例 | 是 |
| 前端分压/增益 | Application calibration | `input_scale` | 恢复输入端幅值 | 硬件本身不一定由SysConfig控制 |
| 零偏校正 | Application calibration | `offset_voltage_v` | 全部点平移 | 否 |
| N | buffer/call | `count` | voltage RAM=`4N`、CPU O(N) | 否 |

## API Reference

`SignalADCToVoltage_Process(raw_codes, voltage_v, count, config)`：同步转换一帧，成功返回 `SIGNAL_ALGORITHM_OK`。
