# comparator

## 什么时候用它（先做 Resource Check）

- **优先使用**：方波/边沿测频、一般过零、硬件阈值、Trigger 和 Comparator Event → Timer Capture；内部互联能省接线和软件延迟。
- **慎用**：小幅正弦的精密过零或相位；内部 Comparator 最大 ±20 mV 级 offset、hysteresis/filter 和传播延迟都可能形成误差。
- **不要强行使用**：输入超出 0～VDD、要求远低于几十 ns 的延迟/dispersion，或阈值精度明显超过内部能力；应评估具体外置 Comparator。

性能卡与外置选择条件见 [MSPM0G3507 资源能力指南](../../00_docs/MSPM0G3507_RESOURCE_CAPABILITY_GUIDE.md) 和 [内部/外置选择指南](../../00_docs/INTERNAL_VS_EXTERNAL_SELECTION_GUIDE.md)。

## 你真的需要这个模块吗？

**只读比较器输出或使用固定 SysConfig 配置时不需要旧 wrapper。** 直接用 SysConfig + Comparator DriverLib。只有 Comparator → Event → Timer Capture 的完整测频流程才进入复杂模块路线。见 [TI DriverLib 初学者指南](../../00_docs/TI_DRIVERLIB_BEGINNER_GUIDE.md)。

## 30 秒拼装路线

1. 固定配置新工程不链接旧 signal_comparator.c/.h；它仅 [REFERENCE ONLY]。完整测频选 Timer Capture。
2. [GENERATED] SysConfig 配 COMP0、input MUX/Pin、reference DAC、hysteresis/filter/polarity、Event。
3. P05 实例：外部信号 PA27→COMP0 negative input；内部 VDDA DAC 作参考；Event4→TIMG6 Capture。
4. SYSCFG_DL_init() 后可直接读 DL_COMP_getComparatorOutput(SIGNAL_COMP_INST)；测频则使用 Capture platform。
5. P05 不把 comparator output 接到外部 Pin；它通过内部 Event 路由。不要寻找不存在的板外输出线。
6. Clean → Build；先用已知阈值上下电压确认 output 翻转，再接 Timer Capture。

## 第一次把本模块加入母版工程

### STEP 1～4：文件、SysConfig、引脚与题目参数

- [LINK] 固定比较无 BSP 源；Timer 测频按其 README linked source；[COPY] 无；[GENERATED] ti_msp_dl_config.*；[REFERENCE ONLY] P05。
- .syscfg 添加 COMP，选择输入 terminal/channel、合法 analog Pin、reference source/DAC code、hysteresis/filter、polarity；要硬件测频再发布 Event 给 Capture subscriber。
- 输入电压必须在 MSPM0G3507 允许范围并共地；P05 的 PA27 是 COMP0 negative input，所以 output 极性必须结合 P/N 端与 polarity 判断。

### STEP 5～10：main、结果与连接

~~~c
#include <stdbool.h>
#include "ti_msp_dl_config.h"
volatile bool g_comp_high;
int main(void)
{
    SYSCFG_DL_init();
    g_comp_high =
        (DL_COMP_getComparatorOutput(SIGNAL_COMP_INST) ==
         DL_COMP_OUTPUT_HIGH);
    while (1) { __WFI(); }
}
~~~

g_comp_high 是当前数字比较结果，不是输入电压。连接：Comparator output→状态判断；Comparator Event→Timer Capture→frequency_hz；Comparator→Trigger Capture。

### STEP 11～12：Build 与最小验证

SIGNAL_COMP_INST 不存在=未使用 P05 名称；不翻转=Pin/P-N/reference/极性错误；Capture 无 timestamp=publisher/subscriber channel 不一致。保存 SysConfig → Clean → Build；先读 output，再跑 timer_capture_minimum。

## 比赛现场最常改的地方

经常改输入 Pin/terminal、threshold、hysteresis、polarity；偶尔改 filter/Event；通常不改底层 DriverLib，也不使用旧 callback。

## 从母版到成功调用：完整例子

