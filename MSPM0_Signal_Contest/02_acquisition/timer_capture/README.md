# Timer Capture：硬件测频

> **当前 MSPM0G3507 平台入口说明（必须先看）**：正式驱动是
> `signal_timer_capture_mspm0g3507.c/.h`，公开接口为
> `SignalTimerCapture_MSPM0_Init/Start/Stop/GetResult`。本入口同时支持
> Combined Capture 和 Comparator→EVENT→Trigger Capture。Trigger Capture
> 的边沿在 `CC0_DN`，模块已经专门处理；只要 SysConfig 打开 `CC0_DN`、事件链路闭合，
> 不会再因为误用 `CC1_DN` 而一直得到 0。`GetResult()` 在收到两个同向边沿前返回
> `SIGNAL_RESULT_NO_DATA`，这是等待状态，不是测得的 0 Hz。

## CCS SysConfig GUI Configuration

### Required resources

需要 `TIMER-CAPTURE`、`COMP` 和 `EVENT`。`TIMER-CAPTURE`/`COMP` 是 SysConfig module，`TIMG6`/`COMP0` 是硬件 instance，`DL_TimerG_*`/`DL_COMP_*` 是 DriverLib C 名称。

### Step 1 - Capture timer and verified clock

GUI Path: `Add` -> `TIMER-CAPTURE` -> instance `SIGNAL_CAPTURE` -> `Basic Configuration`。

Action: 在同一实例的 `Basic Configuration` -> `Clock Configuration` 设置 `Timer Clock Source`、`Clock Divider`、`Clock Prescaler`、`Timer Mode` 和 `Desired Timer Period`；在 `Capture Configuration` -> `Capture Source` 选择 `Trigger`。P05 基线为 TIMG6、BUSCLK、divider/prescaler 1、2 ms、Trigger；这些值仅作参考，GUI 中应以右侧 `Calculated Timer Clock`、`Actual Timer Period` 为准。

### Step 2 - Clock Configuration and low-frequency boundary

共享教材：[MSPM0G3507 SysConfig 时钟、Timer、ADC 与 DAC 保姆教程](../../00_docs/MSPM0_SYSCONFIG_CLOCK_TIMER_ADC_DAC_BEGINNER_GUIDE.md)。Capture 应使用 GUI 显示的实际 Timer counter clock 与 `LOAD+1` 计算周期；P05 的 BUSCLK/2 ms 仅覆盖相邻边沿不发生多次回绕的频率范围。10 Hz 的 LFCLK/LFXT 方案尚未由 P05 profile 或 GUI 截图确认，不能在本模块宣称为已验证设置。

### Step 3 - Comparator and Event

GUI Path: `Add` -> `COMP` -> instance `SIGNAL_COMP`；`Event Configuration` -> Comparator output publisher。

Action: 左侧 `COMP` -> 实例 -> `Input Configuration` 设置正/负输入与参考/阈值；进入 `Event Configuration` -> `Output Edge Publisher` -> `Publisher Channel ID`。左侧 `TIMER-CAPTURE` -> 实例 -> `Event Configuration` -> `Subscriber Port Selection`、`Event Subscriber Channel ID`，再在 `Capture Configuration` -> `Capture Source = Trigger`。P05 的 COMP0/PA27/channel 4/OUTPUT_EDGE 仅作参考，实际 channel 必须同号闭合。

### Expected generated symbols

Generate 后核对 `SIGNAL_CAPTURE_INST`、`SIGNAL_CAPTURE_INST_LOAD_VALUE`、`SIGNAL_CAPTURE_INST_INT_IRQN`、Capture subscriber/event 宏以及 Comparator instance/input/event 宏。PROJECT_AUDIT 记录 `GUI field -> .syscfg property -> generated symbol`。

