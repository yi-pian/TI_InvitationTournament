# THD：总谐波失真

> **LEVEL C / REAL ALGORITHM MODULE：** 公式本身短，但输入必须是同一规则得到的 H1 与 H2+ 能量；保留模块是为了固定能量/幅值和 ratio/percent 契约。

## 比赛复制版：先看这里

**适合：** Harmonic 已经给出含 H1 和 H2+ 的能量结果，现在计算 `sqrt(sum(H2..Hm energy)/H1 energy)`。

**复制到 `modules/`：** `signal_thd.c`、`signal_thd.h`、`04_dsp/harmonic/signal_harmonic.h` 和 `03_measurement/common/signal_algorithm_status.h`。THD 只消费 harmonic result，因此单独 COPY TEST 不需要 `signal_harmonic.c`；完整测量链仍要按 Harmonic README 复制其实现和 MultiBinEnergy。

```c
#include "signal_thd.h"

signal_thd_result_t thd;
signal_algorithm_status_t status = SignalTHD_Process(&harmonics, &thd);
if (status == SIGNAL_ALGORITHM_OK) {
    float thd_percent = thd.thd_percent;
    // ===== 这里写你自己的逻辑：显示/判限 thd_percent =====
}
```

**输入 / 输出：** `signal_harmonic_result_t` -> 基波能量、谐波总能量、THD ratio 与 THD percent。输入必须包含 H1 和至少一个 H2 以上结果，且 H1 energy > 0。

| 题目变化 | 修改 |
|---|---|
| THD 统计到 H5 | 在上游 Harmonic 设置 `last_order=5` |
| 窗函数/radius 改变 | 所有阶次必须使用同一 FFT/窗口/半径规则 |
| 要 THD+N | 本模块不包含噪声，不能直接冒充 THD+N |

**Build / 最小验证：** H1 energy=100、H2=4、H3=1 时 THD=`sqrt(5/100)`。隔离复制工程已 `SysConfig / Compile / Full Link PASS`，Flash 2488 B、SRAM（含栈）533 B。完整代码见 `README_MINIMAL_EXAMPLE.c`。

**连接：** `Harmonic -> THD -> thd.thd_percent`。常见错误是把 amplitude 当 energy、漏 H1、把 ratio 当 percent 或各阶积分规则不一致。

> 下文保留公式和详细 API；比赛 COPY 以本节为准。

## 你真的需要这个模块吗？

**已有 Harmonic 结果，并且要计算总谐波失真时需要。** 这是 C `ALGORITHM_MODULE`，只处理内存数据。

## 你应该已经有什么输入数据

包含 H1 和至少一个 H2 以上结果的 `signal_harmonic_result_t`。

## 最短接入步骤

1. **文件：** 复制顶部清单到 `modules/`，include `signal_thd.h`；完整链另按 Harmonic README 复制其实现。
2. **参数：** 无硬件参数；谐波阶数和积分半径已经体现在上游 Harmonic 结果中。
3. **Workspace / Result：** 准备 `signal_thd_result_t result`。
4. **调用：** `SignalTHD_Process(&harmonics, &result)`。
5. **输出：** 基波能量、谐波能量和、THD ratio 与 `thd_percent`。
6. **连接下一步：** 结果显示、判限或保存。
7. **Build / 最小验证：** 人工给定 H1/H2 energy，按 `sqrt(sum(H2..)/H1)` 手算并核对百分比。

> 算法边界：不配置 Pin，不修改 SysConfig，不调用 DriverLib，也不需要 Platform Adapter。上游硬件变化时，只把真实 `Fs/N/VREF` 等事实同步到算法参数。

## 1 这个算法是干什么的？

把 2 次及以上谐波的 RMS/能量合并，再与基波相比，输出百分数。

## 2 一个最简单的例子

A1=1、A2=0.1，无其他谐波，则 THD=0.1=10%。

## 3 原理

`THD=sqrt((E2+...+Em)/E1)`。能量平方相加而不是幅值直接相加，因为不同频率正交。相同 FFT 标度/窗/积分规则在比值中相消。

## 4 比赛里什么时候用？

放大器、滤波器、信号源非线性质量评估。

## 5 输入

必须含 H1 和 H2 以上的 Harmonic result。

## 6 输出

fundamental_energy、harmonic_energy_sum、`thd_ratio`、`thd_percent`（1.0表示1%）。

## 7 API怎么调用

`SignalTHD_Process(&harmonics,&thd);`

## 8 参数怎么改

