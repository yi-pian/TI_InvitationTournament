# adc

## 什么时候用它（先做 Resource Check）

- **优先使用**：输入在 0～VREF/VDD、1～2 路采集、12-bit/实际 ENOB 与最高 4 MSPS 能满足题目，并希望用 Timer/Event/DMA 内部触发。
- **慎用**：接近 4 MSPS、高源阻、500 kHz 级输入、需要严格幅相/噪声指标；必须核对采样建立时间、每周期点数、VREF 和前端驱动。
- **不要强行使用**：16/18/24-bit 精密测量、双极性/高压直入、超过两路严格同步或 >4 MSPS；应评估外置 ADC。

先看 [MSPM0G3507 资源能力指南](../../00_docs/MSPM0G3507_RESOURCE_CAPABILITY_GUIDE.md) 和 [内部/外置选择指南](../../00_docs/INTERNAL_VS_EXTERNAL_SELECTION_GUIDE.md)。

## 你真的需要这个模块吗？

**普通单点 ADC 验证不需要。** MSPM0G3507 比赛新工程请直接使用 SysConfig + ADC DriverLib；本目录旧 wrapper 仅供历史参考。要按固定 `Fs` 采集 `N` 点，请使用 ADC DMA，而不是在 `main.c` 循环单点读取。见 [TI DriverLib 初学者指南](../../00_docs/TI_DRIVERLIB_BEGINNER_GUIDE.md)。

## 30 秒拼装路线

1. 单点 ADC 新工程 [LINK] 无，旧 signal_adc.c/.h 仅 [REFERENCE ONLY]；N 点采集 [LINK] ADC DMA。
2. [GENERATED] SysConfig 配 ADC instance/MEM/channel/Pin/reference/trigger；P07 单点，P01 DMA。
3. 单点：SYSCFG_DL_init → clear flag → startConversion → wait → getMemResult。
4. N 点：不要循环单点，按 ADC DMA README 配 Timer/Event/DMA。
5. 输出 uint16_t raw/raw[N] 是 code；接 ADC To Voltage 才是 V。
6. Clean → Build；GND/已知输入做最小验证。

## 第一次把本模块加入母版工程

### STEP 1～4：文件、SysConfig、引脚与参数

- [LINK] 无 BSP ADC；[COPY] 无；[GENERATED] ti_msp_dl_config.*；[REFERENCE ONLY] P07/P01。
- .syscfg 添加 ADC12，选 ADC0、MEM0、合法 analog channel/Pin、resolution/reference、software 或 Event trigger。P07 默认 PA25/ADC0.2。
- 换 Pin 同时换 ADC channel/PinMux和板外接线；换 VREF/resolution 同步 ToVoltage；Event trigger 使用 ADC DMA 完整链。

### STEP 5～10：main、结果与连接

单点完整代码见 09_examples/platform_closure/adc_basic_minimum/main.c，当前调用为：

~~~c
DL_ADC12_clearInterruptStatus(SIGNAL_BASIC_ADC_INST,
    DL_ADC12_INTERRUPT_MEM0_RESULT_LOADED);
DL_ADC12_startConversion(SIGNAL_BASIC_ADC_INST);
while (DL_ADC12_getRawInterruptStatus(SIGNAL_BASIC_ADC_INST,
           DL_ADC12_INTERRUPT_MEM0_RESULT_LOADED) == 0U) {}
raw = DL_ADC12_getMemResult(
    SIGNAL_BASIC_ADC_INST, SIGNAL_BASIC_ADC_ADCMEM_0);
~~~

必须在 SYSCFG_DL_init() 后执行。连接：raw→ADC To Voltage；raw[N]→测量/FFT；双通道→Dual ADC platform。

### STEP 11～12：Build 与验证

宏不存在=ADC 实例/生成失败；一直等待=trigger/MEM/interrupt 配错；满量程/零=Pin、VREF、共地或前端超量程。Clean → Build；先看 raw，再接算法。

## 比赛现场最常改的地方

经常改 Pin/channel、VREF/resolution、Fs/N（上层）；偶尔改 sample time/trigger；通常不要使用旧 BSP callback。

## 从母版到成功调用：完整例子

单点闭环看 adc_basic_minimum；波形闭环看 adc_dma_minimum。两者均从母版 SysConfig 到最终 raw。

## MSPM0G3507 比赛推荐方式

- 只读一个 ADC 结果：SysConfig 配好 ADC/MEM/Pin/Reference，`SYSCFG_DL_init()` 后直接调用 `DL_ADC12_startConversion()` 与 `DL_ADC12_getMemResult()`。
- 要采一帧稳定的 `raw[N]`：使用 `02_acquisition/adc_dma`，不要在比赛工程重拼 Timer/Event/DMA。
- 本目录的 callback/descriptor BSP 是旧跨平台抽象，**新 MSPM0G3507 工程通常不推荐**；保留只为兼容现有引用。

直接用法见 [ADC Basic direct example](../../09_examples/platform_closure/adc_basic_minimum/README.md) 和 [DriverLib 指南](../../00_docs/TI_DRIVERLIB_BEGINNER_GUIDE.md)。

## 1. 模块作用

统一单次 ADC 原始码读取接口和通道/参考配置。

