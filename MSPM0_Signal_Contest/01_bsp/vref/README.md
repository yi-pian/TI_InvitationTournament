# vref

## 什么时候用片内 VREF（先做 Resource Check）

- **优先使用**：内部 ADC/DAC/COMP 需要方便的 1.4 V 或 2.5 V 参考，且百分比级初始范围可校准或已满足题目。
- **慎用**：精密幅值、温漂或低噪声测量；必须把 VREF 初始误差、约 80 ppm/°C 量级温漂、启动时间和去耦算入误差。
- **不要强行使用**：题目需要更高绝对精度/更低温漂/噪声，或想把 VREF 当外部模块电源；应评估外部精密参考。

性能边界见 [MSPM0G3507 资源能力指南](../../00_docs/MSPM0G3507_RESOURCE_CAPABILITY_GUIDE.md) 和 [内部/外置选择指南](../../00_docs/INTERNAL_VS_EXTERNAL_SELECTION_GUIDE.md)。

## 你真的需要这个模块吗？

**只启用片上 VREF 硬件时不需要本 helper。** 硬件配置直接放在 SysConfig；只有需要在算法中统一选择“标称/实测 VREF”时，才链接本目录的纯计算 helper。它不会操作 VREF 外设。

## 30 秒拼装路线

1. 只做标称/实测 VREF 选择时加入 `signal_vref.c/.h` 和公共状态头；新比赛母版采用冻结复制，维护旧 Application 时可以保留 linked source。
2. 该 helper [COPY]/[GENERATED] 无，不需要 SysConfig、不占 Pin/Timer/DMA。
3. 准备 signal_vref_calibration_t；实测值 >0 时优先用实测，否则用标称。
4. 调用 SignalVREF_GetEffectiveVoltage(&cal, &vref_v)，结果 vref_v 单位 V。
5. 若真正启用片上 VREF，必须另在 SysConfig 配 VREF 与 ADC/COMP reference；helper 不操作硬件。
6. Clean → Build；用 nominal=3.3/measured=3.28 验证输出 3.28 V。

## 第一次把本模块加入母版工程

### STEP 1～4：文件、CCS、硬件边界和参数

- 必要文件：`01_bsp/vref/signal_vref.c/.h` 与 `01_bsp/common/signal_status.h`；无 generated source，文档仅作参考。
- 旧 projectspec 可以使用 `MSPM0_SIGNAL_LIBRARY_ROOT` linked source；新比赛母版把必要文件冻结复制到 `modules/`，此纯 helper 不改 `.syscfg`。
- 若题目要求内部 VREF：打开 .syscfg 添加 VREF、选电压档位/consumer/启动稳定；generated config 属硬件层。当前 P01–P07 没有独立 VREF Profile，不能假造实例名。

### STEP 5～10：main、结果与连接

~~~c
#include "signal_vref.h"
static float g_vref_v;
int main(void)
{
    const signal_vref_calibration_t cal = {3.3f, 3.28f};
    (void) SignalVREF_GetEffectiveVoltage(&cal, &g_vref_v);
    while (1) {}
}
~~~

结果 g_vref_v 是应用参考电压。连接：VREF helper→ADC To Voltage config；VREF hardware→ADC/COMP reference；实测 VREF→电压校准。

### STEP 11～12：Build 与最小验证

header not found=本目录/common include 缺失；undefined symbol=未 linked signal_vref.c；返回 invalid=标称≤0或实测<0。PC/板上均可验证 helper；真实 VREF consumer 仍需板上测量。

## 比赛现场最常改的地方

经常改 nominal/measured V；偶尔改 SysConfig 的 VREF 档位与 consumer；通常不要改 helper 选择规则。

## 从母版到成功调用：完整例子

母版加入 `signal_vref.c/.h` → 填 calibration → 得 `g_vref_v` → 写入 ADC To Voltage `reference_voltage_v`；硬件 VREF 若启用则单独按 SysConfig 配置。

## MSPM0G3507 比赛推荐方式

内部 VREF 的电压档位、给 ADC/COMP 的参考与上电稳定配置优先交给 SysConfig；简单运行时控制使用 `DL_VREF_*`。本模块不控制硬件，只在“实测参考电压优先，否则用标称值”时作为可选纯计算 helper。

## 1. 模块作用

选择标称或实测参考电压，显式传递标定依据。

## 2. 输入

输入由公开头文件中的指针、长度、配置结构或平台回调给出；所有单位写在字段名中。

## 3. 输出

输出写入调用者提供的结果/缓冲区，并返回 `signal_result_t`；不通过隐藏全局变量传递数据。

## 4. 依赖

`signal_status.h`。

## 5. SysConfig 设置

