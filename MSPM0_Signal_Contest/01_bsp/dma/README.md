# dma

## 什么时候用它（先做 Resource Check）

- **优先使用**：ADC→RAM、RAM→DAC/FIFO、连续串口等规则搬运；MSPM0G3507 只有 7 个 DMA channel，应由完整功能模块统一配置。
- **慎用**：多个 ADC、DAC、UART/SPI 同时工作；先做 channel、trigger、transfer width 和 Event 资源表。
- **不要单独选择**：DMA 不是题目功能；采集选 ADC DMA，连续输出选 DAC DMA。通道已耗尽时不能靠复制 wrapper 解决。

芯片 DMA/Event 能力见 [MSPM0G3507 资源能力指南](../../00_docs/MSPM0G3507_RESOURCE_CAPABILITY_GUIDE.md)，冲突检查见 [RESOURCE_CONFLICT_GUIDE.md](../../00_docs/RESOURCE_CONFLICT_GUIDE.md)。

## 你真的需要这个模块吗？

**通常不要单独选择 DMA wrapper。** ADC 采集直接选 ADC DMA，连续 DAC 输出直接选 DAC DMA；这些复杂模块负责把 DMA 与 Timer/Event/外设正确协同。只做一个简单片上动作时见 [TI DriverLib 初学者指南](../../00_docs/TI_DRIVERLIB_BEGINNER_GUIDE.md)。

## 30 秒拼装路线

1. 不把 DMA 当题目功能入口；新工程不链接旧 signal_dma.c/.h，它仅 [REFERENCE ONLY]。
2. ADC 采集直接选 ADC DMA；DAC 波形直接选 DAC DMA。维护现有 Application 时保留正式 linked source；新比赛母版按对应 README 冻结复制必要文件。
3. [GENERATED] SysConfig 负责 channel、trigger、width、increment、mode；禁止手改生成配置。
4. 正式平台只重装 src/dst/count 并 enable channel，不在 main 复制。
5. 结果是目标 buffer 或外设 FIFO 的搬运完成状态，不是测量值。
6. Clean → full link；先跑对应 minimum example。

## 第一次把本模块加入母版工程

### STEP 1～4：文件、CCS、SysConfig 与参数

- [LINK] 原始 BSP DMA 无；按目标 [LINK] signal_adc_dma.c 或 signal_dac_dma.c + DAC platform；[COPY] 无；[GENERATED] ti_msp_dl_config.*；P01/P03 [REFERENCE ONLY]。
- ADC：peripheral→buffer、half-word、destination increment、single；DAC：buffer→peripheral、half-word、source increment、repeat。channel/trigger/width/increment 必须与外设一致。
- 可换 DMA channel，但同步 .syscfg 生成的 *_CHAN_ID 并检查 P06 的 DMA0/1/2 占用；不要只在 C 里改数字。

### STEP 5～10：真实平台调用、结果与连接

DAC platform 当前真实重装顺序如下；这段留在正式 platform，不复制到应用：

~~~c
DL_DMA_disableChannel(DMA, SIGNAL_DAC_DMA_CHAN_ID);
DL_DMA_setSrcAddr(DMA, SIGNAL_DAC_DMA_CHAN_ID, (uint32_t) samples);
DL_DMA_setDestAddr(DMA, SIGNAL_DAC_DMA_CHAN_ID,
    (uint32_t) &(DAC0->DATA0));
DL_DMA_setTransferSize(DMA, SIGNAL_DAC_DMA_CHAN_ID, (uint16_t) count);
DL_DMA_enableChannel(DMA, SIGNAL_DAC_DMA_CHAN_ID);
~~~

连接：ADC MEM→DMA→raw[N]；DDS block→DMA→DAC FIFO。应用读取 raw[] 或完成状态。

### STEP 11～12：Build 与最小验证

undefined *_CHAN_ID=SysConfig 名称不匹配；buffer 不完整=width/increment/count/trigger 错；resource conflict=channel 重复。保存 SysConfig → Clean → full link；跑 adc_dma_minimum 或 dac_dma_minimum。

## 比赛现场最常改的地方

经常改的是上层 Fs/N/block；偶尔因冲突换 DMA channel；通常不要改 width/increment/trigger，除非完全理解数据方向。

## 从母版到成功调用：完整例子

完整例子位于 `09_examples/platform_closure/adc_dma_minimum` 和 `dac_dma_minimum`。旧工程可按 projectspec linked source 维护；新比赛母版只复用其必要文件清单和 Profile 契约，并采用冻结复制。

## MSPM0G3507 比赛推荐方式

简单单通道 DMA 配置直接使用 SysConfig + `DL_DMA_*`；本通用 descriptor/callback wrapper 没有替你解决 trigger、Event、通道归属和外设时序，**新工程通常不推荐**。ADC N 点采集用 ADC DMA，DAC 连续输出用 DAC DMA；这两类复杂模块才负责完整协作链。

## 1. 模块作用

描述并校验 DMA 传输，再交给平台适配器执行。

## 2. 输入

输入由公开头文件中的指针、长度、配置结构或平台回调给出；所有单位写在字段名中。

## 3. 输出

输出写入调用者提供的结果/缓冲区，并返回 `signal_result_t`；不通过隐藏全局变量传递数据。

## 4. 依赖

`signal_status.h`。

## 5. SysConfig 设置