保存后点击 Generate，核对 `SIGNAL_CAPTURE_INST_LOAD_VALUE`、IRQ、Capture subscriber 与 Comparator publisher 宏，并在资源冲突视图确认 Timer/COMP/Event 无冲突。

### Final checklist / Common mistakes / Do not change

- Capture source、subscriber port/channel、Comparator publisher channel 完整闭合。
- `timer_hz` 使用 GUI 实际计数频率，不能直接假定 `CPUCLK_FREQ`。
- 不把 ZERO interrupt 当作自动的多回绕扩展；不直接编辑 `.syscfg` 或生成文件。

## 0. 什么时候用

当输入能先变成稳定数字边沿，且你要低 CPU、高精度测周期/频率时使用。24_C 用它为周期波形截取 3 周期时间轴，范围约 95 Hz~10.5 kHz；复合信号的 H1/频谱时间轴仍以 FFT 基波为准。示例链路是 `模拟信号 -> Comparator -> Event -> Timer Capture`。任意波形、弱信号或严重噪声下，先解决比较器阈值/整形。

## 1. 30 秒接入路线

你需要复制：`signal_timer_capture_mspm0g3507.c`、`signal_timer_capture_mspm0g3507.h`、`01_bsp/common/signal_status.h`。本模块【需要 SysConfig】。

按第 3 节在 CCS 的 SysConfig 图形界面配置 Comparator 与 Capture，复制 MSPM0G3507 比赛入口，粘贴变量/初始化/循环代码，只改 Timer 真实时钟、超时和捕获次数，然后 Build。不要直接编辑 `.syscfg` 文本，也不要修改生成的 `ti_msp_dl_config.c/.h`。

## 2. 输入和输出

- 输入：同方向边沿对应的 Timer capture 值。
- 输出：`float frequency_hz`、平均周期 ticks。
- 至少捕获 2 个时间戳；更多周期可降低单次量化抖动，但会增加刷新时间。

### 2.1 Trigger Capture 与 Combined Capture 的驱动差异

本 MSPM0G3507 入口同时兼容两种合法 SysConfig 链路：

- `Combined Capture`（外部 CCP 引脚）：驱动使用 `CC1_DN`，从 CC1/CC0 组合捕获值计算周期和高电平时间；
- `Trigger Capture`（Comparator 输出事件经 EVENT 订阅）：每个事件写入 `CC0`，驱动按相邻 CC0 捕获值的模计数差计算周期，`duty_percent` 在该模式返回 0。

因此，使用 Comparator→EVENT→TIMER 的工程必须打开 `CC0_DN`；不要把 Trigger Capture 配置成只开 CC1_DN，否则不会得到频率结果。`timer_clock_hz` 和 `load_value` 仍必须取 SysConfig 生成的实际计数时钟与 `LOAD+1`。

## 3. SysConfig / Pin

【需要 SysConfig】。`PROFILE_05_FREQUENCY` 只作为配置项和资源名称参考；实际操作在 CCS 中双击工程的 `.syscfg`，用 SysConfig 图形界面完成：

1. 在左侧外设列表添加 `TIMER-CAPTURE`，实例名设为 `SIGNAL_CAPTURE`；在图形页选择 Timer instance、Clock Source、Clock Divider/Prescaler 和 Timer Period。不要把 DriverLib 的 `TimerG` 名称当作 SysConfig module 名。
2. Capture Source 选 `Trigger`，订阅 Event；打开 `CC0_DN` 和 `ZERO` interrupts。
3. 添加 `COMP`，实例名设为 `SIGNAL_COMP`；把 Comparator Output Edge 发布到 Capture 使用的同一 Event Channel。
4. 示例模拟输入使用 `COMP0` 的 `PA27`，阈值来自内部 VDDA DAC；按题目电平在图形页调整阈值、迟滞、滤波、边沿和输入极性。
5. 保存 `.syscfg`，让 CCS 自动重新生成配置；只检查生成的 `SIGNAL_CAPTURE_INST`、`SIGNAL_CAPTURE_INST_INT_IRQN`、`SIGNAL_CAPTURE_INST_LOAD_VALUE` 等宏，不手改生成文件。
6. 在 SysConfig 的资源冲突视图检查 Timer、Event channel、COMP0 和 PA27 没有冲突，再 Clean/Build。