新手详细配置：[ADC Reference](../../00_docs/SYSCONFIG_BEGINNER_GUIDE.md#adc) 与 [DAC Reference](../../00_docs/SYSCONFIG_BEGINNER_GUIDE.md#dac)。本模块重点检查 Internal/External VREF、目标电压、VREF+/- Pin 和 ready/startup；修改后同步 raw↔V 换算。现场速查见 [Quick Reference](../../00_docs/SYSCONFIG_QUICK_REFERENCE.md)。

通用代码不绑定 SysConfig 实例名；接到 MSPM0 时由独立平台适配器使用生成宏。当前状态不代表对应外设已实板验证。

## 6. 初始化方法

模块不做隐式全局初始化。包含 `signal_vref.h`，由调用者准备配置、缓冲区或平台回调；如头文件提供 Init/Configure，先调用它。

## 7. 调用方法

`SignalVREF_GetEffectiveVoltage`、`SignalVREF_GetModuleStatus`。

## 8. 参数修改方法

只修改调用者配置结构、count/capacity 和采样率等函数参数；不要为某个 Demo 改底层 `.c`。

## 9. 与其他模块如何连接

通过 `signal_types.h` 的数组+长度+采样率语义或本模块公开结构连接；先检查返回码再消费输出。

## 10. 最小示例

~~~c
#include "signal_vref.h"

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

当题目需要“vref”目录对应的公开功能，并且输入数据、单位和硬件资源满足前文约束时使用本模块。若只需要更简单的上游功能、输入尚未准备好，或需要不同的数据模型/外设资源，应先选择相邻模块而不是强行调用本 API。

### 输入 / 输出

输入由各 API 的只读数组、配置、频率/阈值和平台对象组成；输出写入 result/output/buffer 参数或由 Get API 返回。调用者负责数组容量、生命周期和物理单位；失败返回时输出不是有效结果。

### 调用顺序

```text
SignalVREF_GetEffectiveVoltage -> SignalVREF_GetModuleStatus
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

### `signal_result_t SignalVREF_GetEffectiveVoltage(const signal_vref_calibration_t *calibration, float *voltage_v);`

**它做什么：** 读取最近一次操作保存的状态、结果或配置；先确认前置操作已经成功。

**什么时候调用：** 读取最近一次操作保存的状态、结果或配置；先确认前置操作已经成功。

| 参数 | 类型 | 初学者解释 |
|---|---|---|
| `calibration` | `const signal_vref_calibration_t *` | `calibration`（`const signal_vref_calibration_t `）是该 API 的输入/输出参数；按本 README 前面的数据单位和边界条件准备。 |
| `voltage_v` | `float *` | `voltage_v`（`float `）是该 API 的输入/输出参数；按本 README 前面的数据单位和边界条件准备。 |

**返回：** 返回 signal_result_t 类型结果；调用者应检查该值。

**最小调用形状：** `SignalVREF_GetEffectiveVoltage(...);`。可直接从 README_MINIMAL_EXAMPLE.c 复制正常流程；README_FULL_EXAMPLE.c 展示全部公开 API，其中取消类 API 会以 #if 0 隔离。

**注意：** 所有指针和数组都由调用者拥有；先检查返回值。异步采集、DMA 或回调还在使用 buffer 时，不能读取结果或改写该 buffer。

### `signal_module_status_t SignalVREF_GetModuleStatus();`

**它做什么：** 读取模块当前的验证成熟度或静态状态，不会启动硬件操作。

**什么时候调用：** 读取模块当前的验证成熟度或静态状态，不会启动硬件操作。

**参数：** 无。

**返回：** 返回 signal_module_status_t 类型结果；调用者应检查该值。

**最小调用形状：** `SignalVREF_GetModuleStatus(...);`。可直接从 README_MINIMAL_EXAMPLE.c 复制正常流程；README_FULL_EXAMPLE.c 展示全部公开 API，其中取消类 API 会以 #if 0 隔离。

**注意：** 所有指针和数组都由调用者拥有；先检查返回值。异步采集、DMA 或回调还在使用 buffer 时，不能读取结果或改写该 buffer。

### 示例、模块链与验收

- 最小入门：`README_MINIMAL_EXAMPLE.c`，只保留正常入口和结果读取。
- 全功能：`README_FULL_EXAMPLE.c`，以正确顺序展示当前头文件全部公开 API；`Stop` 等非常规路径不会默认执行。
- 模块链：先由上游提供单位、采样率和有效数据，再调用本模块；成功后将输出交给显示、控制、测量或下一步 DSP。硬件资源仍以 SysConfig 合约为唯一来源。
- 文档验收：README/API、两份示例和头文件会由 `tools/validate_beginner_documentation.ps1` 覆盖检查；这只表示文档与源码签名一致，不代替未进行的实板验证。

### 模块链

`上游采集/配置 -> 本模块 -> 检查返回值和结果 -> 测量、显示、控制或下一步 DSP`。只有确认本模块的输出单位和有效状态后，才交给下一模块。

