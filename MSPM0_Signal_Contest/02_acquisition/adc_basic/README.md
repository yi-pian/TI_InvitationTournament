# adc_basic

## 你真的需要这个模块吗？

**普通比赛新工程不推荐本旧组合层。** 单点通道验证直接使用 SysConfig + ADC DriverLib；固定 `Fs` 的 `raw[N]` 使用 ADC DMA。见 [TI DriverLib 初学者指南](../../00_docs/TI_DRIVERLIB_BEGINNER_GUIDE.md)。

## 30 秒拼装路线

1. 新工程不链接本目录旧 wrapper：`signal_adc_basic.c/.h` 标为 `[REFERENCE ONLY]`；`[COPY]` 无。
2. `[GENERATED]` 目标工程 `.syscfg` 生成的 `ti_msp_dl_config.c/.h`；main 只 include `ti_msp_dl_config.h`。
3. SysConfig 对照 P07：ADC0 MEM0、software trigger、channel 2/PA25、12 bit、参考源与中断标志。
4. `SYSCFG_DL_init()` 后清 MEM0 result-loaded 标志、启动 conversion、等待标志、读 MEM0。
5. 结果 `volatile uint16_t g_adc_raw` 是 ADC code，不是 V；接 ADC To Voltage 才得到电压。
6. Clean → Build；输入 GND/已知电压，确认 raw 随输入变化。要 N 点稳定帧直接换 ADC DMA。

## 第一次把本模块加入母版工程

### STEP 1～4：文件、CCS、SysConfig 与参数

- `[LINK]` 无正式模块源：简单单点读取采用 SDK DriverLib；不要复制 `signal_adc_basic.c`。
- projectspec 保留母版 SDK include 与 `.syscfg`；若母版没有 ADC，在 SysConfig 添加 ADC12、选择 software trigger、MEM0 和合法 ADC Pin。参考 `[REFERENCE ONLY]`：`09_examples/integration_profiles/PROFILE_07_BASIC_IO/profile.syscfg`。
- 可换 ADC instance/channel/Pin；换后只使用重新生成的 `SIGNAL_BASIC_ADC_*` 宏。VREF/分辨率改变后同步 ADC To Voltage。不要手改 `ti_msp_dl_config.*`。

| 题目要求 | 在哪里改 | 影响/同步项 |
|---|---|---|
| 输入接线 | `.syscfg` ADC channel/PinMux | 板外信号接新 Pin，共地 |
| 电压量程 | `.syscfg` reference/resolution | 同步 ToVoltage config |
| 多点/固定 Fs | 不继续堆单点循环 | 换 ADC DMA，设置 Fs/N |

### STEP 5～10：main、调用、结果与连接

```c
#include <stdint.h>
#include "ti_msp_dl_config.h"
volatile uint16_t g_adc_raw;

int main(void)
{
    SYSCFG_DL_init();
    DL_ADC12_clearInterruptStatus(SIGNAL_BASIC_ADC_INST,
        DL_ADC12_INTERRUPT_MEM0_RESULT_LOADED);
    DL_ADC12_startConversion(SIGNAL_BASIC_ADC_INST);
    while (DL_ADC12_getRawInterruptStatus(SIGNAL_BASIC_ADC_INST,
               DL_ADC12_INTERRUPT_MEM0_RESULT_LOADED) == 0U) {}
    g_adc_raw = DL_ADC12_getMemResult(
        SIGNAL_BASIC_ADC_INST, SIGNAL_BASIC_ADC_ADCMEM_0);
    while (1) { __WFI(); }
}
```

`SYSCFG_DL_init` 先让 ADC/Pin/clock 生效；clear 避免读到旧完成标志；start 开始一次转换；轮询等待真实完成；getMemResult 读取 code。常见连接：单点 raw→阈值判断；单点 raw→ADC To Voltage；需要 `raw[N]`→ADC DMA。

### STEP 11～12：Build 与最小验证