## 2. 输入

输入由公开头文件中的指针、长度、配置结构或平台回调给出；所有单位写在字段名中。

## 3. 输出

输出写入调用者提供的结果/缓冲区，并返回 `signal_result_t`；不通过隐藏全局变量传递数据。

## 4. 依赖

`signal_status.h`。

## 5. SysConfig 设置

新手详细配置：[ADC 与 PinMux 教程](../../00_docs/SYSCONFIG_BEGINNER_GUIDE.md#adc)。本模块重点检查 ADC instance、ADCMEM Input Channel、Reference、Resolution、Sample Time、Trigger Source 和模拟 Pin；现场速查见 [Quick Reference](../../00_docs/SYSCONFIG_QUICK_REFERENCE.md)。

通用代码不绑定 SysConfig 实例名；接到 MSPM0 时由独立平台适配器使用生成宏。当前状态不代表对应外设已实板验证。

## 6. 初始化方法

模块不做隐式全局初始化。包含 `signal_adc.h`，由调用者准备配置、缓冲区或平台回调；如头文件提供 Init/Configure，先调用它。

## 7. 调用方法

`SignalADC_ValidateConfig`、`SignalADC_ReadRaw`、`SignalADC_GetBspModuleStatus`。

## 8. 参数修改方法

只修改调用者配置结构、count/capacity 和采样率等函数参数；不要为某个 Demo 改底层 `.c`。

## 9. 与其他模块如何连接

通过 `signal_types.h` 的数组+长度+采样率语义或本模块公开结构连接；先检查返回码再消费输出。

## 10. 最小示例

~~~c
#include "signal_adc.h"

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

**与上层模块的关系：** 这是低层 building block；普通 N 点块采集优先选 ADC DMA，不要在应用里重新手拼 ADC+Timer+DMA。

以下声明来自真实公开头文件；源码没有说明的项保留 `UNKNOWN / NOT EXPOSED`。

### `signal_result_t SignalADC_ValidateConfig(const signal_adc_config_t *config);`

- **作用：** UNKNOWN / NOT EXPOSED

| 参数 | 真实类型 | 真实说明 |
|---|---|---|
| `config` | `const signal_adc_config_t *` | UNKNOWN / NOT EXPOSED |

- **返回：** UNKNOWN / NOT EXPOSED
- **调用前/后：** UNKNOWN / NOT EXPOSED

```c
signal_result_t result_value = SignalADC_ValidateConfig(config);
```

### `signal_result_t SignalADC_ReadRaw(const signal_adc_t *adc, uint16_t *raw);`

- **作用：** UNKNOWN / NOT EXPOSED

| 参数 | 真实类型 | 真实说明 |
|---|---|---|
| `adc` | `const signal_adc_t *` | UNKNOWN / NOT EXPOSED |
| `raw` | `uint16_t *` | UNKNOWN / NOT EXPOSED |

- **返回：** UNKNOWN / NOT EXPOSED
- **调用前/后：** UNKNOWN / NOT EXPOSED

```c
signal_result_t result_value = SignalADC_ReadRaw(adc, raw);
```

### `signal_module_status_t SignalADC_GetBspModuleStatus(void);`

- **作用：** UNKNOWN / NOT EXPOSED

参数：无。

- **返回：** UNKNOWN / NOT EXPOSED
- **调用前/后：** UNKNOWN / NOT EXPOSED

```c
signal_module_status_t result_value = SignalADC_GetBspModuleStatus();
```

## 18. Call Sequence / Connecting / Buffer Rules

```text
SignalADC_ValidateConfig -> SignalADC_ReadRaw -> SignalADC_GetBspModuleStatus
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

`read/enable/disable` callbacks 用来隔离 BSP 与 ADC instance/ADCMEM。软件触发单点采样的正式实现位于 `08_applications/common/mspm0g3507/signal_mspm0g3507_platform.c/.h`，通过 `SignalMSPM0G3507_ADC_Bind()` 一次性绑定，用户不写 callback。它只适用于 software-trigger ADC；Timer/Event/DMA 采一帧应直接用 ADC DMA。

## Copy Into Target Project

链接 `01_bsp/adc/signal_adc.c` 与平台 `.c`；Include 加 `01_bsp/adc`、`01_bsp/common`、平台目录、SDK/CMSIS 和生成目录。SysConfig 参考 `PROFILE_07_BASIC_IO`；最小工程见 `09_examples/platform_closure/adc_basic_minimum`。`channel` 是 ADC channel 编号，raw 为 unsigned code，reference 为 V，clock 为 Hz。

## Hardware / Platform Binding

- Platform：[MSPM0G3507 Platform Adapter](../../08_applications/common/mspm0g3507/README.md)
- 头/源文件：`signal_mspm0g3507_platform.h/.c`
- 绑定：`SignalMSPM0G3507_ADC_Bind` 填入 read/enable/disable callbacks；用户不写 DriverLib glue。
- SysConfig：`PROFILE_07_BASIC_IO`，ADC0/PA25/software trigger/MEM0 interrupt。
- 【COMPILE-VERIFIED EXAMPLE】：[`adc_basic_minimum/main.c`](../../09_examples/platform_closure/adc_basic_minimum/main.c)
