# comparator_threshold

## CCS SysConfig GUI Configuration

### Required resources

硬件落地需要一个 `COMP`。`COMP` 是 SysConfig module 名，P05 的 `COMP0` 是硬件 instance，`SIGNAL_COMP` 是工程实例名，`DL_COMP_*` 是 DriverLib C 枚举/函数前缀；四者不能混写。本 helper 本身只计算软件 config，只有把结果应用到片上比较器时才执行以下 GUI 步骤。

### Step 1 - Add and identify COMP

GUI Path: 左侧 `Add` -> `COMP` -> 在新建实例的 `Basic Configuration` 顶部设置 `Name`，然后选择 `Comparator Instance`（P05 基线为 COMP0）。工程实例名可设为 `SIGNAL_COMP`；硬件 instance 必须按当前资源视图选择，二者不要混写。

### Step 2 - Configure threshold, hysteresis, mode and filter

GUI Path: 左侧 `COMP` -> `SIGNAL_COMP` -> `Basic Configuration` -> `Reference/Threshold`（选择参考源与 DAC threshold）-> `Hysteresis` -> `Operating Mode` -> `Output Filter` -> `Output Polarity`。阈值按当前模拟量程换算；P05 的 VDDA/DAC、30 mV、ULP、filter delay 只是示例，不把 DriverLib 枚举或生成的 DAC code 当作 GUI 菜单名称。

### Step 3 - Input, PinMux and optional event

GUI Path: 左侧 `COMP` -> `SIGNAL_COMP` -> `Input Configuration` -> `Positive Input`/`Negative Input` -> `PinMux Peripheral and Pin Configuration` 选择实际模拟输入 Pin；若下游需要硬件事件，再进入 `Event Configuration` -> `Output Edge Publisher` -> `Publisher Channel ID`。纯阈值比较不新增 Timer、DMA 或 Event；P05 的 PA27/channel 4 仅为参考。

### Step 4 - Clock configuration

COMP 静态比较不需要独立时钟配置，也不要为它新增 Timer。若后级使用 Timer Capture，另在 `TIMER-CAPTURE` -> `Basic Configuration` -> `Clock Configuration` 设置计数时钟；COMP 只提供比较输出边沿。

### Expected generated symbols

P05 Generate 后可核对 `SIGNAL_COMP_INST`、`SIGNAL_COMP_DACCODE0`、`SIGNAL_COMP_INST_PUB_CH`、`GPIO_SIGNAL_COMP_IN0N_PIN` 和 IOMUX 宏；PROJECT_AUDIT 逐项记录 `GUI field -> .syscfg property -> generated symbol`。

保存后点击 SysConfig Generate，并在资源冲突视图确认 COMP 输入 Pin、可选 publisher channel 与其他模块不冲突。

### Final checklist / Common mistakes / Do not change

- 阈值源、阈值 code、输入极性、迟滞与滤波均与实际信号范围一致。
- 没有把 `DL_COMP_*` 或 `.syscfg` property 写成 GUI 字段，没有把 P05 的 PA27/channel 4/threshold 0 当成固定值。
- 不直接编辑 `.syscfg` 或 `ti_msp_dl_config.c/.h`；不为静态比较无故新增 Timer/Event/DMA。

## MSPM0G3507 比赛推荐方式

硬件 Comparator 配置优先在 SysConfig 完成，运行时动态调整用 `DL_COMP_*`。本模块只生成旧 comparator config struct，可作为兼容性 helper；它不应成为新工程的硬件入口。

## 1. 模块作用

生成任意阈值、迟滞与输出极性的比较器配置。

## 2. 输入

输入由公开头文件中的指针、长度、配置结构或平台回调给出；所有单位写在字段名中。

## 3. 输出

输出写入调用者提供的结果/缓冲区，并返回 `signal_result_t`；不通过隐藏全局变量传递数据。

## 4. 依赖

`signal_comparator.h`。

## 5. SysConfig 设置

