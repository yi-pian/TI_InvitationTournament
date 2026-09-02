# uart

## CCS SysConfig GUI Configuration

### Required resources

SysConfig module 是 `UART`，硬件 instance 示例为 `UART0`，`DL_UART_Main_*` 是 DriverLib C 名称。外部终端还需要 TX/RX PinMux 和共地；DMA/IRQ 仅在异步或高吞吐方案需要。

### Step 1 - UART instance and pins

GUI Path: `Add` -> `UART` -> `SIGNAL_UART` -> UART instance/PinMux 页面。

Action: 在 UART 实例页面依次展开 `Basic Configuration` -> `UART Peripheral`，选择硬件实例（P07 示例为 `UART0`）；再展开 `PinMux Peripheral and Pin Configuration`，把 `TX` 分配到 `PA10`、`RX` 分配到 `PA11`。若当前工程使用其他引脚，以 PinMux 下拉框中可用且未冲突的模拟/数字引脚为准。

### Step 2 - Clock and baud

GUI Path: `SIGNAL_UART` -> UART clock/baud section。

Set：在 `Basic Configuration` -> `Clock Configuration` 选择 `UART Clock Source = BUSCLK`，在 `Baud Rate Configuration` 填 `Target Baud Rate = 115200`；在 `Data Format` 选择 `8 Data Bits`、`No Parity`、`1 Stop Bit`；在 `FIFO Configuration` 勾选 `Enable FIFO`，需要回环测试时才在 `Loopback` 中勾选 `Internal Loopback`，接外部 USB-UART 时保持关闭。以页面右侧 `Calculated Baud` 和实际误差提示为最终结果。

共享教材：[MSPM0G3507 SysConfig 时钟、Timer、ADC 与 DAC 保姆教程](../../00_docs/MSPM0_SYSCONFIG_CLOCK_TIMER_ADC_DAC_BEGINNER_GUIDE.md)。UART 波特率依赖 UART/system clock，必须以 GUI 的 calculated/actual baud 为准，不能只凭 `BUSCLK` 或 `CPUCLK_FREQ` 心算。

### Step 3 - External terminal check

Action: 外部串口必须关闭 Internal Loopback；USB-UART RX 接 MCU TX、USB-UART TX 接 MCU RX、GND 共地。若当前 GUI 显示对应 loopback 字段，请设置为 disabled；字段页面仍需截图确认。

### Expected generated symbols

Generate 后核对 `SIGNAL_UART_INST`、UART IRQ/FIFO 相关宏（若启用）以及 TX/RX Pin 相关宏。PROJECT_AUDIT 记录 `GUI field -> .syscfg property -> generated symbol`；不要用 `DL_UART_Main_transmitDataBlocking` 反推 GUI 字段。

### GUI verification

保存 `.syscfg` 后点击 SysConfig 的生成按钮，重新打开 `UART0` 实例，逐项复核 `Clock Configuration`、`Baud Rate Configuration`、`Data Format`、`FIFO Configuration`、`Loopback` 和 `PinMux Peripheral and Pin Configuration`。再在生成的 `ti_msp_dl_config.h` 中核对 `SIGNAL_UART_INST` 及 TX/RX 宏；只要任一字段改变，就重新 Generate、Clean 和 Build。

### Final checklist / Common mistakes / Do not change

- UART module、UART0 instance、TX/RX pins 和 DriverLib 名称已分开。
- GUI 实际 calculated baud 与 PC 终端设置一致。
- 不把内部回环误认为外部串口连通；不直接编辑 `.syscfg`/生成文件。

## 你真的需要这个模块吗？

**发送少量调试字节/文本不需要旧 wrapper。** 新工程用 SysConfig + `DL_UART_Main_transmitDataBlocking()`；只有异步缓冲、协议状态机或复杂通信才值得建立更高层驱动。见 [TI DriverLib 初学者指南](../../00_docs/TI_DRIVERLIB_BEGINNER_GUIDE.md)。

## 30 秒拼装路线

1. 新工程 [LINK] 无：旧 signal_uart.c/.h 仅 [REFERENCE ONLY]；禁止复制。
2. [GENERATED] SysConfig ti_msp_dl_config.*；P07 默认 UART0 TX PA10、RX PA11、115200 8-N-1。
3. main include ti_msp_dl_config.h，SYSCFG_DL_init() 后逐字节调用 DL_UART_Main_transmitDataBlocking。
4. TX 接 USB-UART RX、RX 接 USB-UART TX、GND 共地；使用 3.3 V TTL，不是 RS-232。
5. 输出结果在串口终端，字节发送不会自动完成 printf 格式化。
6. Clean → Build；先发送 HELLO。