静态比较使用上面 main；频率闭环使用 timer_capture_minimum：PA27→COMP0→Event4→TIMG6→frequency_hz。

## MSPM0G3507 比赛推荐方式

固定阈值、输入 MUX、迟滞、极性与事件路由优先在 SysConfig 完成；运行时只需简单 enable、读输出或清标志时直接使用 `DL_COMP_*`。本目录的 callback 配置抽象层在锁定 MSPM0G3507 后收益不足，**新工程通常不推荐**。完整“Comparator → Event → Timer Capture → Hz”仍使用 Timer Capture 复杂流程。

## 1. 模块作用

描述比较器阈值、迟滞和极性并交给平台适配器。

## 2. 输入

输入由公开头文件中的指针、长度、配置结构或平台回调给出；所有单位写在字段名中。

## 3. 输出

输出写入调用者提供的结果/缓冲区，并返回 `signal_result_t`；不通过隐藏全局变量传递数据。

## 4. 依赖

`signal_status.h`。

## 5. SysConfig 设置

新手详细配置：[Comparator + Event + Capture 教程](../../00_docs/SYSCONFIG_BEGINNER_GUIDE.md#comparator)。本模块重点检查正/负输入、参考源、DAC threshold、hysteresis、filter、输出极性、Event/IRQ 和输入 Pin。现场速查见 [Quick Reference](../../00_docs/SYSCONFIG_QUICK_REFERENCE.md)。

通用代码不绑定 SysConfig 实例名；接到 MSPM0 时由独立平台适配器使用生成宏。当前状态不代表对应外设已实板验证。

## 6. 初始化方法

模块不做隐式全局初始化。包含 `signal_comparator.h`，由调用者准备配置、缓冲区或平台回调；如头文件提供 Init/Configure，先调用它。

## 7. 调用方法

`SignalComparator_ValidateConfig`、`SignalComparator_Apply`、`SignalComparator_GetModuleStatus`。

## 8. 参数修改方法

只修改调用者配置结构、count/capacity 和采样率等函数参数；不要为某个 Demo 改底层 `.c`。

## 9. 与其他模块如何连接

通过 `signal_types.h` 的数组+长度+采样率语义或本模块公开结构连接；先检查返回码再消费输出。

## 10. 最小示例

~~~c
#include "signal_comparator.h"

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

## 17. README Usability Upgrade：完整 API

**与上层模块的关系：** 这是低层比较器边界；硬件测频优先 Comparator Zero Cross/Threshold + Timer Capture。

以下声明来自真实公开头文件；源码没有说明的项保留 `UNKNOWN / NOT EXPOSED`。

### `signal_result_t SignalComparator_ValidateConfig( const signal_comparator_config_t *config, float supply_voltage_v);`

- **作用：** UNKNOWN / NOT EXPOSED

| 参数 | 真实类型 | 真实说明 |
|---|---|---|
| `config` | `const signal_comparator_config_t *` | UNKNOWN / NOT EXPOSED |
| `supply_voltage_v` | `float` | UNKNOWN / NOT EXPOSED |

- **返回：** UNKNOWN / NOT EXPOSED
- **调用前/后：** UNKNOWN / NOT EXPOSED

```c
signal_result_t result_value = SignalComparator_ValidateConfig(config, supply_voltage_v);
```

### `signal_result_t SignalComparator_Apply(const signal_comparator_t *comparator, const signal_comparator_config_t *config, float supply_voltage_v);`

- **作用：** UNKNOWN / NOT EXPOSED

| 参数 | 真实类型 | 真实说明 |
|---|---|---|
| `comparator` | `const signal_comparator_t *` | UNKNOWN / NOT EXPOSED |
| `config` | `const signal_comparator_config_t *` | UNKNOWN / NOT EXPOSED |
| `supply_voltage_v` | `float` | UNKNOWN / NOT EXPOSED |

- **返回：** UNKNOWN / NOT EXPOSED
- **调用前/后：** UNKNOWN / NOT EXPOSED

```c
signal_result_t result_value = SignalComparator_Apply(comparator, config, supply_voltage_v);
```

### `signal_module_status_t SignalComparator_GetModuleStatus(void);`

- **作用：** UNKNOWN / NOT EXPOSED

参数：无。

- **返回：** UNKNOWN / NOT EXPOSED
- **调用前/后：** UNKNOWN / NOT EXPOSED

```c
signal_module_status_t result_value = SignalComparator_GetModuleStatus();
```

## 18. Call Sequence / Connecting / Buffer Rules

```text
SignalComparator_ValidateConfig -> SignalComparator_Apply -> SignalComparator_GetModuleStatus
```

按具体功能只调用需要的 API；Init/Validate 在执行前，Get/Is 在执行后，Stop 在退出/取消时。每步检查返回值。指针、count、capacity 按元素数和真实声明准备；对象/数组由调用者拥有，模块不动态分配；DMA 或外设仍使用 buffer 时不能改写。

## 19. Config vs SysConfig / Resources / Verification

- 原 SysConfig 说明：通用代码不绑定 SysConfig 实例名；接到 MSPM0 时由独立平台适配器使用生成宏。当前状态不代表对应外设已实板验证。
- 软件参数和 buffer 长度为 CONFIG ONLY；pin、instance、clock、Timer、DMA、Event、IRQ、reference 为 SYSCONFIG REQUIRED。
- RAM 看实例和调用者 buffer；Flash/Stack 看最终 `.map`。
- 用已知输入与边界返回码做最小验证；未实板不得写 BOARD_VERIFIED。

## 20. Quick Modify Table

| 我想改什么 | 去哪里 | 影响 | SysConfig? |
|---|---|---|---|
| 软件参数/长度 | 上述真实 API/结构 | 按参数说明；UNKNOWN 项不猜测 | 否 |
| pin/外设/时钟 | `.syscfg` 与平台层 | 改变物理资源，需核对生成宏 | 是 |
| buffer 容量 | Application 声明与 count/capacity | RAM/可处理数据量 | 否 |

常见错误：不检查返回码、byte/element 混用、生命周期不足、跳过 Init/Validate、把配置值当实测值、改硬件后未重新生成 SysConfig。

## Integration Closure

Comparator callback 把 V 单位的 threshold/hysteresis 与 COMP0 寄存器隔开。正式 MSPM0G3507 实现是 `SignalMSPM0G3507_Comparator_Apply()`：threshold 按 `round(V*256/Vref)` 量化到 COMP 内部 DAC8 的 0..255；hysteresis 量化到 DriverLib 明确提供的典型 0/10/20/30 mV；polarity 映射到 `DL_COMP_setOutputPolarity()`。用户不写 callback。

SysConfig 必须先确定输入 pin、DAC reference source、DACCODE0 software control、event publisher channel 与 Capture subscriber channel；参考 `PROFILE_05_FREQUENCY`（COMP0 PA27，event channel 4，TIMG6 capture）。

## Copy Into Target Project

链接 BSP Comparator 和统一平台 `.c`。先 `SYSCFG_DL_init()`，再创建 `signal_mspm0g3507_comparator_context_t { SIGNAL_COMP_INST, 3.3f }` 并 Bind/Apply。完整 Comparator→Event→Timer Capture 工程见 `09_examples/platform_closure/timer_capture_minimum`。threshold/hysteresis 单位是 V，实际量化值受 VDDA 与芯片典型迟滞误差影响。

## Hardware / Platform Binding

- Platform：[MSPM0G3507 Platform Adapter](../../08_applications/common/mspm0g3507/README.md)。
- 头/源文件：`signal_mspm0g3507_platform.h/.c`；capture 再加 `signal_mspm0g3507_capture_platform.h/.c`。
- `SignalMSPM0G3507_Comparator_Bind` 填入 apply callback；用户不写 DAC8/迟滞 glue。
- SysConfig：`PROFILE_05_FREQUENCY`。
- 【COMPILE-VERIFIED EXAMPLE】：[`timer_capture_minimum/main.c`](../../09_examples/platform_closure/timer_capture_minimum/main.c)