换输入 Pin 时在 CCS SysConfig 图形页选择 MSPM0G3507 对应 Comparator 合法 Pin；模块 `.c` 不写死 PA27。若用外部比较器，也可让外部数字边沿进入合法 Capture 路径，但实例名和事件/引脚配置仍要满足生成宏。

### 3.1 10 Hz 配置边界（尚未完成 GUI 验证）

10 Hz 的周期是 100 ms；P05 的 `BUSCLK + 2 ms period` 会在相邻边沿之间发生多次回绕。需要低频测量时，打开左侧 `SYSCTL` -> `Clock Tree`，再回到 `TIMER-CAPTURE` -> `Basic Configuration` -> `Clock Configuration`，从 `Clock Source` 下拉框选择当前器件实际提供的低频源，观察 `Calculated Timer Clock` 和 `Actual Timer Period` 后再回算无歧义 capture 间隔；不要直接照抄某个示例的 LFCLK 数字。

完成低频配置后，以 GUI 显示的 `Calculated Timer Clock` 和生成的 `SIGNAL_CAPTURE_INST_LOAD_VALUE` 回算 `SIGNAL_CAPTURE_TIMER_HZ`，再写入应用参数。

截图和成功 `.syscfg` 到位后，再按 `实际 Timer counter clock -> period/load -> 最大无歧义 capture 间隔` 回算 10 Hz 配置，并用生成的 `SIGNAL_CAPTURE_INST_LOAD_VALUE` 核对；在此之前不得写死 `SIGNAL_CAPTURE_TIMER_HZ`。

## 4. 复制哪些文件

从本目录复制：

- `signal_timer_capture_mspm0g3507.c`
- `signal_timer_capture_mspm0g3507.h`

再复制 `01_bsp/common/signal_status.h`，全部放到 `modules/`。不要复制旧 Platform、Adapter 或纯数学版 `signal_timer_capture.c/h`。

## 5. main.c 顶部复制什么

```c
#include <stddef.h>
#include <stdint.h>
#include "ti_msp_dl_config.h"
#include "signal_timer_capture_mspm0g3507.h"

// ===== 你需要根据刷新速度/稳定性修改 =====
#define SIGNAL_CAPTURE_COUNT  (8U)

// 必须等于 SysConfig 图形页显示的 Timer 实际计数时钟。
// P05 默认 BUSCLK/1：仅当 GUI 实际计数时钟等于 CPUCLK 时才能这样写。
#define SIGNAL_CAPTURE_TIMER_HZ  (CPUCLK_FREQ)
#define SIGNAL_CAPTURE_TIMEOUT_OVERFLOWS  (100U)

// ===== 一般不用改 =====
static volatile uint32_t g_isr_ticks[SIGNAL_CAPTURE_COUNT];
static uint32_t g_ticks[SIGNAL_CAPTURE_COUNT];
volatile float g_frequency_hz;
volatile signal_result_t g_capture_status;
```

## 6. 比赛参数

| 题目变化 | 修改 |
|---|---|
| 要更快刷新 | 减小 `SIGNAL_CAPTURE_COUNT` |
| 要平均更多周期 | 增大 `SIGNAL_CAPTURE_COUNT` |
| 最低频率更低 | 在 CCS SysConfig 图形页降低 Timer 时钟并增大周期；`timeout_overflows` 只延长等待时间，不能修复多次回绕 |
| Timer 时钟/分频变化 | `timer_hz` 必须写真实计数频率 |
| 输入噪声/幅值变化 | SysConfig 中 Comparator threshold/hysteresis/filter |
| 换输入脚 | SysConfig 的 Comparator/Capture Pin |

