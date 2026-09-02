# trigger_capture

## CCS SysConfig GUI Configuration

### Required resources

`KNOWLEDGE_MISMATCH`：Canonical Registry/Module Card 当前把本模块登记为 `sysconfig.class = required`、资源 `TIMER`，但源码/API 只扫描调用者已经采好的 `uint16_t[]` 并复制片段，不访问 Timer、ADC、DMA、Event 或 DriverLib。不能根据错误合同添加 TIMER，也不能为本算法写虚构 GUI 步骤。

### Step 1 - Stop before SysConfig changes

GUI Path: Not Applicable。Action: 只在 Application 中设置 `level`、`hysteresis`、`edge`、`search_start`、`pretrigger_count` 和 buffer 长度。Value: 全部是软件参数/ADC code/sample count，不是 SysConfig field。

### Step 2 - Configure upstream acquisition separately

若 raw buffer 来自硬件 ADC，读取实际选择的 `adc_basic`、`adc_dma`、`adc_pingpong_dma` 或 `adc_timer_trigger` GUI 教程并配置它们；若要比较器边沿测频，使用 `timer_capture`/`comparator_zero_cross`。不要把这些上游资源归到本 `trigger_capture` 算法。

### Step 3 - Clock configuration

本模块没有硬件时钟，Clock: Not Applicable。raw buffer 的时间轴由上游实际 `Fs` 决定：定时 ADC 通常核对 `Timer source/divider/prescaler -> Timer event -> ADC Fs`，ADC functional clock只决定转换预算。算法的 `trigger_index` 只有结合已验证 `Fs` 才能换算时间。详见[共享时钟教材](../../00_docs/MSPM0_SYSCONFIG_CLOCK_TIMER_ADC_DAC_BEGINNER_GUIDE.md)。

### Expected generated symbols

本模块期望生成符号：None。任何 ADC/Timer/DMA/Event symbol 都属于上游采集模块。PROJECT_AUDIT 应先报告 `KNOWLEDGE_MISMATCH` 并阻止自动 patch，不能用生成 symbol 反过来证明本算法需要 Timer。

本算法不需要 SysConfig 页面或截图。若用户选择上游硬件采集，请按所选上游模块的 GUI 路径配置 ADC12/Timer/DMA/Event；`trigger_capture` 只消费已经采好的数组。

### Final checklist / Common mistakes / Do not change

- DMA 已停止或 buffer ownership 已交给 Application 后才搜索；阈值单位是 ADC code。
- 没有把 Timer Capture 与“在数组中找 trigger”混为一谈。
- 不修改 `.syscfg`/生成文件；在修正 Registry/Module Card 前保持 `DRAFT_BLOCKED_KNOWLEDGE_MISMATCH`。

## 你真的需要这个模块吗？

### MSPM0G3507 比赛推荐方式

本模块是对已采 raw buffer 的触发边沿搜索与片段提取算法，继续保留。硬件采样由 ADC DMA/Ring Buffer 提供，二者职责不要混在一个 DriverLib 调用里。

## 第一次使用 Trigger Capture？从这里开始

目标：“在已经采好的 `uint16_t samples[]` 中找触发点，并截取包含 pre-trigger 的固定长度波形”。它不直接启动 ADC/DMA。

### STEP 1：加入工程

链接 `MSPM0_Signal_Contest/02_acquisition/trigger_capture/signal_trigger_capture.c`；Include Path 加本目录和 `MSPM0_Signal_Contest/01_bsp/common`。

### STEP 2：include

```c
#include "signal_trigger_capture.h"
```

### STEP 3：变量

```c
uint16_t samples[N];
uint16_t segment[M];
signal_trigger_config_t cfg;
size_t trigger_index;
```

输入和输出数组都由 Application 创建；本模块不维护隐藏 ring buffer。

### STEP 4：参数

