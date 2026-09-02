# dac

## CCS SysConfig GUI Configuration

### Required resources

SysConfig module 是 `DAC12`，硬件 instance 是 `DAC0`，`DL_DAC12_*` 是 DriverLib C 名称。固定 DC 只需要 DAC12；连续波形才增加 `DMA`、`TIMER` 和 `EVENT`。

### Step 1 - DAC12 fixed output

GUI Path: `Add` -> `DAC12` -> `Basic Operation Configuration`。

Action: 选择/确认 `Enable DAC`、`Output Resolution = 12-bit`、`Positive Voltage Reference`/`Negative Voltage Reference`（P07 对照为 VDDA/VSSA），并打开 `Enable DAC Output` 与 `DAC And Output Amplifier = ON`。在 `Pin Configuration`/PinMux 中确认 DAC 输出合法引脚；P07/P03 已验证输出为 `PA15`。

### Step 2 - Clock boundary

固定 DC 只写一次 DAC code，不需要 Timer、DMA 或 DAC update clock。不要为了 `DL_DAC12_output12()` 增加一个周期 Timer。连续波形的 `Fupdate` 由 Timer event + DAC FIFO + DMA 决定，按 [DAC DMA GUI 教程](../../06_generator/dac_dma/README.md#ccs-sysconfig-gui-configuration) 配置；系统 Clock Tree 与外设 functional clock 的区别见[共享时钟教材](../../00_docs/MSPM0_SYSCONFIG_CLOCK_TIMER_ADC_DAC_BEGINNER_GUIDE.md)。

### Step 3 - DMA/FIFO for waveform only

GUI Path: `DAC12` -> `Advanced Configuration` / `DMA Configuration` / `Event Configuration`。

Action: 只有连续波形才启用 `Enable FIFO`、`Configure DMA Trigger`、硬件触发和 threshold；固定 DC 保持关闭这些波形链选项。P03 的已验证值为 FIFO threshold `TWO_QTRS_EMPTY`、trigger `HWTRIG0`、subscriber channel `3`，但应先检查当前工程资源冲突。

### Expected generated symbols

固定输出至少核对 DAC instance/output 相关宏（例如 `DAC0`、输出 pin/enable 宏）；连续波形另核对 DMA channel、trigger、Timer instance 和 `DAC12_INT_IRQN`。PROJECT_AUDIT 记录 `GUI field -> .syscfg property -> generated symbol`。

### Final checklist / Common mistakes / Do not change

- 固定 DC 与连续波形两种资源链没有混用。
- `PA15` 是已验证 profile 的 DAC 输出，不代表任意 GPIO 都可作为 DAC 输出。
- 参考电压、12-bit code 范围和实际测量电压一致。
- 不把 DAC functional clock 当 `Fupdate`；不直接编辑 `.syscfg` 或生成文件。

## 什么时候用它（先做 Resource Check）

- **优先使用**：固定 DC bias、VGA/PGA 控制电压、中低频简单波形；内部 12-bit、1 MSPS DAC 的接线和调试成本最低。
- **慎用**：100 kHz 级正弦、接近满幅的快速跳变、低阻或大电容负载；先算每周期点数、`2πfVpk`、建立时间并上板测 THD/幅值。
- **不要强行使用**：500 kHz 高质量正弦、MHz 波形、精密 DC 或大电流驱动；应评估外置 DDS/DAC/缓冲器。

先看 [MSPM0G3507 资源能力指南](../../00_docs/MSPM0G3507_RESOURCE_CAPABILITY_GUIDE.md) 和 [内部/外置选择指南](../../00_docs/INTERNAL_VS_EXTERNAL_SELECTION_GUIDE.md)。

## 你真的需要这个模块吗？

**输出一个固定 DAC code 时不需要。** 新工程直接用 SysConfig + `DL_DAC12_output12()`；连续波形才使用 DAC DMA，可调频/相位时再加入 DDS。见 [TI DriverLib 初学者指南](../../00_docs/TI_DRIVERLIB_BEGINNER_GUIDE.md)。

## 30 秒拼装路线

1. 固定 code 新工程 [LINK] 无，旧 signal_dac.c/.h 仅 [REFERENCE ONLY]；连续波 [LINK] DAC DMA + platform。
2. [GENERATED] SysConfig 配 DAC0/reference/amplifier/output Pin；P07 固定输出，P03 DMA。
3. 固定输出：SYSCFG_DL_init() 后 DL_DAC12_output12(DAC0, code)。
4. code 0..4095；输出在 PA15。频率/波形链改用 DDS→DAC DMA→Platform。
5. 结果是物理模拟电压，不是函数返回变量，必须仪表测量。
6. Clean → Build；先输出 2048 code。

## 第一次把本模块加入母版工程

### STEP 1～4：文件、SysConfig、引脚与参数

- [LINK] 无 BSP DAC；[COPY] 无；[GENERATED] ti_msp_dl_config.*；[REFERENCE ONLY] P07/P03。
- .syscfg 添加 DAC12，选择 DAC0、12 bit、reference、amplifier/output、PA15；Pin 能否换以 SysConfig/器件封装为准。
- 目标电压改变改 code；reference 改变同步 code↔V 换算。周期波不要在 while 中软件延时逐点写。

### STEP 5～10：main、结果与连接

~~~c
#include "ti_msp_dl_config.h"
volatile uint16_t g_dac_code = 2048U;
int main(void)
{
    SYSCFG_DL_init();
    DL_DAC12_output12(DAC0, g_dac_code);
    while (1) { __WFI(); }
}
~~~

连接：DAC0/PA15→DUT bias；DDS block→DAC DMA→PA15；DAC PA15→ADC PA25 回环需要外部跳线并共地。

### STEP 11～12：Build 与验证

DAC0 未定义=SysConfig 未添加；无电压=输出/放大器/Pin 未启用；幅值错=reference/load/code。Clean → Build；万用表测 PA15。

## 比赛现场最常改的地方

经常改 code/目标电压；偶尔改 reference、DMA update rate（上层）；通常不要改 DAC0/PA15 和生成初始化。

## 从母版到成功调用：完整例子

固定输出看 dac_dc_minimum；连续输出看 dac_dma_minimum。两者都是当前 full-link 入口。

## MSPM0G3507 比赛推荐方式

固定 code 输出不需要 BSP、callback 或 Platform Adapter：

1. SysConfig 配置 DAC0、参考源、12-bit、输出放大器和 PA15。
2. `#include "ti_msp_dl_config.h"`。
3. 调用 `SYSCFG_DL_init();`。
4. 调用 `DL_DAC12_output12(DAC0, 2048U);`，先验证约半量程。

`DAC0` 是当前生成配置使用的 DAC 实例；12-bit `code` 范围为 `0..4095`。本目录的 `SignalDAC_WriteRaw` descriptor/callback 属于旧抽象，**新 MSPM0G3507 工程通常不推荐**。`SignalDAC_VoltageToRaw` 只有换算价值，可作为兼容性可选 helper，但新代码最简单时可直接计算 code。连续波形使用 DAC DMA。

可编译例子：[dac_dc_minimum](../../09_examples/platform_closure/dac_dc_minimum/README.md)。

## 1. 模块作用

统一 DAC 原始码输出及电压到码值换算。

## 2. 输入

输入由公开头文件中的指针、长度、配置结构或平台回调给出；所有单位写在字段名中。

## 3. 输出

输出写入调用者提供的结果/缓冲区，并返回 `signal_result_t`；不通过隐藏全局变量传递数据。

## 4. 依赖

`signal_status.h`。

## 5. SysConfig 设置

新手详细配置：[DAC 固定电压与 DMA 波形教程](../../00_docs/SYSCONFIG_BEGINNER_GUIDE.md#dac)。本模块重点检查 DAC instance、正/负参考、Output Pin、FIFO/Trigger；固定值与连续波形的配置不要混用。现场速查见 [Quick Reference](../../00_docs/SYSCONFIG_QUICK_REFERENCE.md)。

通用代码不绑定 SysConfig 实例名；接到 MSPM0 时由独立平台适配器使用生成宏。当前状态不代表对应外设已实板验证。

## 6. 初始化方法

模块不做隐式全局初始化。包含 `signal_dac.h`，由调用者准备配置、缓冲区或平台回调；如头文件提供 Init/Configure，先调用它。

## 7. 调用方法

`SignalDAC_VoltageToRaw`、`SignalDAC_WriteRaw`、`SignalDAC_GetModuleStatus`。

## 8. 参数修改方法

只修改调用者配置结构、count/capacity 和采样率等函数参数；不要为某个 Demo 改底层 `.c`。

## 9. 与其他模块如何连接

通过 `signal_types.h` 的数组+长度+采样率语义或本模块公开结构连接；先检查返回码再消费输出。

## 10. 最小示例

~~~c
#include "signal_dac.h"

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

**与上层模块的关系：** 这是低层单点输出 building block；连续波形优先 Wave Table/DDS -> DAC DMA。

以下声明来自真实公开头文件；源码没有说明的项保留 `UNKNOWN / NOT EXPOSED`。

### `signal_result_t SignalDAC_VoltageToRaw(float voltage_v, uint8_t bits, float reference_voltage_v, uint16_t *raw);`

- **作用：** UNKNOWN / NOT EXPOSED

| 参数 | 真实类型 | 真实说明 |
|---|---|---|
| `voltage_v` | `float` | UNKNOWN / NOT EXPOSED |
| `bits` | `uint8_t` | UNKNOWN / NOT EXPOSED |
| `reference_voltage_v` | `float` | UNKNOWN / NOT EXPOSED |
| `raw` | `uint16_t *` | UNKNOWN / NOT EXPOSED |

- **返回：** UNKNOWN / NOT EXPOSED
- **调用前/后：** UNKNOWN / NOT EXPOSED

```c
signal_result_t result_value = SignalDAC_VoltageToRaw(voltage_v, bits, reference_voltage_v, raw);
```

### `signal_result_t SignalDAC_WriteRaw(const signal_dac_t *dac, uint16_t raw);`

- **作用：** UNKNOWN / NOT EXPOSED

| 参数 | 真实类型 | 真实说明 |
|---|---|---|
| `dac` | `const signal_dac_t *` | UNKNOWN / NOT EXPOSED |
| `raw` | `uint16_t` | UNKNOWN / NOT EXPOSED |

- **返回：** UNKNOWN / NOT EXPOSED
- **调用前/后：** UNKNOWN / NOT EXPOSED

```c
signal_result_t result_value = SignalDAC_WriteRaw(dac, raw);
```

### `signal_module_status_t SignalDAC_GetModuleStatus(void);`

- **作用：** UNKNOWN / NOT EXPOSED

参数：无。

- **返回：** UNKNOWN / NOT EXPOSED
- **调用前/后：** UNKNOWN / NOT EXPOSED

```c
signal_module_status_t result_value = SignalDAC_GetModuleStatus();
```

## 18. Call Sequence / Connecting / Buffer Rules

```text
SignalDAC_VoltageToRaw -> SignalDAC_WriteRaw -> SignalDAC_GetModuleStatus
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

`signal_dac_write_fn` 存在是为了让电压换算与具体 DAC instance 解耦。MSPM0G3507 正式实现是 `SignalMSPM0G3507_DAC_Write()`，路径为 `08_applications/common/mspm0g3507/signal_mspm0g3507_platform.c/.h`；用户不需要自己写 callback。它检查 0..4095 后调用 `DL_DAC12_output12()`，必要时 `DL_DAC12_enable()`。SysConfig 参考 `PROFILE_07_BASIC_IO`，最终硬件为 DAC0 OUT/PA15。

## Copy Into Target Project

链接 `01_bsp/dac/signal_dac.c` 与平台 `.c`；Include 加 `01_bsp/dac`、`01_bsp/common` 和平台目录。先 `SYSCFG_DL_init()`，再 `SignalMSPM0G3507_DAC_Bind(&dac, DAC0, reference_v)`，之后调用 `SignalDAC_WriteRaw()` 或让 DAC DC 调用它。完整最小工程见 `09_examples/platform_closure/dac_dc_minimum`。

## Hardware / Platform Binding

- Platform：[MSPM0G3507 Platform Adapter](../../08_applications/common/mspm0g3507/README.md)
- 头/源文件：`signal_mspm0g3507_platform.h/.c`
- write 来源：`SignalMSPM0G3507_DAC_Bind(&dac, DAC0, reference_v)` 把 descriptor 的 `write` 设为 `SignalMSPM0G3507_DAC_Write`。
- SysConfig：`PROFILE_07_BASIC_IO`，DAC0/PA15。
- 【COMPILE-VERIFIED EXAMPLE】：[`dac_dc_minimum/main.c`](../../09_examples/platform_closure/dac_dc_minimum/main.c)