## 第一次把本模块加入母版工程

### STEP 1～4：加入方式、SysConfig、引脚与参数

- [LINK]/[COPY] 无；[GENERATED] ti_msp_dl_config.*；[REFERENCE ONLY] P07 与 uart_minimum。
- projectspec 不增加 BSP UART 源。在母版 .syscfg 添加 UART，设置 instance、baud、8-N-1、FIFO、TX/RX Pin；保存生成。
- 改 baud/Pin 后同步 PC 串口参数和接线。UART0/PA10/PA11 是 P07 默认，不代表任意 Pin 都能复用。

### STEP 5～10：main、调用、结果与连接

~~~c
#include <stddef.h>
#include <stdint.h>
#include "ti_msp_dl_config.h"
static const char g_message[] = "HELLO\r\n";
int main(void)
{
    size_t i;
    SYSCFG_DL_init();
    for (i = 0U; i < sizeof(g_message) - 1U; ++i) {
        DL_UART_Main_transmitDataBlocking(
            SIGNAL_UART_INST, (uint8_t) g_message[i]);
    }
    while (1) { __WFI(); }
}
~~~

初始化配置 UART/Pin；循环逐字节阻塞发送；结果在 PC terminal。连接：测量 result→格式化文本→UART；调试状态→UART；命令字节→应用解析。需要异步 ring buffer 时再选专用复杂模块。

### STEP 11～12：Build 与最小验证

宏不存在=UART 实例没叫 SIGNAL_UART；乱码=baud/时钟/8-N-1 不一致；无输出=TX/RX 接反、未共地或选错 COM。保存 SysConfig → Clean → Build，按 uart_minimum 看 HELLO。

## 比赛现场最常改的地方

经常改 baud、TX/RX Pin、发送文本；偶尔改 FIFO/interrupt；通常不要改生成 UART 初始化或使用旧 callback。

## 从母版到成功调用：完整例子

上面的完整 main.c + P07 UART 配置即母版到终端输出的闭环。

## MSPM0G3507 比赛推荐方式

少量调试文本直接在 SysConfig 配 UART 后循环调用 `DL_UART_Main_transmitDataBlocking()`。本目录只在 byte stream 外加 callback/descriptor，**新 MSPM0G3507 工程通常不推荐**；若以后确实需要 ring buffer、协议解析或异步队列，应由对应复杂模块提供，而不是使用本薄包装。

可编译例子：[uart_minimum](../../09_examples/platform_closure/uart_minimum/README.md)。

## 1. 模块作用

统一字节流收发接口，隔离 XDS110 UART 或其他串口实现。

## 2. 输入

输入由公开头文件中的指针、长度、配置结构或平台回调给出；所有单位写在字段名中。

## 3. 输出

输出写入调用者提供的结果/缓冲区，并返回 `signal_result_t`；不通过隐藏全局变量传递数据。

## 4. 依赖

`signal_status.h`。

## 5. SysConfig 设置