### 6.1 可测频率边界必须先算

设 Timer 实际计数频率为 `Ftimer`，模数为 `M = LOAD + 1`，Timer 周期为 `Ttimer = M/Ftimer`。当前实现只保存每次捕获在当前 Timer 周期内的位置，最多只能处理相邻边沿之间的一次边界回绕，因此必须满足：

```text
输入周期 Tinput < Ttimer
最低输入频率 fmin > Ftimer / M
```

默认 `32 MHz / 64000` 的边界是 500 Hz；10 Hz 不满足。`timeout_overflows` 只是“无足够边沿时等多久再结束”，ZERO ISR 没有把 overflow 数拼入时间戳。需要同时覆盖极低频和很高频时，应切换快/慢两套 Timer 配置，或另行实现带回绕竞争处理的 64-bit 扩展时间戳。

## 7. 初始化区复制什么

```c
const signal_timer_capture_mspm0_config_t capture_config = {
    SIGNAL_CAPTURE_TIMER_HZ,             // SysConfig 显示的 Timer 真正计数频率
    SIGNAL_CAPTURE_INST_LOAD_VALUE + 1U,
    SIGNAL_CAPTURE_TIMEOUT_OVERFLOWS     // 总超时约为该值 * Timer period
};
size_t copied;
float mean_ticks;
float frequency_hz;

SYSCFG_DL_init();
g_capture_status = SignalTimerCapture_MSPM0_Init(
    g_isr_ticks, SIGNAL_CAPTURE_COUNT, &capture_config);
if (g_capture_status != SIGNAL_RESULT_OK) {
    while (1) { }
}
```

## 8. while(1) 中复制什么

```c
g_capture_status = SignalTimerCapture_MSPM0_Start();
while (!SignalTimerCapture_MSPM0_IsFinished()) {
    __WFI();
}

g_capture_status = SignalTimerCapture_MSPM0_Copy(
    g_ticks, SIGNAL_CAPTURE_COUNT, &copied);
if (g_capture_status == SIGNAL_RESULT_OK) {
    g_capture_status = SignalTimerCapture_MSPM0_CalculateFrequency(
        g_ticks, copied, &mean_ticks, &frequency_hz);
    if (g_capture_status == SIGNAL_RESULT_OK) {
        g_frequency_hz = frequency_hz;
    }
}

// ===== 这里写你自己的逻辑：显示/判断 g_frequency_hz =====
```

完整代码见 `README_MINIMAL_EXAMPLE.c`。

## 9. 结果和下一步

`g_frequency_hz` 单位 Hz。它通常直接送显示、串口或控制逻辑，不需要 ADC To Voltage/FFT。若还要幅值、THD 或波形，另走 ADC DMA 链。

## 10. Build 与最小验证

先输入频率已知、边沿干净的方波；检查 `copied`、ticks 差值和 `g_frequency_hz`。再接 Comparator 模拟输入。隔离 COPY TEST 为 `SysConfig / Compile / Full Link PASS`，Flash 2368 B、SRAM（含栈）771 B；本轮未上板，状态 `BUILD_VERIFIED`。

## 11. 常见错误

- 一直等待：Comparator 没有输出边沿，或 Event subscriber/channel 不一致。
- 结果约差固定倍数：`timer_hz` 填错或只捕获到单边/双边中的另一种。
- 低频结果错误或返回 `NO_DATA`：输入周期不小于 Timer 周期；增大 `timeout_overflows` 无效，按 3.1 节在 CCS 图形界面改慢时钟/长周期。
- 频率固定差很多倍：应用填写的 `timer_hz` 不是 SysConfig 图形页显示的真实 Timer clock，或 `counter_modulus` 没有使用生成的 `LOAD+1`。
- 频率符号/相位方向异常：检查 Comparator 输入极性与捕获边沿。