| 参数 | 常用初值 | 调大/调小与错误现象 | SysConfig |
|---|---|---|---|
| `level` | 12 bit 中点从 2048 起 | 设错会找不到或位置偏移 | 否 |
| `hysteresis` | 例如 16 code 起试 | 大：抗抖但弱边沿可能漏；小：噪声重复触发。实现采用迟滞状态机，允许经过多个采样点跨过迟滞带，不要求相邻两点一次跨完。 | 否 |
| `edge` | RISING/FALLING/EITHER | EITHER 可能找到不希望的边沿 | 否 |
| `search_start` | 0 或预留起点 | 太大可能跳过目标 | 否 |
| `pretrigger_count` | 小于输出长度且触发前数据足够 | 太大返回范围错误 | 否 |

### STEP 5：SysConfig

本模块：**【不需要 SysConfig】**。上游 ADC DMA 需要 P01；捕获+重放可对照 P04。这里是软件扫描已有 buffer，不是硬件触发单元。

### STEP 6：初始化

没有 Init；输入采集完成后调用 Find。

### STEP 7：真正调用

```c
cfg.level = 2048U;
cfg.hysteresis = 16U;
cfg.edge = SIGNAL_TRIGGER_RISING;
signal_result_t status = SignalTrigger_Find(
    samples, N, &cfg, 0U, &trigger_index);
if (status == SIGNAL_RESULT_OK) {
    status = SignalTrigger_Extract(samples, N, trigger_index,
        PRE, segment, M);
}
```

`Find` 的迟滞行为是：先看见样本进入 `level-hysteresis` 以下，记住“上升沿已武装”；以后某个样本到达 `level+hysteresis` 才触发。下降沿相反。这个状态只在本次数组扫描中维护，特别适合高采样率下相邻点变化小的正弦或任意波。

### STEP 8：结果

`trigger_index` 是触发样本索引；`segment[0..M-1]` 是 raw ADC code。要电压时再接 ADC To Voltage。

### STEP 9：连接

```text
ADC DMA / Ring Buffer -> Trigger Find -> Trigger Extract -> ADC To Voltage -> Analyze
Trigger Extract -> Period Segment / Resample -> DAC DMA replay
```

第二条真实参考见 `08_applications/waveform_capture_replay/main.c`。

### STEP 10：Build

header/status 缺失=Include；undefined symbol=未链接 `.c`；找不到 trigger=level/edge/hysteresis 或输入范围问题；Extract 越界=pretrigger/输出长度超出可用数据。

### STEP 11：验证

构造 `{1000,1500,2100,2500}`、level=2048、RISING，应在 1500→2100 边沿找到触发；再检查 segment 前触发点数。

### STEP 12：常见修改

1. level 2048→1000：只改 `cfg.level`；单位是 raw code，不是 V。
2. 上升改下降：改 `cfg.edge`。
3. 增加 pre-trigger：保证触发索引前有足够样本；输出长度不变时触发后样本减少。
4. 保留最近历史：上游使用 Ring Buffer，不要复制 ring 逻辑进本模块。

### STEP 13：完整最小示例

```c
#include "signal_trigger_capture.h"
void Capture(void)
{
    const uint16_t x[4] = {1000U,1500U,2100U,2500U};
    uint16_t out[3]; size_t index;
    const signal_trigger_config_t c = {
        .level=2048U, .hysteresis=0U, .edge=SIGNAL_TRIGGER_RISING
    };
    if (SignalTrigger_Find(x, 4U, &c, 0U, &index) == SIGNAL_RESULT_OK) {
        (void)SignalTrigger_Extract(x, 4U, index, 1U, out, 3U);
    }
}
```

下面是边沿定义、Buffer 规则、验证证据和完整 API Reference。

## 1. 模块作用

在原始帧中查找带迟滞边沿并提取触发前后数据。

## 2. 输入

输入由公开头文件中的指针、长度、配置结构或平台回调给出；所有单位写在字段名中。

## 3. 输出

输出写入调用者提供的结果/缓冲区，并返回 `signal_result_t`；不通过隐藏全局变量传递数据。

## 4. 依赖

`signal_status.h`。

## 5. SysConfig 设置