新手详细配置：[Comparator 输入、参考、threshold、hysteresis、Event/IRQ 教程](../../00_docs/SYSCONFIG_BEGINNER_GUIDE.md#comparator)。本模块独有的是按真实量程选择阈值和输出极性；P05 的 DACCODE0=0 只是一份 Build 基线。现场速查见 [Quick Reference](../../00_docs/SYSCONFIG_QUICK_REFERENCE.md)。

通用代码不绑定 SysConfig 实例名；接到 MSPM0 时由独立平台适配器使用生成宏。当前状态不代表对应外设已实板验证。

## 6. 初始化方法

模块不做隐式全局初始化。包含 `signal_comparator_threshold.h`，由调用者准备配置、缓冲区或平台回调；如头文件提供 Init/Configure，先调用它。

## 7. 调用方法

`SignalComparatorThreshold_MakeConfig`、`SignalComparatorThreshold_GetModuleStatus`。

## 8. 参数修改方法

只修改调用者配置结构、count/capacity 和采样率等函数参数；不要为某个 Demo 改底层 `.c`。

## 9. 与其他模块如何连接

通过 `signal_types.h` 的数组+长度+采样率语义或本模块公开结构连接；先检查返回码再消费输出。

## 10. 最小示例

~~~c
#include "signal_comparator_threshold.h"

/* 按头文件准备输入/输出，调用上述主 API，并检查 signal_result_t。 */
~~~

纳入 `10_tests/pc` 全库构建；关键数值路径还应按题目范围补充向量和误差上限。

## 11. 常见错误

空指针、零长度、capacity 小于 count、单位混用、把配置采样率当物理实测值，以及复用仍在使用的工作区。

## 12. RAM 占用

模块内动态分配 0；数组/工作区由调用者提供，具体大小由 API 的 count/capacity 决定。

## 13. Flash 占用

无固定常量：取决于编译优化、是否链入数学库和死代码删除。已纳入整库链接检查；比赛应用以 CCS 生成的 .map 为最终数据。

## 14. CPU 计算量估计

函数为同步确定性处理；硬件回调的中断上下文只做最小状态更新，重计算放在主循环。

## 15. 当前验证状态

`MODULE_STATUS_BUILD_VERIFIED`。该状态只表示现有证据等级，不等于完整比赛场景已经验证。

## 16. 以后实板验证步骤

Hardware validation: PENDING。在 SysConfig 中按目标引脚/实例完成平台适配，用已知输入验证启停、边界和连续重启，记录变量与实测条件后才可升级 BOARD_VERIFIED。

不使用时，从工程移除本目录 .c 及上层引用；若有平台外设适配，再从 SysConfig 删除对应实例。

## 17. 统一 API 教程（已按当前头文件核对）

本节由当前公开头文件、实现中实际出现的状态码和正式模块注册表生成。它补充前文的场景教程；函数签名变化时必须重新运行 `tools/upgrade_formal_beginner_docs.ps1`，不要手工保留旧 API。

遵循仓库的 [Beginner README 标准](../../00_docs/BEGINNER_README_STANDARD.md)：先用最小示例完成一次正常数据流，再按需要阅读全功能示例和本节 API 细节。

### 什么时候用 / 什么时候不要用

当题目需要“comparator_threshold”目录对应的公开功能，并且输入数据、单位和硬件资源满足前文约束时使用本模块。若只需要更简单的上游功能、输入尚未准备好，或需要不同的数据模型/外设资源，应先选择相邻模块而不是强行调用本 API。

### 输入 / 输出

输入由各 API 的只读数组、配置、频率/阈值和平台对象组成；输出写入 result/output/buffer 参数或由 Get API 返回。调用者负责数组容量、生命周期和物理单位；失败返回时输出不是有效结果。

### 调用顺序

```text
SignalComparatorThreshold_GetModuleStatus -> SignalComparatorThreshold_MakeConfig
```

`Init` 一般仅一次；`Set/Configure` 仅在参数变化且模块空闲时执行；`Start/Process/Generate` 是每帧或每次任务入口；`Get/Is` 用于读取已完成的结果；`Stop` 只在需要取消时调用。若本模块没有其中某类 API，以实际列出的函数为准。

### SysConfig 边界

本模块需要 SysConfig。先在 CCS 的 .syscfg 添加并核对：COMP；再按前文的模块专用 GUI 步骤选择实际 pin/instance。保存后让 CCS 重新生成配置，核对生成宏；不要直接修改 	i_msp_dl_config.c/.h，也不要照抄示例 pin 或 DMA/Event 编号。

### 参数分级

- 【比赛必须会】输入/输出数组、`count/length/capacity`、采样率/频率、阈值/增益以及本 README 前文标出的 pin。它们直接影响题目范围、RAM、时间轴或物理单位。
- 【出问题再理解】Timer 时钟、DMA 通道、Event 路由、参考源和 IRQ。它们属于硬件链路，必须与 SysConfig 生成结果一致。
- 【以后进阶】多缓冲、运行时重配置、回调调度和 ISR 优化。先用最小示例完成一帧闭环，再处理吞吐或延迟。

### 常见错误 FAQ

- 参数错误：先检查指针非空、count/capacity 的单位是元素数、频率/阈值单位与上游一致。
- 硬件无结果：不要修改生成文件；回到 SysConfig 核对 pin、instance、时钟、Timer、DMA 和 Event 的完整链路。
- 结果异常：确认上一轮异步采集已经完成，真实 Fs/N/参考电压已传到算法，且没有在 DMA 使用期间改写 buffer。

### `signal_result_t SignalComparatorThreshold_MakeConfig(float threshold_v, float hysteresis_v, bool invert_output, signal_comparator_config_t *config);`

**它做什么：** 执行该模块公开的功能；具体数据流以本节参数表和本 README 前面的场景说明为准。

**什么时候调用：** 执行该模块公开的功能；具体数据流以本节参数表和本 README 前面的场景说明为准。

| 参数 | 类型 | 初学者解释 |
|---|---|---|
| `threshold_v` | `float` | 判定阈值；单位必须与输入数据一致。阈值越高通常越不易触发，但可能漏检弱信号。 |
| `hysteresis_v` | `float` | `hysteresis_v`（`float`）是该 API 的输入/输出参数；按本 README 前面的数据单位和边界条件准备。 |
| `invert_output` | `bool` | 由调用者分配的输出对象/数组。成功返回后才读取其中内容。 |
| `config` | `signal_comparator_config_t *` | 调用者填写的配置对象。先阅读该类型的成员；它控制本次初始化或处理方式。 |

**返回：** 返回 signal_result_t 类型结果；调用者应检查该值。

**最小调用形状：** `SignalComparatorThreshold_MakeConfig(...);`。可直接从 README_MINIMAL_EXAMPLE.c 复制正常流程；README_FULL_EXAMPLE.c 展示全部公开 API，其中取消类 API 会以 #if 0 隔离。

**注意：** 所有指针和数组都由调用者拥有；先检查返回值。异步采集、DMA 或回调还在使用 buffer 时，不能读取结果或改写该 buffer。

### `signal_module_status_t SignalComparatorThreshold_GetModuleStatus();`

**它做什么：** 读取模块当前的验证成熟度或静态状态，不会启动硬件操作。

**什么时候调用：** 读取模块当前的验证成熟度或静态状态，不会启动硬件操作。

**参数：** 无。

**返回：** 返回 signal_module_status_t 类型结果；调用者应检查该值。

**最小调用形状：** `SignalComparatorThreshold_GetModuleStatus(...);`。可直接从 README_MINIMAL_EXAMPLE.c 复制正常流程；README_FULL_EXAMPLE.c 展示全部公开 API，其中取消类 API 会以 #if 0 隔离。

**注意：** 所有指针和数组都由调用者拥有；先检查返回值。异步采集、DMA 或回调还在使用 buffer 时，不能读取结果或改写该 buffer。

### 示例、模块链与验收

- 最小入门：`README_MINIMAL_EXAMPLE.c`，只保留正常入口和结果读取。
- 全功能：`README_FULL_EXAMPLE.c`，以正确顺序展示当前头文件全部公开 API；`Stop` 等非常规路径不会默认执行。
- 模块链：先由上游提供单位、采样率和有效数据，再调用本模块；成功后将输出交给显示、控制、测量或下一步 DSP。硬件资源仍以 SysConfig 合约为唯一来源。
- 文档验收：README/API、两份示例和头文件会由 `tools/validate_beginner_documentation.ps1` 覆盖检查；这只表示文档与源码签名一致，不代替未进行的实板验证。

### 模块链

`上游采集/配置 -> 本模块 -> 检查返回值和结果 -> 测量、显示、控制或下一步 DSP`。只有确认本模块的输出单位和有效状态后，才交给下一模块。

## Hardware / Platform Binding

阈值配置通过 MSPM0G3507 Comparator 平台适配层写入真实 COMP instance。若要把比较边沿变成频率或周期结果，可参考 `09_examples/platform_closure/timer_capture_minimum`；先核对 comparator Event、Timer capture 和输入 Pin，再进行板测。