## 12. API Reference

- `SignalTimerCapture_MSPM0_Init(buffer, capacity, config)`
- `SignalTimerCapture_MSPM0_Start()` / `SignalTimerCapture_MSPM0_Stop()`
- `SignalTimerCapture_MSPM0_IsFinished()` / `SignalTimerCapture_MSPM0_GetCount()`
- `SignalTimerCapture_MSPM0_Copy(destination, capacity, copied)`
- `SignalTimerCapture_MSPM0_CalculateFrequency(timestamps, count, mean_ticks, frequency_hz)`
- `SignalTimerCapture_MSPM0_GetModuleMaturity()`

## 13. 24_C 成功案例：硬件频率用于周期时间轴

本模块接收外部整形后的稳定数字边沿，用 Timer Capture 低 CPU 地测周期频率。24_C 应用把有效范围放宽为约 `95 Hz~10.5 kHz`，并在启动时用 SysConfig 生成的 Timer `LOAD+1` 和实际计数时钟复核 `timer_hz`，不能只填 `CPUCLK_FREQ`。

```text
周期正弦/锯齿/脉冲：hardware_frequency -> 3 周期波形窗口的 X 轴
复合信号：FFT 插值得到 fundamental_frequency -> 频谱/波形 X 轴
```

复合信号不能让硬件边沿频率覆盖 FFT 基波，否则整形方波的边沿可能代表脉冲重复率而不是复合模拟波形的基波。占空比需要在同一组捕获边沿上另外记录高电平宽度与周期宽度；本模块公开 API 主要返回周期/频率，应用层负责组合成 `duty = high_ticks / period_ticks`。

比较器输出必须接到 Capture 使用的 Event channel，并确认极性、迟滞和滤波。相邻边沿之间不能发生未处理的多次 Timer 回绕；低频超出当前 `LOAD+1` 窗口时，应改慢 Timer 或实现带回绕扩展的时间戳，单纯增大超时无效。频率稳定但固定差倍数时，优先检查实际 Timer clock、捕获边沿和 `counter_modulus=LOAD+1`。

## 17. 统一 API 教程（已按当前头文件核对）

本节由当前公开头文件、实现中实际出现的状态码和正式模块注册表生成。它补充前文的场景教程；函数签名变化时必须重新运行 `tools/upgrade_formal_beginner_docs.ps1`，不要手工保留旧 API。

遵循仓库的 [Beginner README 标准](../../00_docs/BEGINNER_README_STANDARD.md)：先用最小示例完成一次正常数据流，再按需要阅读全功能示例和本节 API 细节。

### 什么时候用 / 什么时候不要用

当题目需要“timer_capture”目录对应的公开功能，并且输入数据、单位和硬件资源满足前文约束时使用本模块。若只需要更简单的上游功能、输入尚未准备好，或需要不同的数据模型/外设资源，应先选择相邻模块而不是强行调用本 API。

### 输入 / 输出

输入由各 API 的只读数组、配置、频率/阈值和平台对象组成；输出写入 result/output/buffer 参数或由 Get API 返回。调用者负责数组容量、生命周期和物理单位；失败返回时输出不是有效结果。

### 调用顺序

```text
SignalTimerCapture_GetModuleStatus -> SignalTimerCapture_Delta -> SignalTimerCapture_MeanPeriod
```

`Init` 一般仅一次；`Set/Configure` 仅在参数变化且模块空闲时执行；`Start/Process/Generate` 是每帧或每次任务入口；`Get/Is` 用于读取已完成的结果；`Stop` 只在需要取消时调用。若本模块没有其中某类 API，以实际列出的函数为准。

### SysConfig 边界

本模块需要 SysConfig。先在 CCS 的 .syscfg 添加并核对：TIMER；再按前文的模块专用 GUI 步骤选择实际 pin/instance。保存后让 CCS 重新生成配置，核对生成宏；不要直接修改 	i_msp_dl_config.c/.h，也不要照抄示例 pin 或 DMA/Event 编号。

