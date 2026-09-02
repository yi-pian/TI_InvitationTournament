# gpio

## 你真的需要这个模块吗？

**普通 GPIO 拉高、拉低、翻转、读电平不需要本模块。** 新工程直接用 SysConfig + `DL_GPIO_*`；需要消抖事件时选 Button，需要行列扫描时选 Matrix Keypad。见 [TI DriverLib 初学者指南](../../00_docs/TI_DRIVERLIB_BEGINNER_GUIDE.md)。

## 30 秒拼装路线

1. 新工程 [LINK] 无：不链接本目录旧 callback wrapper；signal_gpio.c/.h 仅 [REFERENCE ONLY]，禁止复制。
2. [GENERATED] 目标 .syscfg 生成 ti_msp_dl_config.*；在 SysConfig 添加 GPIO、方向/上下拉/初值和 Pin。
3. main include ti_msp_dl_config.h；SYSCFG_DL_init() 后直接调用 DL_GPIO_setPins/clearPins/togglePins/readPins。
4. P07 可对照 PA12 output；换 Pin 后只使用新生成的 PORT/PIN 宏。
5. 结果是实际 GPIO 电平；输入读取返回 bit mask，不是 bool。
6. Clean → Build；LED/万用表/逻辑分析仪观察一次翻转。

## 第一次把本模块加入母版工程

### STEP 1～4：加入方式、SysConfig 和参数

- [LINK] 无，[COPY] 无，[GENERATED] ti_msp_dl_config.*，[REFERENCE ONLY] P07 与 gpio_minimum。
- projectspec 不增加 BSP GPIO 源。打开母版 .syscfg，添加 GPIO instance，选择 Input/Output、pull、initial value、合法 Pin；保存生成宏。
- 可换合法 GPIO Pin；换 Pin 不改 DriverLib 代码，只改 .syscfg。方向/上下拉/中断边沿改变时同步电路接法。输出 Pin 不直接驱动超额负载。

### STEP 5～10：main、调用、结果与连接

~~~c
#include "ti_msp_dl_config.h"
int main(void)
{
    SYSCFG_DL_init();
    DL_GPIO_setPins(SIGNAL_GPIO_PORT, SIGNAL_GPIO_OUTPUT_PIN);
    while (1) { __WFI(); }
}
~~~

初始化后把 P07 的 PA12 置高。连接：GPIO output→LED/片选；GPIO input→普通按键；多根 GPIO→4×4 Keypad；SPI+GPIO→ILI9341。上层 Button/Keypad/TFT 仍按各自 README 加正式模块，不恢复旧 callback。

### STEP 11～12：Build 与最小验证

header/宏不存在=SysConfig 未生成或实例名不同；引脚不动=方向/PinMux/接线错误；读值反常=上下拉或有效电平错误。保存 SysConfig → Clean → Build；参考 09_examples/platform_closure/gpio_minimum，测 PA12 高电平。

## 根据题目修改参数

经常改 Pin、方向、pull、active level；偶尔改 drive/interrupt edge；通常不要改生成文件或恢复 BSP callback。

## 比赛现场最常改的地方

只先改 .syscfg 中的 Pin/方向/上下拉，再改应用使用的生成宏；其他底层设置先不动。

## 从母版到成功调用：完整例子

上面的 main.c + P07 GPIO 配置就是完整闭环：母版→SysConfig→generated macros→PA12 置高→仪器验证。

## MSPM0G3507 比赛推荐方式

SysConfig 配好 Pin、方向、上下拉与初值后直接调用：`DL_GPIO_setPins()`、`DL_GPIO_clearPins()`、`DL_GPIO_togglePins()`、`DL_GPIO_readPins()`。本目录仅把这些调用再包进 callback/descriptor，**新 MSPM0G3507 工程通常不推荐**。只有 Button 消抖、矩阵键盘扫描、TFT 等上层状态/协议模块仍值得复用。

可编译例子：[gpio_minimum](../../09_examples/platform_closure/gpio_minimum/README.md)。

## 1. 模块作用

用回调接口隔离具体 GPIO 实例，提供读、写、翻转操作。

## 2. 输入

输入由公开头文件中的指针、长度、配置结构或平台回调给出；所有单位写在字段名中。

## 3. 输出

输出写入调用者提供的结果/缓冲区，并返回 `signal_result_t`；不通过隐藏全局变量传递数据。

## 4. 依赖

`signal_status.h`。

## 5. SysConfig 设置