新手详细配置：[DMA 教程](../../00_docs/SYSCONFIG_BEGINNER_GUIDE.md#dma)。本模块重点检查 Channel owner、Source/Destination 方向、Half Word/Word 宽度、地址递增、Single/Repeat 和完成通知。现场速查见 [Quick Reference](../../00_docs/SYSCONFIG_QUICK_REFERENCE.md)。

通用代码不绑定 SysConfig 实例名；接到 MSPM0 时由独立平台适配器使用生成宏。当前状态不代表对应外设已实板验证。

## 6. 初始化方法

模块不做隐式全局初始化。包含 `signal_dma.h`，由调用者准备配置、缓冲区或平台回调；如头文件提供 Init/Configure，先调用它。

## 7. 调用方法

`SignalDMA_ValidateTransfer`、`SignalDMA_Start`、`SignalDMA_Stop`、`SignalDMA_GetModuleStatus`。

## 8. 参数修改方法

只修改调用者配置结构、count/capacity 和采样率等函数参数；不要为某个 Demo 改底层 `.c`。

## 9. 与其他模块如何连接

通过 `signal_types.h` 的数组+长度+采样率语义或本模块公开结构连接；先检查返回码再消费输出。

## 10. 最小示例

~~~c
#include "signal_dma.h"

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

**与上层模块的关系：** 这是低层搬运 building block；采集/输出优先选 ADC DMA 或 DAC DMA。

以下声明来自真实公开头文件；源码没有说明的项保留 `UNKNOWN / NOT EXPOSED`。

### `signal_result_t SignalDMA_ValidateTransfer(const signal_dma_transfer_t *transfer);`

- **作用：** UNKNOWN / NOT EXPOSED

| 参数 | 真实类型 | 真实说明 |
|---|---|---|
| `transfer` | `const signal_dma_transfer_t *` | UNKNOWN / NOT EXPOSED |

- **返回：** UNKNOWN / NOT EXPOSED
- **调用前/后：** UNKNOWN / NOT EXPOSED

```c
signal_result_t result_value = SignalDMA_ValidateTransfer(transfer);
```

### `signal_result_t SignalDMA_Start(const signal_dma_t *dma, const signal_dma_transfer_t *transfer);`

- **作用：** UNKNOWN / NOT EXPOSED

| 参数 | 真实类型 | 真实说明 |
|---|---|---|
| `dma` | `const signal_dma_t *` | UNKNOWN / NOT EXPOSED |
| `transfer` | `const signal_dma_transfer_t *` | UNKNOWN / NOT EXPOSED |

- **返回：** UNKNOWN / NOT EXPOSED
- **调用前/后：** UNKNOWN / NOT EXPOSED

```c
signal_result_t result_value = SignalDMA_Start(dma, transfer);
```

### `signal_result_t SignalDMA_Stop(const signal_dma_t *dma);`

- **作用：** UNKNOWN / NOT EXPOSED

| 参数 | 真实类型 | 真实说明 |
|---|---|---|
| `dma` | `const signal_dma_t *` | UNKNOWN / NOT EXPOSED |

- **返回：** UNKNOWN / NOT EXPOSED
- **调用前/后：** UNKNOWN / NOT EXPOSED

```c
signal_result_t result_value = SignalDMA_Stop(dma);
```

### `signal_module_status_t SignalDMA_GetModuleStatus(void);`

- **作用：** UNKNOWN / NOT EXPOSED

参数：无。

- **返回：** UNKNOWN / NOT EXPOSED
- **调用前/后：** UNKNOWN / NOT EXPOSED

```c
signal_module_status_t result_value = SignalDMA_GetModuleStatus();
```

## 18. Call Sequence / Connecting / Buffer Rules

```text
SignalDMA_ValidateTransfer -> SignalDMA_Start -> SignalDMA_Stop -> SignalDMA_GetModuleStatus
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

DMA callbacks 存在是因为 channel、trigger 与 address mode 属于平台资源。正式通用实现为 `SignalMSPM0G3507_DMA_*`：设置 source/destination/transfer size/width/increment 并启停 channel。`transfer_count` 是元素数且当前 MSPM0 adapter 限制为最多 65535；width 为 1/2/4 byte；increment 只接受 `-1`、`0`、`+1`。

触发源、channel transfer mode 与 event route 不能由这份通用 transfer 描述推断，必须在 SysConfig 配好。ADC DMA/DAC DMA 普通应用优先使用各自已经闭环的专用模块，不要手拼通用 DMA。

## Copy Into Target Project

链接 BSP DMA 与统一平台 `.c`；Include 加 DMA、common 和平台目录。初始化 `signal_mspm0g3507_dma_context_t { DMA, generated_channel_id }` 后调用 `SignalMSPM0G3507_DMA_Bind()`。SysConfig 参考 ADC 的 `PROFILE_01_ADC_CAPTURE` 或 DAC 的 `PROFILE_03_DAC_GENERATOR`，但 source/destination width 与 increment 必须和 transfer 一致。

## Hardware / Platform Binding

- Platform：[MSPM0G3507 Platform Adapter](../../08_applications/common/mspm0g3507/README.md)，正式文件 `signal_mspm0g3507_platform.h/.c`。
- `SignalMSPM0G3507_DMA_Bind` 提供 configure/start/stop callbacks；trigger、mode、channel ownership 仍由 SysConfig 决定。
- SysConfig：ADC 参考 `PROFILE_01_ADC_CAPTURE`，DAC 参考 `PROFILE_03_DAC_GENERATOR`。
- 【COMPILE-VERIFIED EXAMPLE】：[`adc_dma_minimum/main.c`](../../09_examples/platform_closure/adc_dma_minimum/main.c)；DAC 方向另见 `dac_dma_minimum`。
