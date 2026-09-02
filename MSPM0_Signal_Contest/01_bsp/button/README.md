# 普通瞬时按键模块

## CCS SysConfig GUI Configuration

### Required resources

只需要 `GPIO`。时钟状态：`NO_INDEPENDENT_PERIPHERAL_CLOCK`；普通 GPIO 没有本教程需要配置的独立 peripheral clock，消抖扫描周期属于应用层调度。

### Step 1 - Add the input pin

GUI Path: 左侧 `Software` -> `Add` -> `GPIO` -> 实例 -> `Group Pins` -> `ADD`；在新行依次设置 `Name`、`Direction = Input`、`IO Structure`，名称和 Port/Pin 按当前工程接线填写。

### Step 2 - Electrical input configuration

GUI Path: 左侧 `GPIO` -> 对应 pin -> `Digital IOMUX Features` -> `Internal Resistor`，按原理图选择 `Pull-up` 或 `Pull-down`；若按键接地通常选 `Pull-up`。同页的 `Invert` 只在硬件有效电平与软件 callback 约定不一致时启用，避免重复反相。

### Step 3 - PinMux

GUI Path: pin -> `Assigned Port` / `Assigned Port Segment` / `Assigned Pin`，并展开 `PinMux Peripheral and Pin Configuration`。Action: 选择用户接线对应的 Port/Pin，保存后检查无冲突。

### Clock Configuration

`NO_INDEPENDENT_PERIPHERAL_CLOCK`。5 ms 等消抖调用周期来自主循环、任务或已有系统 tick；本模块不要求为 GPIO 新建 `TIMER`。时钟概念边界见[共享时钟教材](../../00_docs/MSPM0_SYSCONFIG_CLOCK_TIMER_ADC_DAC_BEGINNER_GUIDE.md)。

### Expected generated symbols

核对 GPIO group 的 `*_PORT`、按键 `*_PIN` 与 `*_IOMUX` 宏；真实前缀由 `Name` 决定。PROJECT_AUDIT 记录 `Direction/Internal Resistor/Assigned Pin -> .syscfg property -> generated symbol`。

### Final checklist / Common mistakes / Do not change

- 松开/按下电平与 callback 的 active-low 处理一致。
- 没有把应用层 debounce cadence 写成 GPIO clock，也没有无故添加 Timer。
- 不直接编辑 `.syscfg` 或生成文件，不占用当前工程固定 Pin。

## 你真的需要这个模块吗？

### MSPM0G3507 比赛推荐方式

- 只想知道引脚当前高/低：SysConfig 配 GPIO input/pull-up，直接 `DL_GPIO_readPins()`。
- 需要消抖以及“刚按下/刚松开”事件：继续使用本 Button 模块；这部分状态处理不是一行 DriverLib 能替代的。
- Button 读取 callback 只负责把 active-low 电平变成逻辑 pressed；不应为了单次读键强制引入整个通用 Platform Adapter。

## 1. 模块作用

读取一个普通瞬时按键，过滤机械触点抖动，并输出稳定状态、刚按下和刚松开事件。按住时状态为 pressed，松手后恢复 released。

小白解释：机械按键看起来只按了一次，电气触点却可能在几毫秒内快速通断很多次。本模块要求连续多次读到相同结果才确认，从而避免一次按键被程序当成多次。

它和自锁开关不同：普通按键不会自己保持，必须一直按住才是 ON。

## 2. 输入

- `read_pressed(context, &pressed)` 平台回调；回调必须把实际 GPIO 电平转换成逻辑 pressed。
- `debounce_scans`：连续多少次相同读数后才确认，必须大于 0。
- 应用提供固定调用周期，推荐每 5 ms 调用一次 `Update`。

常用 active-low 接法中，GPIO 低电平表示按下，因此回调应返回 `pressed = (pin_is_low)`。

## 3. 输出

每次 `SignalButton_Update()` 输出：