新手详细配置：[Comparator → Event → Timer Capture 完整链](../../00_docs/SYSCONFIG_BEGINNER_GUIDE.md#comparator)。本模块重点检查触发边沿、前后缓冲的 ISR/DMA owner 与 Timer tick；当前硬件资源基线是 P05，不等于触发波形已板测。现场速查见 [Quick Reference](../../00_docs/SYSCONFIG_QUICK_REFERENCE.md)。

通用代码不绑定 SysConfig 实例名；接到 MSPM0 时由独立平台适配器使用生成宏。当前状态不代表对应外设已实板验证。

## 6. 初始化方法

模块不做隐式全局初始化。包含 `signal_trigger_capture.h`，由调用者准备配置、缓冲区或平台回调；如头文件提供 Init/Configure，先调用它。

## 7. 调用方法

`SignalTrigger_Find`、`SignalTrigger_Extract`、`SignalTrigger_GetModuleStatus`。

## 8. 参数修改方法

只修改调用者配置结构、count/capacity 和采样率等函数参数；不要为某个 Demo 改底层 `.c`。

## 9. 与其他模块如何连接

通过 `signal_types.h` 的数组+长度+采样率语义或本模块公开结构连接；先检查返回码再消费输出。

## 10. 最小示例

~~~c
#include "signal_trigger_capture.h"

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

## 17. 使用场景与信号链

想到“已经采到一帧 raw，但我只想保留触发点前后的一小段”时使用。它是纯数组算法，不负责 ADC、DMA 或硬件比较器触发。

```text
ADC DMA / Ring Buffer -> uint16_t raw[N] -> Trigger Find -> trigger_index
                                             -> Trigger Extract -> segment[M]
```

## 18. 完整 Public API Reference

### `SignalTrigger_Find(samples, count, config, search_start, trigger_index)`

- `samples`：只读 `uint16_t[count]` raw code。
- `config.level`/`hysteresis`：ADC code；`edge` 为 rising/falling/either。
- `search_start`：开始检查的数组索引；必须留出前一个样本。
- `trigger_index`：成功时返回检测到边沿的索引。
- 返回：OK、INVALID_ARGUMENT、OUT_OF_RANGE 或 NO_FEATURE（以 `signal_trigger_capture.c` 实际判断为准）。调用前不需要 Init；每帧可调用一次或多次。

### `SignalTrigger_Extract(samples, count, trigger_index, pretrigger_count, output, output_count)`

从 `trigger_index` 向前保留 `pretrigger_count` 点，连续复制 `output_count` 个 raw 到 `output`。输入/输出必须非空，触发前后范围必须落在原数组内；模块没有隐藏 workspace。源码使用复制循环，输入输出不应重叠。

### `SignalTrigger_GetModuleStatus()`

返回当前固定证据等级 `MODULE_STATUS_BUILD_VERIFIED`；不表示已经在目标触发波形上实板验证。

```c
signal_trigger_config_t cfg = {
    .level = 2048U, .hysteresis = 16U, .edge = SIGNAL_TRIGGER_RISING
};
size_t trigger;
if (SignalTrigger_Find(raw, N, &cfg, 1U, &trigger) == SIGNAL_RESULT_OK) {
    (void)SignalTrigger_Extract(raw, N, trigger, 64U, segment, 256U);
}
```

## 19. Call Sequence / Buffer Rules

`采集完成 -> Find -> 检查 OK -> Extract -> 使用 segment`。raw 由上游拥有且只读；segment 由调用者创建和写入。`output_count` 是元素数，RAM 为 `2*output_count` bytes。提取失败后不要使用 segment。

## 20. Parameter Guide / Modification Tasks

| 参数 | 增大后的影响 | 减小后的影响 | RAM | SysConfig |
|---|---|---|---|---|
| `level` | 触发电平提高 | 电平降低 | 无 | 否 |
| `hysteresis` | 更抗噪，也可能漏小边沿 | 更敏感，也可能抖动重复触发 | 无 | 否 |
| `search_start` | 跳过更多帧头 | 更早开始查找 | 无 | 否 |
| `pretrigger_count` | 保留更多触发前历史 | 更少历史 | 仅随 output buffer | 否 |
| `output_count` | 片段更长 | 片段更短 | `2M` | 否 |

若想用 V 作阈值，先自行用 ADC 标定把 V 换成 code；本 API 的 `level/hysteresis` 都不是 V。

## 21. Config vs SysConfig / Verification

本模块全部为 CONFIG ONLY，SysConfig Not Applicable。只有上游采集通道/Timer/DMA 或硬件 Comparator 触发才改 SysConfig。验证时构造明显跨过阈值的 raw 数组，检查 trigger 索引、首个提取样本和边界失败；再用已知方波采集帧实测。

常见错误：把阈值当 V、`search_start=0` 却未考虑前一采样、pretrigger 超出帧头、输出尾部越界、输入输出重叠、在 DMA 未完成时搜索。

## 22. Quick Modify Table

| 我想改什么 | 去哪里 | 改什么 | 影响 | SysConfig? |
|---|---|---|---|---|
| 触发电平 | Application config | `level`（ADC code） | 触发位置 | 否 |
| 抗抖程度 | Application config | `hysteresis` | 灵敏度/重复触发 | 否 |
| 上升/下降沿 | Application config | `edge` | 匹配边沿 | 否 |
| 触发前点数 | Extract call | `pretrigger_count` | 片段布局 | 否 |
| 片段长度 | output 声明/call | `output_count` | RAM/显示长度 | 否 |

## 17. 统一 API 教程（已按当前头文件核对）

本节由当前公开头文件、实现中实际出现的状态码和正式模块注册表生成。它补充前文的场景教程；函数签名变化时必须重新运行 `tools/upgrade_formal_beginner_docs.ps1`，不要手工保留旧 API。

遵循仓库的 [Beginner README 标准](../../00_docs/BEGINNER_README_STANDARD.md)：先用最小示例完成一次正常数据流，再按需要阅读全功能示例和本节 API 细节。

### 什么时候用 / 什么时候不要用

当题目需要“trigger_capture”目录对应的公开功能，并且输入数据、单位和硬件资源满足前文约束时使用本模块。若只需要更简单的上游功能、输入尚未准备好，或需要不同的数据模型/外设资源，应先选择相邻模块而不是强行调用本 API。

### 输入 / 输出

输入由各 API 的只读数组、配置、频率/阈值和平台对象组成；输出写入 result/output/buffer 参数或由 Get API 返回。调用者负责数组容量、生命周期和物理单位；失败返回时输出不是有效结果。

### 调用顺序

```text
SignalTrigger_GetModuleStatus -> SignalTrigger_Find -> SignalTrigger_Extract
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

### `signal_result_t SignalTrigger_Find(const uint16_t *samples, size_t count, const signal_trigger_config_t *config, size_t search_start, size_t *trigger_index);`

**它做什么：** 从已有 raw 帧中寻找满足迟滞条件的第一条边沿。

**什么时候调用：** 执行该模块公开的功能；具体数据流以本节参数表和本 README 前面的场景说明为准。

| 参数 | 类型 | 初学者解释 |
|---|---|---|
| `samples` | `const uint16_t *` | 调用者持有的数据或缓冲区。容量、单位和读写方向以函数名及本 README 的输入输出说明为准；异步硬件操作完成前不得改写。 |
| `count` | `size_t` | 元素数量或容量，单位是“元素个数”而不是字节；必须与实际数组大小一致。 |
| `config` | `const signal_trigger_config_t *` | 调用者填写的配置对象。先阅读该类型的成员；它控制本次初始化或处理方式。 |
| `search_start` | `size_t` | 搜索起点之前的样本索引；至少要为边沿比较保留下一点。 |
| `trigger_index` | `size_t *` | 成功时写入边沿的“当前样本”索引。 |

**返回：** OK 找到；NO_DATA 未找到；INVALID_ARGUMENT 为非法数组/范围。

**最小调用形状：** `SignalTrigger_Find(...);`。可直接从 README_MINIMAL_EXAMPLE.c 复制正常流程；README_FULL_EXAMPLE.c 展示全部公开 API，其中取消类 API 会以 #if 0 隔离。

**注意：** 所有指针和数组都由调用者拥有；先检查返回值。异步采集、DMA 或回调还在使用 buffer 时，不能读取结果或改写该 buffer。

### `signal_result_t SignalTrigger_Extract(const uint16_t *samples, size_t count, size_t trigger_index, size_t pretrigger_count, uint16_t *output, size_t output_count);`

**它做什么：** 将触发点前 pretrigger_count 点开始的连续片段复制到 output。

**什么时候调用：** 执行该模块公开的功能；具体数据流以本节参数表和本 README 前面的场景说明为准。

| 参数 | 类型 | 初学者解释 |
|---|---|---|
| `samples` | `const uint16_t *` | 调用者持有的数据或缓冲区。容量、单位和读写方向以函数名及本 README 的输入输出说明为准；异步硬件操作完成前不得改写。 |
| `count` | `size_t` | 元素数量或容量，单位是“元素个数”而不是字节；必须与实际数组大小一致。 |
| `trigger_index` | `size_t` | 索引或通道号；范围由相应数组长度、FFT bin 数或当前硬件配置决定。 |
| `pretrigger_count` | `size_t` | 元素数量或容量，单位是“元素个数”而不是字节；必须与实际数组大小一致。 |
| `output` | `uint16_t *` | 由调用者分配的输出对象/数组。成功返回后才读取其中内容。 |
| `output_count` | `size_t` | 由调用者分配的输出对象/数组。成功返回后才读取其中内容。 |

**返回：** OK 成功；INSUFFICIENT_BUFFER 表示所需片段超出原帧。

**最小调用形状：** `SignalTrigger_Extract(...);`。可直接从 README_MINIMAL_EXAMPLE.c 复制正常流程；README_FULL_EXAMPLE.c 展示全部公开 API，其中取消类 API 会以 #if 0 隔离。

**注意：** 所有指针和数组都由调用者拥有；先检查返回值。异步采集、DMA 或回调还在使用 buffer 时，不能读取结果或改写该 buffer。

### `signal_module_status_t SignalTrigger_GetModuleStatus();`

**它做什么：** 返回构建验证证据等级，不表示上游 ADC 已实板验证。

**什么时候调用：** 读取模块当前的验证成熟度或静态状态，不会启动硬件操作。

**参数：** 无。

**返回：** 当前实现中出现的返回/成熟度枚举值：`MODULE_STATUS_BUILD_VERIFIED`。

**最小调用形状：** `SignalTrigger_GetModuleStatus(...);`。可直接从 README_MINIMAL_EXAMPLE.c 复制正常流程；README_FULL_EXAMPLE.c 展示全部公开 API，其中取消类 API 会以 #if 0 隔离。

**注意：** 所有指针和数组都由调用者拥有；先检查返回值。异步采集、DMA 或回调还在使用 buffer 时，不能读取结果或改写该 buffer。

### 示例、模块链与验收

- 最小入门：`README_MINIMAL_EXAMPLE.c`，只保留正常入口和结果读取。
- 全功能：`README_FULL_EXAMPLE.c`，以正确顺序展示当前头文件全部公开 API；`Stop` 等非常规路径不会默认执行。
- 模块链：先由上游提供单位、采样率和有效数据，再调用本模块；成功后将输出交给显示、控制、测量或下一步 DSP。硬件资源仍以 SysConfig 合约为唯一来源。
- 文档验收：README/API、两份示例和头文件会由 `tools/validate_beginner_documentation.ps1` 覆盖检查；这只表示文档与源码签名一致，不代替未进行的实板验证。

### 模块链

`上游采集/配置 -> 本模块 -> 检查返回值和结果 -> 测量、显示、控制或下一步 DSP`。只有确认本模块的输出单位和有效状态后，才交给下一模块。