### 参数分级

- 【比赛必须会】输入/输出数组、`count/length/capacity`、采样率/频率、阈值/增益以及本 README 前文标出的 pin。它们直接影响题目范围、RAM、时间轴或物理单位。
- 【出问题再理解】Timer 时钟、DMA 通道、Event 路由、参考源和 IRQ。它们属于硬件链路，必须与 SysConfig 生成结果一致。
- 【以后进阶】多缓冲、运行时重配置、回调调度和 ISR 优化。先用最小示例完成一帧闭环，再处理吞吐或延迟。

### 常见错误 FAQ

- 参数错误：先检查指针非空、count/capacity 的单位是元素数、频率/阈值单位与上游一致。
- 硬件无结果：不要修改生成文件；回到 SysConfig 核对 pin、instance、时钟、Timer、DMA 和 Event 的完整链路。
- 结果异常：确认上一轮异步采集已经完成，真实 Fs/N/参考电压已传到算法，且没有在 DMA 使用期间改写 buffer。

### `signal_result_t SignalTimerCapture_Delta(uint32_t earlier, uint32_t later, uint32_t counter_modulus, uint32_t *delta_ticks);`

**它做什么：** 执行该模块公开的功能；具体数据流以本节参数表和本 README 前面的场景说明为准。

**什么时候调用：** 执行该模块公开的功能；具体数据流以本节参数表和本 README 前面的场景说明为准。

| 参数 | 类型 | 初学者解释 |
|---|---|---|
| `earlier` | `uint32_t` | `earlier`（`uint32_t`）是该 API 的输入/输出参数；按本 README 前面的数据单位和边界条件准备。 |
| `later` | `uint32_t` | `later`（`uint32_t`）是该 API 的输入/输出参数；按本 README 前面的数据单位和边界条件准备。 |
| `counter_modulus` | `uint32_t` | 元素数量或容量，单位是“元素个数”而不是字节；必须与实际数组大小一致。 |
| `delta_ticks` | `uint32_t *` | `delta_ticks`（`uint32_t `）是该 API 的输入/输出参数；按本 README 前面的数据单位和边界条件准备。 |

**返回：** 返回 signal_result_t 类型结果；调用者应检查该值。

**最小调用形状：** `SignalTimerCapture_Delta(...);`。可直接从 README_MINIMAL_EXAMPLE.c 复制正常流程；README_FULL_EXAMPLE.c 展示全部公开 API，其中取消类 API 会以 #if 0 隔离。

**注意：** 所有指针和数组都由调用者拥有；先检查返回值。异步采集、DMA 或回调还在使用 buffer 时，不能读取结果或改写该 buffer。

### `signal_result_t SignalTimerCapture_MeanPeriod(const uint32_t *timestamps, size_t timestamp_count, const signal_timer_capture_config_t *config, float *mean_ticks, float *frequency_hz);`

**它做什么：** 执行该模块公开的功能；具体数据流以本节参数表和本 README 前面的场景说明为准。

**什么时候调用：** 执行该模块公开的功能；具体数据流以本节参数表和本 README 前面的场景说明为准。

| 参数 | 类型 | 初学者解释 |
|---|---|---|
| `timestamps` | `const uint32_t *` | `timestamps`（`const uint32_t `）是该 API 的输入/输出参数；按本 README 前面的数据单位和边界条件准备。 |
| `timestamp_count` | `size_t` | 元素数量或容量，单位是“元素个数”而不是字节；必须与实际数组大小一致。 |
| `config` | `const signal_timer_capture_config_t *` | 调用者填写的配置对象。先阅读该类型的成员；它控制本次初始化或处理方式。 |
| `mean_ticks` | `float *` | `mean_ticks`（`float `）是该 API 的输入/输出参数；按本 README 前面的数据单位和边界条件准备。 |
| `frequency_hz` | `float *` | 频率/速率参数，单位 Hz。必须传入实际配置或测得的上游数值，不能把 ADC 时钟名称直接当采样率。 |