- `raw_pressed`：本次直接读到的逻辑状态；
- `stable_pressed`：消抖后的当前状态；
- `pressed`：本次刚确认按下，只出现一次；
- `released`：本次刚确认松开，只出现一次。

`SignalButton_GetPressed()` 可随时读取稳定状态。

## 4. 依赖

- `01_bsp/common/signal_status.h`；
- 1 个数字 GPIO 输入；
- 通常需要内部上拉；
- 不依赖 Timer、IRQ、DMA，也不动态分配内存。

## 5. SysConfig 设置

新手详细配置：[PinMux / GPIO 教程](../../00_docs/SYSCONFIG_BEGINNER_GUIDE.md#pinmux)。本模块独有设置是 Digital Input、与接法匹配的 pull-up/pull-down 和有效电平；改 Pin 后同步改接线。现场速查见 [Quick Reference](../../00_docs/SYSCONFIG_QUICK_REFERENCE.md)。

### 最推荐的两脚接法

```text
MSPM0 GPIO input ───── 按键 ───── GND
        │
        └── internal pull-up enabled
```

| 项目 | SysConfig 设置 |
|---|---|
| Pin function | Digital GPIO Input |
| Internal resistor | Pull-up |
| Interrupt | 不需要，先用周期轮询 |
| 逻辑 | 未按=高，按下=低 |

四脚轻触按键内部通常是“同一侧两脚永久相连、按下后两侧导通”。不要随便选相邻两脚；用万用表通断档找到按下前不通、按下后导通的两脚。

### LP-MSPM0G3507 两种示例

1. **不接线快速测试：** 使用板载 S2，真实 MCU pin 为 `PB21`。该按钮按下接地，应配置内部上拉，低电平表示按下。
2. **外接按键示例：** `PB1`（LaunchPad 40-pin 第 39 脚）接按键一端，另一端接 GND；PB1 配成带内部上拉的输入。

示例 pin 不是模块源码要求。若 PB1 已被其他功能占用，在 SysConfig 换任意安全空闲 GPIO，并只修改平台回调使用的生成宏。

## 6. 初始化方法

1. 在 SysConfig 配好 GPIO input + pull-up；
2. 调用 `SYSCFG_DL_init()`；
3. 写一个很薄的 `read_pressed` 回调，把低电平转换为 `true`；
4. 设置 `debounce_scans=3`；
5. 调用 `SignalButton_Init()`。

若上电时一直按住，连续达到消抖次数后会产生一次 `pressed` 事件。这适合实现“按住按键上电进入设置模式”。

## 7. 调用方法

推荐每 5 ms 调用一次：

```text
5 ms/update × debounce_scans 3 ≈ 15 ms 消抖时间
```

- 一次按下只执行一次动作：检查 `event.pressed`；
- 松开时执行动作：检查 `event.released`；
- 按住期间持续有效：检查 `event.stable_pressed`。

不要在无延时的 `while(1)` 中连续 Update，否则“三次确认”可能只过了几微秒。

## 8. 参数修改方法

- 改引脚：只改 `.syscfg` 和平台回调中的生成 pin 宏。
- 改有效电平：只改回调中的 high/low → pressed 映射。
- 改消抖时间：修改调用周期或 `debounce_scans`，实际时间约为二者乘积。
- 需要长按：应用对 `stable_pressed` 持续时间计数；底层模块不绑定某个比赛的长按阈值。
- 需要多个普通按键：为每个按键创建独立 `signal_button_t` 实例，不复制模块源码。

## 9. 与其他模块如何连接

```text
GPIO → read_pressed callback → Button debounce/event
                                  ↓
                       Application / TFT / DDS / Analyzer
```

按键事件属于 UI 控制，不是 ADC 或 FFT 数据。它通常切换页面、启动采样、确认参数或改变 DDS 设置。

## 10. 最小示例

~~~c
#include "signal_button.h"

static signal_result_t app_read_button(void *context, bool *pressed)
{
    (void)context;
    *pressed = (DL_GPIO_readPins(BUTTON_PORT, BUTTON_PIN) == 0U);
    return SIGNAL_RESULT_OK;
}

signal_button_t button;
const signal_button_config_t config = {
    .context = NULL,
    .read_pressed = app_read_button,
    .debounce_scans = 3U
};

SYSCFG_DL_init();
(void)SignalButton_Init(&button, &config);

/* 每 5 ms 执行一次。 */
signal_button_event_t event;
if (SignalButton_Update(&button, &event) == SIGNAL_RESULT_OK &&
    event.pressed) {
    /* 这里只执行一次“刚按下”的动作。 */
}
~~~

`BUTTON_PORT/BUTTON_PIN` 必须替换为你自己的 `.syscfg` 生成宏，不要照抄猜名字。

## 11. 常见错误

- GPIO 没有上拉：未按时输入悬空，程序随机触发。
- 四脚轻触按键接了同一侧两脚：两脚本来永久导通，按不按都一样。
- 把低电平直接当 false：模块回调要返回“是否按下”的逻辑值，active-low 时低应转换为 true。
- 使用 `stable_pressed` 做一次性菜单动作：按住期间会重复执行；应使用 `pressed`。
- Update 调用间隔不固定：消抖时间随主循环负载变化。
- 同时又开 GPIO 双边沿中断又轮询：容易出现两个 owner；入门阶段保留一种方式即可。

## 12. RAM 占用

动态分配 0。每个按键实例仅保存回调、三个布尔状态、计数和初始化标志，通常为几十字节；事件结构为几个布尔值。

## 13. Flash 占用

只有一次 GPIO 回调、状态比较和计数，不链接数学库。最终大小以应用 `.map` 为准；未使用函数可被链接器移除。

## 14. CPU 计算量估计

每次 Update 读取一次 GPIO并做常数次比较，计算量为 O(1)。即使 5 ms 扫描一次，CPU 占用也很低；不要为了这个模块使用高速中断。

## 15. 当前验证状态

`MODULE_STATUS_BUILD_VERIFIED`。PC mock 已覆盖消抖、按下和松开事件；TI Arm Clang `-Wall -Werror` 与 44 模块外设库聚合链接均已通过。本轮未读取真实按键，所以不能写 `BOARD_VERIFIED`。

## 16. 以后实板验证步骤

1. 断电接线，万用表确认按下时 GPIO 与 GND 导通。
2. 先打印 raw 状态，确认未按为高、按下为低。
3. 每 5 ms Update、3 次消抖，连续按 50 次，确认每次只有一个 pressed 和一个 released。
4. 测试短按、长按、快速连按以及按住上电。
5. 若用板载 S2，确认 SysConfig pin 为 PB21，且没有其他模块占用。
6. 保存测试记录后才可升级 `BOARD_VERIFIED`。

## 17. 统一 API 教程（已按当前头文件核对）

本节由当前公开头文件、实现中实际出现的状态码和正式模块注册表生成。它补充前文的场景教程；函数签名变化时必须重新运行 `tools/upgrade_formal_beginner_docs.ps1`，不要手工保留旧 API。

遵循仓库的 [Beginner README 标准](../../00_docs/BEGINNER_README_STANDARD.md)：先用最小示例完成一次正常数据流，再按需要阅读全功能示例和本节 API 细节。

### 什么时候用 / 什么时候不要用

当题目需要“button”目录对应的公开功能，并且输入数据、单位和硬件资源满足前文约束时使用本模块。若只需要更简单的上游功能、输入尚未准备好，或需要不同的数据模型/外设资源，应先选择相邻模块而不是强行调用本 API。

### 输入 / 输出

输入由各 API 的只读数组、配置、频率/阈值和平台对象组成；输出写入 result/output/buffer 参数或由 Get API 返回。调用者负责数组容量、生命周期和物理单位；失败返回时输出不是有效结果。

### 调用顺序

```text
SignalButton_Init -> SignalButton_Update -> SignalButton_GetPressed -> SignalButton_GetModuleStatus
```

`Init` 一般仅一次；`Set/Configure` 仅在参数变化且模块空闲时执行；`Start/Process/Generate` 是每帧或每次任务入口；`Get/Is` 用于读取已完成的结果；`Stop` 只在需要取消时调用。若本模块没有其中某类 API，以实际列出的函数为准。

### SysConfig 边界

本模块需要 SysConfig。先在 CCS 的 .syscfg 添加并核对：GPIO；再按前文的模块专用 GUI 步骤选择实际 pin/instance。保存后让 CCS 重新生成配置，核对生成宏；不要直接修改 	i_msp_dl_config.c/.h，也不要照抄示例 pin 或 DMA/Event 编号。

### 参数分级

- 【比赛必须会】输入/输出数组、`count/length/capacity`、采样率/频率、阈值/增益以及本 README 前文标出的 pin。它们直接影响题目范围、RAM、时间轴或物理单位。
- 【出问题再理解】Timer 时钟、DMA 通道、Event 路由、参考源和 IRQ。它们属于硬件链路，必须与 SysConfig 生成结果一致。
- 【以后进阶】多缓冲、运行时重配置、回调调度和 ISR 优化。先用最小示例完成一帧闭环，再处理吞吐或延迟。

### 常见错误 FAQ

- 参数错误：先检查指针非空、count/capacity 的单位是元素数、频率/阈值单位与上游一致。
- 硬件无结果：不要修改生成文件；回到 SysConfig 核对 pin、instance、时钟、Timer、DMA 和 Event 的完整链路。
- 结果异常：确认上一轮异步采集已经完成，真实 Fs/N/参考电压已传到算法，且没有在 DMA 使用期间改写 buffer。

### `signal_result_t SignalButton_Init(signal_button_t *button, const signal_button_config_t *config);`

**它做什么：** 根据调用者提供的配置或对象完成一次初始化；通常在 `SYSCFG_DL_init()` 之后且仅调用一次。

**什么时候调用：** 根据调用者提供的配置或对象完成一次初始化；通常在 `SYSCFG_DL_init()` 之后且仅调用一次。

| 参数 | 类型 | 初学者解释 |
|---|---|---|
| `button` | `signal_button_t *` | `button`（`signal_button_t `）是该 API 的输入/输出参数；按本 README 前面的数据单位和边界条件准备。 |
| `config` | `const signal_button_config_t *` | 调用者填写的配置对象。先阅读该类型的成员；它控制本次初始化或处理方式。 |

**返回：** 返回 signal_result_t 类型结果；调用者应检查该值。

**最小调用形状：** `SignalButton_Init(...);`。可直接从 README_MINIMAL_EXAMPLE.c 复制正常流程；README_FULL_EXAMPLE.c 展示全部公开 API，其中取消类 API 会以 #if 0 隔离。

**注意：** 所有指针和数组都由调用者拥有；先检查返回值。异步采集、DMA 或回调还在使用 buffer 时，不能读取结果或改写该 buffer。

### `signal_result_t SignalButton_Update(signal_button_t *button, signal_button_event_t *event);`

**它做什么：** 执行该模块公开的功能；具体数据流以本节参数表和本 README 前面的场景说明为准。

**什么时候调用：** 执行该模块公开的功能；具体数据流以本节参数表和本 README 前面的场景说明为准。

| 参数 | 类型 | 初学者解释 |
|---|---|---|
| `button` | `signal_button_t *` | `button`（`signal_button_t *`）是该 API 的输入/输出参数；按本 README 前面的数据单位和边界条件准备。 |
| `event` | `signal_button_event_t *` | `event`（`signal_button_event_t *`）是该 API 的输入/输出参数；按本 README 前面的数据单位和边界条件准备。 |

**返回：** 当前实现中出现的返回/成熟度枚举值：`SIGNAL_RESULT_INVALID_ARGUMENT`、`SIGNAL_RESULT_NOT_INITIALIZED`、`SIGNAL_RESULT_OK`。

**最小调用形状：** `SignalButton_Update(...);`。可直接从 README_MINIMAL_EXAMPLE.c 复制正常流程；README_FULL_EXAMPLE.c 展示全部公开 API，其中取消类 API 会以 #if 0 隔离。

**注意：** 所有指针和数组都由调用者拥有；先检查返回值。异步采集、DMA 或回调还在使用 buffer 时，不能读取结果或改写该 buffer。

### `signal_result_t SignalButton_GetPressed(const signal_button_t *button, bool *pressed);`

**它做什么：** 读取最近一次操作保存的状态、结果或配置；先确认前置操作已经成功。

**什么时候调用：** 读取最近一次操作保存的状态、结果或配置；先确认前置操作已经成功。

| 参数 | 类型 | 初学者解释 |
|---|---|---|
| `button` | `const signal_button_t *` | `button`（`const signal_button_t `）是该 API 的输入/输出参数；按本 README 前面的数据单位和边界条件准备。 |
| `pressed` | `bool *` | `pressed`（`bool `）是该 API 的输入/输出参数；按本 README 前面的数据单位和边界条件准备。 |

**返回：** 返回 signal_result_t 类型结果；调用者应检查该值。

**最小调用形状：** `SignalButton_GetPressed(...);`。可直接从 README_MINIMAL_EXAMPLE.c 复制正常流程；README_FULL_EXAMPLE.c 展示全部公开 API，其中取消类 API 会以 #if 0 隔离。

**注意：** 所有指针和数组都由调用者拥有；先检查返回值。异步采集、DMA 或回调还在使用 buffer 时，不能读取结果或改写该 buffer。

### `signal_module_status_t SignalButton_GetModuleStatus();`

**它做什么：** 读取模块当前的验证成熟度或静态状态，不会启动硬件操作。

**什么时候调用：** 读取模块当前的验证成熟度或静态状态，不会启动硬件操作。

**参数：** 无。

**返回：** 返回 signal_module_status_t 类型结果；调用者应检查该值。

**最小调用形状：** `SignalButton_GetModuleStatus(...);`。可直接从 README_MINIMAL_EXAMPLE.c 复制正常流程；README_FULL_EXAMPLE.c 展示全部公开 API，其中取消类 API 会以 #if 0 隔离。

**注意：** 所有指针和数组都由调用者拥有；先检查返回值。异步采集、DMA 或回调还在使用 buffer 时，不能读取结果或改写该 buffer。

### 示例、模块链与验收

- 最小入门：`README_MINIMAL_EXAMPLE.c`，只保留正常入口和结果读取。
- 全功能：`README_FULL_EXAMPLE.c`，以正确顺序展示当前头文件全部公开 API；`Stop` 等非常规路径不会默认执行。
- 模块链：先由上游提供单位、采样率和有效数据，再调用本模块；成功后将输出交给显示、控制、测量或下一步 DSP。硬件资源仍以 SysConfig 合约为唯一来源。
- 文档验收：README/API、两份示例和头文件会由 `tools/validate_beginner_documentation.ps1` 覆盖检查；这只表示文档与源码签名一致，不代替未进行的实板验证。

### 模块链

`上游采集/配置 -> 本模块 -> 检查返回值和结果 -> 测量、显示、控制或下一步 DSP`。只有确认本模块的输出单位和有效状态后，才交给下一模块。

## Hardware / Platform Binding

本模块的 GPIO 读操作由统一 MSPM0G3507 平台适配层提供；应用只需在 SysConfig 选择实际输入脚并使用生成的端口/引脚宏。公共 PC 回归入口为 `10_tests/pc/test_signal_library.c`，它验证消抖状态机，不代表真实按键已完成板测。