THD 本身无参数；在 Harmonic 中改 last_order/radius，并把定义写入结果报告，例如“THD2-5”。

## 9 参数改大会怎样

纳入更多阶通常 THD 不减，但高阶接近噪声/带宽边界会使不确定度增加。

## 10 这个算法的代价是什么

Benefits：标准能量定义。Trade-offs：不含噪声/非谐波；前端衰减高阶会让 THD 虚低。

## 11 什么时候不要用

前面加了会滤掉 H2~H5 的低通时；这会改变被测对象。信号削顶/混叠也不能仅信一个 THD 数。

## 12 怎么和前一个模块接

`RemoveDC -> Hann -> FFT -> MultiBin -> Harmonic -> THD`

## 13 怎么和后一个模块接

`THD percent -> display/limit decision`。

## 14 最小Demo

```c
signal_thd_result_t t;
(void)SignalTHD_Process(&h,&t);
```

## 15 PC测试

理论11.1803408%；BASIC=11.1803312%，COMPETITION=11.1817274%，均按声明容差 PASS。

排查：异常低查低通/前端带宽；异常高查泄漏、radius、噪声和谐波窗口；必须报告包含到几阶。

## 16 MCU资源

O(H)，O(1)，一次 sqrtf。

## 17 验证状态

PC_VERIFIED；未做板端模拟前端频响校准。

## 17. 统一 API 教程（已按当前头文件核对）

本节由当前公开头文件、实现中实际出现的状态码和正式模块注册表生成。它补充前文的场景教程；函数签名变化时必须重新运行 `tools/upgrade_formal_beginner_docs.ps1`，不要手工保留旧 API。

遵循仓库的 [Beginner README 标准](../../00_docs/BEGINNER_README_STANDARD.md)：先用最小示例完成一次正常数据流，再按需要阅读全功能示例和本节 API 细节。

### 什么时候用 / 什么时候不要用

当题目需要“thd”目录对应的公开功能，并且输入数据、单位和硬件资源满足前文约束时使用本模块。若只需要更简单的上游功能、输入尚未准备好，或需要不同的数据模型/外设资源，应先选择相邻模块而不是强行调用本 API。

### 输入 / 输出

输入由各 API 的只读数组、配置、频率/阈值和平台对象组成；输出写入 result/output/buffer 参数或由 Get API 返回。调用者负责数组容量、生命周期和物理单位；失败返回时输出不是有效结果。

### 调用顺序

```text
SignalTHD_Process
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

### `signal_algorithm_status_t SignalTHD_Process(const signal_harmonic_result_t *harmonics, signal_thd_result_t *result);`

**它做什么：** 根据 Harmonic 结果计算 sqrt(sum(H2..Hm energy)/H1 energy)。

**什么时候调用：** 对调用者提供的数据执行一次同步计算或生成，并在成功后写入输出对象/数组。

| 参数 | 类型 | 初学者解释 |
|---|---|---|
| `harmonics` | `const signal_harmonic_result_t *` | 必须包含 1 阶和至少一个 2 阶以上结果。 |
| `result` | `signal_thd_result_t *` | 输出基波/谐波能量、THD 比值和百分数。 |

**返回：** 成功返回 SIGNAL_ALGORITHM_OK；缺基波或基波能量为零返回错误。

**最小调用形状：** `SignalTHD_Process(...);`。可直接从 README_MINIMAL_EXAMPLE.c 复制正常流程；README_FULL_EXAMPLE.c 展示全部公开 API，其中取消类 API 会以 #if 0 隔离。

**注意：** 所有指针和数组都由调用者拥有；先检查返回值。异步采集、DMA 或回调还在使用 buffer 时，不能读取结果或改写该 buffer。

### 示例、模块链与验收

- 最小入门：`README_MINIMAL_EXAMPLE.c`，只保留正常入口和结果读取。
- 全功能：`README_FULL_EXAMPLE.c`，以正确顺序展示当前头文件全部公开 API；`Stop` 等非常规路径不会默认执行。
- 模块链：先由上游提供单位、采样率和有效数据，再调用本模块；成功后将输出交给显示、控制、测量或下一步 DSP。硬件资源仍以 SysConfig 合约为唯一来源。
- 文档验收：README/API、两份示例和头文件会由 `tools/validate_beginner_documentation.ps1` 覆盖检查；这只表示文档与源码签名一致，不代替未进行的实板验证。

### 模块链

`上游采集/配置 -> 本模块 -> 检查返回值和结果 -> 测量、显示、控制或下一步 DSP`。只有确认本模块的输出单位和有效状态后，才交给下一模块。