新手详细配置：[UART baud、TX/RX、Interrupt 与 DMA 教程](../../00_docs/SYSCONFIG_BEGINNER_GUIDE.md#uart)。本模块重点检查 Calculated Baud、8-N-1、TX/RX Pin；接外部终端必须关闭 Internal Loopback。现场速查见 [Quick Reference](../../00_docs/SYSCONFIG_QUICK_REFERENCE.md)。

通用代码不绑定 SysConfig 实例名；接到 MSPM0 时由独立平台适配器使用生成宏。当前状态不代表对应外设已实板验证。

## 6. 初始化方法

模块不做隐式全局初始化。包含 `signal_uart.h`，由调用者准备配置、缓冲区或平台回调；如头文件提供 Init/Configure，先调用它。

## 7. 调用方法

`SignalUART_Write`、`SignalUART_WriteString`、`SignalUART_Read`、`SignalUART_GetModuleStatus`。

## 8. 参数修改方法

只修改调用者配置结构、count/capacity 和采样率等函数参数；不要为某个 Demo 改底层 `.c`。

## 9. 与其他模块如何连接

通过 `signal_types.h` 的数组+长度+采样率语义或本模块公开结构连接；先检查返回码再消费输出。

## 10. 最小示例

~~~c
#include "signal_uart.h"

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

### `signal_result_t SignalUART_Write(const signal_uart_t *uart, const uint8_t *data, size_t count);`

- **作用：** UNKNOWN / NOT EXPOSED

| 参数 | 真实类型 | 真实说明 |
|---|---|---|
| `uart` | `const signal_uart_t *` | UNKNOWN / NOT EXPOSED |
| `data` | `const uint8_t *` | UNKNOWN / NOT EXPOSED |
| `count` | `size_t` | UNKNOWN / NOT EXPOSED |

- **返回：** UNKNOWN / NOT EXPOSED
- **调用前/后：** UNKNOWN / NOT EXPOSED

```c
signal_result_t result_value = SignalUART_Write(uart, data, count);
```

### `signal_result_t SignalUART_WriteString(const signal_uart_t *uart, const char *text);`

- **作用：** UNKNOWN / NOT EXPOSED

| 参数 | 真实类型 | 真实说明 |
|---|---|---|
| `uart` | `const signal_uart_t *` | UNKNOWN / NOT EXPOSED |
| `text` | `const char *` | UNKNOWN / NOT EXPOSED |

- **返回：** UNKNOWN / NOT EXPOSED
- **调用前/后：** UNKNOWN / NOT EXPOSED

```c
signal_result_t result_value = SignalUART_WriteString(uart, text);
```

### `signal_result_t SignalUART_Read(const signal_uart_t *uart, uint8_t *data, size_t capacity, size_t *received);`

- **作用：** UNKNOWN / NOT EXPOSED

| 参数 | 真实类型 | 真实说明 |
|---|---|---|
| `uart` | `const signal_uart_t *` | UNKNOWN / NOT EXPOSED |
| `data` | `uint8_t *` | UNKNOWN / NOT EXPOSED |
| `capacity` | `size_t` | UNKNOWN / NOT EXPOSED |
| `received` | `size_t *` | UNKNOWN / NOT EXPOSED |

- **返回：** UNKNOWN / NOT EXPOSED
- **调用前/后：** UNKNOWN / NOT EXPOSED

```c
signal_result_t result_value = SignalUART_Read(uart, data, capacity, received);
```

### `signal_module_status_t SignalUART_GetModuleStatus(void);`

- **作用：** UNKNOWN / NOT EXPOSED

参数：无。

- **返回：** UNKNOWN / NOT EXPOSED
- **调用前/后：** UNKNOWN / NOT EXPOSED

```c
signal_module_status_t result_value = SignalUART_GetModuleStatus();
```

## 18. Call Sequence / Connecting / Buffer Rules

```text
SignalUART_Write -> SignalUART_WriteString -> SignalUART_Read -> SignalUART_GetModuleStatus
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

UART callbacks 把字节流接口与 UART instance 解耦。正式实现 `SignalMSPM0G3507_UART_Write/Read` 位于统一平台层：TX 使用 `DL_UART_Main_transmitDataBlocking()`；RX 使用 `DL_UART_Main_receiveDataCheck()`，一次最多读取 capacity 字节，不会无限等待。用户不写 callback。

SysConfig 参考 `PROFILE_07_BASIC_IO`：UART0，PA10 TX、PA11 RX，115200 8-N-1，FIFO enable，`enableInternalLoopback=false`。如果不关闭 internal loopback，Build 虽通过但引脚不会形成正常外部串口链。

## Copy Into Target Project

链接 `01_bsp/uart/signal_uart.c` 和平台 `.c`，加入两个模块与 common 的 Include Path。`SYSCFG_DL_init()` 后调用 `SignalMSPM0G3507_UART_Bind(&uart, SIGNAL_UART_INST, SIGNAL_UART_BAUD_RATE)`。完整发送示例和真实 full-link 证据见 `09_examples/platform_closure/uart_minimum`。

## Hardware / Platform Binding

- Platform：[MSPM0G3507 Platform Adapter](../../08_applications/common/mspm0g3507/README.md)，正式文件 `signal_mspm0g3507_platform.h/.c`。
- `SignalMSPM0G3507_UART_Bind` 填入 blocking TX 与 bounded non-blocking RX callbacks。
- SysConfig：`PROFILE_07_BASIC_IO`，UART0 PA10/PA11、115200、FIFO、internal loopback off。
- 【COMPILE-VERIFIED EXAMPLE】：[`uart_minimum/main.c`](../../09_examples/platform_closure/uart_minimum/main.c)