新手详细配置：[PinMux / GPIO 教程](../../00_docs/SYSCONFIG_BEGINNER_GUIDE.md#pinmux)。本模块重点检查 Input/Output、initial value、pull-up/pull-down、drive strength、interrupt edge 和 Pin owner。现场速查见 [Quick Reference](../../00_docs/SYSCONFIG_QUICK_REFERENCE.md)。

通用代码不绑定 SysConfig 实例名；接到 MSPM0 时由独立平台适配器使用生成宏。当前状态不代表对应外设已实板验证。

## 6. 初始化方法

模块不做隐式全局初始化。包含 `signal_gpio.h`，由调用者准备配置、缓冲区或平台回调；如头文件提供 Init/Configure，先调用它。

## 7. 调用方法

`SignalGPIO_Write`、`SignalGPIO_Read`、`SignalGPIO_Toggle`、`SignalGPIO_GetModuleStatus`。

## 8. 参数修改方法

只修改调用者配置结构、count/capacity 和采样率等函数参数；不要为某个 Demo 改底层 `.c`。

## 9. 与其他模块如何连接

通过 `signal_types.h` 的数组+长度+采样率语义或本模块公开结构连接；先检查返回码再消费输出。

## 10. 最小示例

~~~c
#include "signal_gpio.h"

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

以下声明来自真实公开头文件；源码没有说明的项保留 `UNKNOWN / NOT EXPOSED`。

### `signal_result_t SignalGPIO_Write(const signal_gpio_port_t *port, uint32_t pin, bool high);`

- **作用：** UNKNOWN / NOT EXPOSED

| 参数 | 真实类型 | 真实说明 |
|---|---|---|
| `port` | `const signal_gpio_port_t *` | UNKNOWN / NOT EXPOSED |
| `pin` | `uint32_t` | UNKNOWN / NOT EXPOSED |
| `high` | `bool` | UNKNOWN / NOT EXPOSED |

- **返回：** UNKNOWN / NOT EXPOSED
- **调用前/后：** UNKNOWN / NOT EXPOSED

```c
signal_result_t result_value = SignalGPIO_Write(port, pin, high);
```

### `signal_result_t SignalGPIO_Read(const signal_gpio_port_t *port, uint32_t pin, bool *high);`

- **作用：** UNKNOWN / NOT EXPOSED

| 参数 | 真实类型 | 真实说明 |
|---|---|---|
| `port` | `const signal_gpio_port_t *` | UNKNOWN / NOT EXPOSED |
| `pin` | `uint32_t` | UNKNOWN / NOT EXPOSED |
| `high` | `bool *` | UNKNOWN / NOT EXPOSED |

- **返回：** UNKNOWN / NOT EXPOSED
- **调用前/后：** UNKNOWN / NOT EXPOSED

```c
signal_result_t result_value = SignalGPIO_Read(port, pin, high);
```

### `signal_result_t SignalGPIO_Toggle(const signal_gpio_port_t *port, uint32_t pin);`

- **作用：** UNKNOWN / NOT EXPOSED

| 参数 | 真实类型 | 真实说明 |
|---|---|---|
| `port` | `const signal_gpio_port_t *` | UNKNOWN / NOT EXPOSED |
| `pin` | `uint32_t` | UNKNOWN / NOT EXPOSED |

- **返回：** UNKNOWN / NOT EXPOSED
- **调用前/后：** UNKNOWN / NOT EXPOSED

```c
signal_result_t result_value = SignalGPIO_Toggle(port, pin);
```

### `signal_module_status_t SignalGPIO_GetModuleStatus(void);`

- **作用：** UNKNOWN / NOT EXPOSED

参数：无。

- **返回：** UNKNOWN / NOT EXPOSED
- **调用前/后：** UNKNOWN / NOT EXPOSED

```c
signal_module_status_t result_value = SignalGPIO_GetModuleStatus();
```

## 18. Call Sequence / Connecting / Buffer Rules

```text
SignalGPIO_Write -> SignalGPIO_Read -> SignalGPIO_Toggle -> SignalGPIO_GetModuleStatus
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

GPIO callbacks 存在是为了让 Button、Keypad 与 UI 不写死 GPIOA/GPIOB。正式实现为平台层的 `SignalMSPM0G3507_GPIO_Write/Read/Toggle`，分别调用 `DL_GPIO_setPins/clearPins/readPins/togglePins`；用户不写 callback。`pin` 必须传 `DL_GPIO_PIN_x` bit mask，不是数字 x。

## Copy Into Target Project

链接 `01_bsp/gpio/signal_gpio.c` 与 `08_applications/common/mspm0g3507/signal_mspm0g3507_platform.c`；Include 加 GPIO、common、平台、SDK/CMSIS/生成目录。SysConfig 先把目标 pin 配成输入或输出，再 `SignalMSPM0G3507_GPIO_Bind(&port, GPIOA)`。PinMux 与上拉/初值仍以生成的 `ti_msp_dl_config.h` 为准。

## Hardware / Platform Binding

- Platform：[MSPM0G3507 Platform Adapter](../../08_applications/common/mspm0g3507/README.md)
- 头/源文件：`signal_mspm0g3507_platform.h/.c`
- 绑定：`SignalMSPM0G3507_GPIO_Bind` 填入 read/write/toggle callbacks。
- SysConfig：`PROFILE_07_BASIC_IO` 的 `SIGNAL_GPIO/OUTPUT` 为 PA12 输出。
- 【COMPILE-VERIFIED EXAMPLE】：[`gpio_minimum/main.c`](../../09_examples/platform_closure/gpio_minimum/main.c)