保存 SysConfig → Clean → Build。头文件/宏不存在通常是 SysConfig 未生成或实例名不一致；undefined `DL_ADC12_*` 通常是 SDK include/link 配置损坏；raw 不变先查 Pin/channel/VREF 和共地。最小验证工程：`09_examples/platform_closure/adc_basic_minimum`。

## 比赛现场最常改的地方

经常改 ADC Pin/channel、reference/resolution；偶尔改采样时间；通常不要改 DriverLib 调用顺序。单点变成波形采集时直接换 ADC DMA。

## 从母版到成功调用：完整例子

上面的完整 `main.c` 就是母版最小闭环：母版 → P07 ADC SysConfig → generated config → software conversion → `g_adc_raw` → Build/观察。

## MSPM0G3507 比赛推荐方式

单次/少量 bring-up 读取直接用 SysConfig + `DL_ADC12_startConversion()` + `DL_ADC12_getMemResult()`；要 `raw[N]` 使用 ADC DMA。本阻塞 block wrapper 依赖旧 BSP ADC callback，**新工程通常不推荐**。直接例子：[adc_basic_minimum](../../09_examples/platform_closure/adc_basic_minimum/README.md)。

## 1. 模块作用

用阻塞读接口采集一段 ADC 原始码，适合最小验证。

## 2. 输入

输入由公开头文件中的指针、长度、配置结构或平台回调给出；所有单位写在字段名中。

## 3. 输出

输出写入调用者提供的结果/缓冲区，并返回 `signal_result_t`；不通过隐藏全局变量传递数据。

## 4. 依赖

`signal_adc.h`、`signal_status.h`。

## 5. SysConfig 设置