**返回：** 返回 signal_result_t 类型结果；调用者应检查该值。

**最小调用形状：** `SignalTimerCapture_MeanPeriod(...);`。可直接从 README_MINIMAL_EXAMPLE.c 复制正常流程；README_FULL_EXAMPLE.c 展示全部公开 API，其中取消类 API 会以 #if 0 隔离。

**注意：** 所有指针和数组都由调用者拥有；先检查返回值。异步采集、DMA 或回调还在使用 buffer 时，不能读取结果或改写该 buffer。

### `signal_module_status_t SignalTimerCapture_GetModuleStatus();`

**它做什么：** 读取模块当前的验证成熟度或静态状态，不会启动硬件操作。

**什么时候调用：** 读取模块当前的验证成熟度或静态状态，不会启动硬件操作。

**参数：** 无。

**返回：** 返回 signal_module_status_t 类型结果；调用者应检查该值。

**最小调用形状：** `SignalTimerCapture_GetModuleStatus(...);`。可直接从 README_MINIMAL_EXAMPLE.c 复制正常流程；README_FULL_EXAMPLE.c 展示全部公开 API，其中取消类 API 会以 #if 0 隔离。

**注意：** 所有指针和数组都由调用者拥有；先检查返回值。异步采集、DMA 或回调还在使用 buffer 时，不能读取结果或改写该 buffer。

### 示例、模块链与验收

- 最小入门：`README_MINIMAL_EXAMPLE.c`，只保留正常入口和结果读取。
- 全功能：`README_FULL_EXAMPLE.c`，以正确顺序展示当前头文件全部公开 API；`Stop` 等非常规路径不会默认执行。
- 模块链：先由上游提供单位、采样率和有效数据，再调用本模块；成功后将输出交给显示、控制、测量或下一步 DSP。硬件资源仍以 SysConfig 合约为唯一来源。
- 文档验收：README/API、两份示例和头文件会由 `tools/validate_beginner_documentation.ps1` 覆盖检查；这只表示文档与源码签名一致，不代替未进行的实板验证。

### 模块链

`上游采集/配置 -> 本模块 -> 检查返回值和结果 -> 测量、显示、控制或下一步 DSP`。只有确认本模块的输出单位和有效状态后，才交给下一模块。

## 18. 联合捕获结果接口（24_C 已收录接口）

若 SysConfig 已把 `TIMG6` 配为 Combined Capture，外部整形后的数字信号接到
`CCP0`，可直接使用当前 MSPM0G3507 平台接口读取周期和高电平宽度：

```c
signal_timer_capture_mspm0_result_t result;

SignalTimerCapture_MSPM0_Init(&capture_config);
SignalTimerCapture_MSPM0_Start();

if (SignalTimerCapture_MSPM0_GetResult(&result) == SIGNAL_RESULT_OK &&
    result.valid) {
    /* result.frequency_hz: Hz；result.duty_percent: % */
    DisplayFrequencyAndDuty(result.frequency_hz, result.duty_percent);
}
```

`capture_config.timer_clock_hz` 必须填写 SysConfig GUI 显示的实际计数时钟，
`capture_config.load_value` 必须使用生成的 `SIGNAL_CAPTURE_INST_LOAD_VALUE`。
驱动在 Capture ISR 中只保存 `period_ticks`、`high_ticks` 和无锁结果序号；
`GetResult` 在主循环中复制稳定快照并计算 `frequency_hz = timer_clock_hz / period_ticks`
和 `duty_percent = 100 * high_ticks / period_ticks`。它不新增 SysConfig 资源，但必须按
本 README 前文的 Capture pin、时钟、Combined mode 和中断配置完成闭环。