新手详细配置：[软件触发单次 ADC 教程](../../00_docs/SYSCONFIG_BEGINNER_GUIDE.md#adc)。本模块重点使用 ADC instance、ADCMEM Input Channel、Reference、Resolution、Sample Time、Software Trigger 和 PinMux；不需要 Timer/DMA。现场速查见 [Quick Reference](../../00_docs/SYSCONFIG_QUICK_REFERENCE.md)。

通用代码不绑定 SysConfig 实例名；接到 MSPM0 时由独立平台适配器使用生成宏。当前状态不代表对应外设已实板验证。

## 6. 初始化方法

模块不做隐式全局初始化。包含 `signal_adc_basic.h`，由调用者准备配置、缓冲区或平台回调；如头文件提供 Init/Configure，先调用它。

## 7. 调用方法

`SignalADCBasic_ReadBlock`、`SignalADCBasic_GetModuleStatus`。

## 8. 参数修改方法

只修改调用者配置结构、count/capacity 和采样率等函数参数；不要为某个 Demo 改底层 `.c`。

## 9. 与其他模块如何连接

通过 `signal_types.h` 的数组+长度+采样率语义或本模块公开结构连接；先检查返回码再消费输出。

## 10. 最小示例

~~~c
#include "signal_adc_basic.h"

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

以下内容从正式 `.h` 和现有 README 整理；未公开说明的字段明确标为 `UNKNOWN / NOT EXPOSED`。

### `signal_result_t SignalADCBasic_ReadBlock(const signal_adc_t *adc, uint16_t *destination, size_t sample_count);`

- **作用：** UNKNOWN / NOT EXPOSED

| 参数 | 真实类型 | 真实说明 |
|---|---|---|
| `adc` | `const signal_adc_t *` | UNKNOWN / NOT EXPOSED |
| `destination` | `uint16_t *` | UNKNOWN / NOT EXPOSED |
| `sample_count` | `size_t` | UNKNOWN / NOT EXPOSED |

- **返回：** UNKNOWN / NOT EXPOSED
- **调用前/后：** UNKNOWN / NOT EXPOSED

```c
signal_result_t result_value = SignalADCBasic_ReadBlock(adc, destination, sample_count);
```

### `signal_module_status_t SignalADCBasic_GetModuleStatus(void);`

- **作用：** UNKNOWN / NOT EXPOSED

参数：无。

- **返回：** UNKNOWN / NOT EXPOSED
- **调用前/后：** UNKNOWN / NOT EXPOSED

```c
signal_module_status_t result_value = SignalADCBasic_GetModuleStatus();
```

## 18. Call Sequence / Connecting / Buffer Rules

```text
SignalADCBasic_ReadBlock -> SignalADCBasic_GetModuleStatus
```

按模块角色选择实际所需 API：Init/Validate 先于 Start/Process/Generate，Get/Is 在执行后，Stop 在取消或退出时。所有返回码先检查；buffer 由调用者创建和持有，capacity 按元素数；运行中的 DMA/回调 buffer 不得并发改写。模块不动态分配。

## 19. Config vs SysConfig / Resources / Verification

- 原 SysConfig 说明：通用代码不绑定 SysConfig 实例名；接到 MSPM0 时由独立平台适配器使用生成宏。当前状态不代表对应外设已实板验证。
- 软件参数和数组长度为 CONFIG ONLY；真实 pin、peripheral、clock、Timer、DMA、Event、IRQ、reference 为 SYSCONFIG REQUIRED。
- RAM 由结构体与调用者 buffer 决定；Flash/Stack 看应用 `.map`。
- 用已知输入验证边界和返回码，再接真实 profile；未实板不得写 BOARD_VERIFIED。

## 20. Quick Modify Table

| 我想改什么 | 去哪里 | 会影响什么 | SysConfig? |
|---|---|---|---|
| count/capacity | Application buffer + API | RAM、处理长度、响应 | 否 |
| rate/frequency | Application config；若为真实外设率再改 profile | 时间轴、Nyquist或输出频率 | 视硬件而定 |
| threshold/gain/offset | 公开 config/API | 灵敏度、量程或偏置 | 通常否；硬件前端变化另算 |
| pin/instance/channel | `.syscfg` | 物理连线和资源冲突 | 是 |

常见错误：不检查返回码、byte/element 混用、对象生命周期不足、把请求速率当配置/实测速率、修改硬件资源后未重新生成 SysConfig。

## Integration Closure

```text
ADC Basic -> BSP ADC -> SignalMSPM0G3507_ADC_Read
          -> software-trigger ADC12 -> MEM0 raw code
```

- 正式源码：`02_acquisition/adc_basic/signal_adc_basic.c`、`01_bsp/adc/signal_adc.c`。
- Platform Adapter：`08_applications/common/mspm0g3507/signal_mspm0g3507_platform.c`。
- SysConfig：必须是 software trigger；参考 `PROFILE_07_BASIC_IO` 的 ADC0 MEM0/channel 2/PA25/12-bit/VDDA。
- callback 不由用户编写。Adapter 启动转换、轮询 `DL_ADC12_INTERRUPT_MEM0_RESULT_LOADED`、读取 `DL_ADC12_getMemResult()`，并带有限 timeout。
- 输出是 `uint16_t raw[]` ADC code，不是 V；后接 ADC To Voltage 时传 12-bit 与同一 VREF。

## Copy Into Target Project

链接 ADC Basic、BSP ADC 和平台三个 `.c`；Include Path 加三个模块目录、`01_bsp/common`、SDK/CMSIS 与 SysConfig 生成目录。完整调用见 `09_examples/platform_closure/adc_basic_minimum/main.c`。不要把 `PROFILE_01_ADC_CAPTURE` 用于此 adapter：它是 event trigger，不是软件触发。

## Hardware / Platform Binding

- Platform：[MSPM0G3507 Platform Adapter](../../08_applications/common/mspm0g3507/README.md)，文件 `signal_mspm0g3507_platform.h/.c`。
- 绑定：先用 `SignalMSPM0G3507_ADC_Bind` 构造 `signal_adc_t`，再交给 `SignalADCBasic_ReadBlock`。
- SysConfig：`PROFILE_07_BASIC_IO`，不能换成 event-trigger 的 Profile 01。
- 【COMPILE-VERIFIED EXAMPLE】：[`adc_basic_minimum/main.c`](../../09_examples/platform_closure/adc_basic_minimum/main.c)
