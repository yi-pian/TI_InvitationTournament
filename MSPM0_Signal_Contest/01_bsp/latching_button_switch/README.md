# 自锁按键开关模块

## CCS SysConfig GUI Configuration

### Required resources

只需要 `GPIO`。时钟状态：`NO_INDEPENDENT_PERIPHERAL_CLOCK`；自锁开关的稳定状态判定由应用调用周期决定。

### Step 1 - Add the switch input

GUI Path: 左侧 `Software` -> `Add` -> `GPIO` -> 实例 -> `Group Pins` -> `ADD`；在新行设置 `Name`、`Direction = Input`、`IO Structure`，Port/Pin 按自锁开关接线选择。

### Step 2 - Input level and resistor

GUI Path: 左侧 `GPIO` -> 对应 pin -> `Digital IOMUX Features` -> `Internal Resistor`，按自锁开关原理图选择 `Pull-up` 或 `Pull-down`；同页 `Invert` 只保留一处逻辑反相，和 callback 的 ON/OFF 定义一致。

### Step 3 - PinMux and clock

GUI Path: pin -> `Assigned Port` / `Assigned Port Segment` / `Assigned Pin` -> `PinMux Peripheral and Pin Configuration`。Action: 选择当前接线并检查冲突。Clock: `NO_INDEPENDENT_PERIPHERAL_CLOCK`；README 推荐的 5 ms 是应用扫描 cadence，不要求新建 `TIMER`。概念边界见[共享时钟教材](../../00_docs/MSPM0_SYSCONFIG_CLOCK_TIMER_ADC_DAC_BEGINNER_GUIDE.md)。

### Expected generated symbols

核对 GPIO `*_PORT`、switch `*_PIN`、`*_IOMUX`；PROJECT_AUDIT 记录 GUI field、`.syscfg` property 和 symbol。

### Final checklist / Common mistakes / Do not change

- 程序读取真实物理状态，不用软件 toggle 冒充自锁。
- 上拉/下拉与 ON/OFF 极性一致，无多余 Timer。
- 不直接编辑 `.syscfg`/生成文件，不擅自换当前固定 Pin。

## 你真的需要这个模块吗？

### MSPM0G3507 比赛推荐方式

只读自锁开关当前电平时直接使用 `DL_GPIO_readPins()`。需要机械消抖、首次状态有效、changed/turned_on/turned_off 事件时继续使用本模块；这些状态管理是本模块的实际封装收益。

## 1. 模块作用

读取一个具有机械自锁能力的按键开关，消除触点抖动，并报告稳定 ON/OFF、刚切到 ON 和刚切到 OFF。

小白解释：这种开关按一下会机械保持，再按一下才弹回。即使 MCU 断电，它的物理位置也可能仍保持，所以程序不应该自己执行 `state = !state`；程序只负责读取开关现在真正接通还是断开。

它和普通按键的关键区别：普通按键松手就恢复，自锁开关松手后仍保持新的物理状态。

## 2. 输入

- `read_on(context, &on)`：平台回调把实际 GPIO 电平转换为逻辑 ON/OFF。
- `debounce_scans`：连续多少次相同读数后确认新状态。
- 应用固定周期调用 Update，推荐 5 ms。

模块第一次得到稳定读数时只完成“上电状态同步”，不会错误地产生 `turned_on/turned_off` 事件。

## 3. 输出

`SignalLatchingButtonSwitch_Update()` 输出：

- `raw_on`：本次直接读取的逻辑 ON/OFF；
- `state_valid`：是否已经完成第一次稳定状态同步；
- `stable_on`：当前消抖后的物理位置；
- `changed`：本次是否确认发生切换；
- `turned_on`：刚从 OFF 切到 ON；
- `turned_off`：刚从 ON 切到 OFF。

初始化后的前几次扫描可能 `state_valid=false`。`GetState()` 此时返回 `SIGNAL_RESULT_NO_DATA`，确认后才返回 ON/OFF。

## 4. 依赖

- `01_bsp/common/signal_status.h`；
- 1 个 GPIO input；
- 通常使用内部上拉；
- 无 Timer、IRQ、DMA 或动态内存依赖。

发光自锁按钮上的 LED 是另一套电路，不能因为开关只占一个输入脚，就把 LED 电源也直接接到 GPIO。

## 5. SysConfig 设置

新手详细配置：[PinMux / GPIO 教程](../../00_docs/SYSCONFIG_BEGINNER_GUIDE.md#pinmux)。本模块独有设置是与 COM/NO/NC 实际接法一致的 pull-up/pull-down 和有效电平；先用万用表认脚，再配 GPIO。现场速查见 [Quick Reference](../../00_docs/SYSCONFIG_QUICK_REFERENCE.md)。

### 三脚 COM/NO/NC 自锁开关

常见标记：

- `COM`：公共端；
- `NO`：Normally Open，OFF 时断开，ON 时与 COM 导通；
- `NC`：Normally Closed，OFF 时与 COM 导通，ON 时断开。

推荐接法：

```text
GND ───── COM
GPIO input + internal pull-up ───── NO
NC 不接
```

这样 OFF 时 GPIO 被上拉为高，ON 时 COM 与 NO 导通，把 GPIO 拉低。平台回调返回 `on = (pin_is_low)`。

如果使用 COM+NC，逻辑会反过来，只修改回调中的极性，不修改模块状态机。没有标记时先用万用表确认 COM/NO/NC。

### 两脚或带小板模块

- 两脚自锁按钮可按“GPIO—开关—GND + 上拉”连接。
- 带 `VCC/GND/S` 的模块必须先确认供电和输出逻辑；MSPM0 GPIO 只能接受安全的 3.3 V 逻辑，不能把未知 5 V `S` 直接输入 MCU。
- 带灯按钮可能另有 LED+/LED−，按 LED 额定电压、电流和限流要求接驱动，不要与 COM/NO/NC 混接。

### LP-MSPM0G3507 示例 pin

使用 `PA28`（LaunchPad 40-pin 第 38 脚）作为 input + pull-up：PA28 接 NO，GND 接 COM，NC 悬空。这个分配避开前面 TFT 和 4×4 键盘示例脚；若工程已有 PWM 等功能占用 PA28，请在 SysConfig 换一个安全空闲 GPIO。

## 6. 初始化方法

1. 万用表确认开关端子和 ON/OFF 导通关系；
2. SysConfig 配 GPIO input + pull-up；
3. `SYSCFG_DL_init()`；
4. 平台回调把 GPIO 电平转换为逻辑 `on`；
5. 推荐 `debounce_scans=3`，调用 `SignalLatchingButtonSwitch_Init()`；
6. 周期 Update，等待 `state_valid=true` 后再使用状态。

第一次稳定状态不会产生 changed 事件，避免 MCU 上电时把“开关本来就在 ON”误当成一次新的切换动作。

## 7. 调用方法

每 5 ms Update 一次，三次确认约为 15 ms：

- 持续控制系统使能：使用 `stable_on`；
- 只在切到 ON 时执行一次：使用 `turned_on`；
- 只在切到 OFF 时执行一次：使用 `turned_off`；
- 启动阶段先检查 `state_valid`。

如果开关控制危险输出，应用还应加入安全条件；不能仅凭单个机械开关就绕过过压、过流或急停保护。

## 8. 参数修改方法

- 改 GPIO：只改 `.syscfg` 和平台回调生成宏。
- COM+NO 改成 COM+NC：反转回调 high/low → on 映射。
- 改消抖时间：修改 Update 周期或 `debounce_scans`。
- 发光 LED：单独使用 GPIO/PWM/驱动模块管理，不写入本开关输入模块。
- 多个自锁开关：每个开关创建一个独立实例，不复制 `.c`。

## 9. 与其他模块如何连接

```text
Latching switch GPIO → stable physical ON/OFF
                              ↓
             Application enable / mode / output arm
                              ↓
                     TFT status indication
```

它适合做长期模式选择、输出允许、量程选择或页面锁定。输出是 UI/控制状态，不直接作为 ADC/FFT 数据。

## 10. 最小示例

~~~c
#include "signal_latching_button_switch.h"

static signal_result_t app_read_switch(void *context, bool *on)
{
    (void)context;
    *on = (DL_GPIO_readPins(SWITCH_PORT, SWITCH_PIN) == 0U);
    return SIGNAL_RESULT_OK;
}

signal_latching_button_switch_t mode_switch;
const signal_latching_button_switch_config_t config = {
    .context = NULL,
    .read_on = app_read_switch,
    .debounce_scans = 3U
};

SYSCFG_DL_init();
(void)SignalLatchingButtonSwitch_Init(&mode_switch, &config);

/* 每 5 ms 执行一次。 */
signal_latching_button_switch_event_t event;
if (SignalLatchingButtonSwitch_Update(&mode_switch, &event) ==
        SIGNAL_RESULT_OK && event.state_valid) {
    output_enabled = event.stable_on;
}
~~~

`SWITCH_PORT/SWITCH_PIN` 必须使用本工程 SysConfig 实际生成宏。

## 11. 常见错误

- 把自锁开关当普通按键，每次边沿执行软件 toggle：机械状态和软件状态会失去同步。
- COM/NO/NC 猜错：表现为逻辑反向或永远不变化，先用万用表。
- 没有上拉：断开时 GPIO 漂浮。
- 把发光 LED 端子当开关触点：可能导致错误电压或烧坏 GPIO。
- 5 V 模块输出直连 MSPM0：必须先确认输出电平，默认使用 3.3 V 逻辑。
- 忽略 `state_valid`：上电前几次扫描的 stable 值尚未确认。
- 用 changed 控制长期输出：changed 只保持一次 Update；长期状态应读 stable_on。

## 12. RAM 占用

动态分配 0。每个实例保存一个回调配置、候选/稳定状态、计数和有效标志，通常为几十字节；无数组和大 buffer。

## 13. Flash 占用

只有一次 GPIO 回调和固定状态机，不链接数学库。最终以应用 `.map` 为准。

## 14. CPU 计算量估计

每次 Update 为 O(1)，只读一次 GPIO。5 ms 周期轮询对 CPU 影响很小，不需要高速中断。

## 15. 当前验证状态

`MODULE_STATUS_BUILD_VERIFIED`。PC mock 已验证首次同步、ON/OFF 消抖和转换事件；TI Arm Clang `-Wall -Werror` 与 44 模块外设库聚合链接均已通过。没有真实自锁开关实测，不能写 `BOARD_VERIFIED`。

## 16. 以后实板验证步骤

1. 断电，用万用表确认 COM、NO、NC和两种机械位置。
2. 接 COM→GND、NO→GPIO pull-up，先打印 raw_on。
3. 开关分别保持 ON/OFF 后重启 MCU，确认首次同步正确且没有虚假 changed。
4. 连续切换 50 次，确认每次只有一个 turned_on 或 turned_off。
5. 如有 LED，单独验证它的电压、电流、极性和驱动，不与触点测试混在一起。
6. 保存接线、型号、逻辑和测试记录后才可升级 `BOARD_VERIFIED`。

## 17. 统一 API 教程（已按当前头文件核对）

本节由当前公开头文件、实现中实际出现的状态码和正式模块注册表生成。它补充前文的场景教程；函数签名变化时必须重新运行 `tools/upgrade_formal_beginner_docs.ps1`，不要手工保留旧 API。

遵循仓库的 [Beginner README 标准](../../00_docs/BEGINNER_README_STANDARD.md)：先用最小示例完成一次正常数据流，再按需要阅读全功能示例和本节 API 细节。

### 什么时候用 / 什么时候不要用

当题目需要“latching_button_switch”目录对应的公开功能，并且输入数据、单位和硬件资源满足前文约束时使用本模块。若只需要更简单的上游功能、输入尚未准备好，或需要不同的数据模型/外设资源，应先选择相邻模块而不是强行调用本 API。

### 输入 / 输出

输入由各 API 的只读数组、配置、频率/阈值和平台对象组成；输出写入 result/output/buffer 参数或由 Get API 返回。调用者负责数组容量、生命周期和物理单位；失败返回时输出不是有效结果。

### 调用顺序

```text
SignalLatchingButtonSwitch_Init -> SignalLatchingButtonSwitch_Update -> SignalLatchingButtonSwitch_GetState -> SignalLatchingButtonSwitch_GetModuleStatus
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

### `signal_result_t SignalLatchingButtonSwitch_Init(signal_latching_button_switch_t *switch_module, const signal_latching_button_switch_config_t *config);`

**它做什么：** 根据调用者提供的配置或对象完成一次初始化；通常在 `SYSCFG_DL_init()` 之后且仅调用一次。

**什么时候调用：** 根据调用者提供的配置或对象完成一次初始化；通常在 `SYSCFG_DL_init()` 之后且仅调用一次。

| 参数 | 类型 | 初学者解释 |
|---|---|---|
| `switch_module` | `signal_latching_button_switch_t *` | `switch_module`（`signal_latching_button_switch_t `）是该 API 的输入/输出参数；按本 README 前面的数据单位和边界条件准备。 |
| `config` | `const signal_latching_button_switch_config_t *` | 调用者填写的配置对象。先阅读该类型的成员；它控制本次初始化或处理方式。 |

**返回：** 返回 signal_result_t 类型结果；调用者应检查该值。

**最小调用形状：** `SignalLatchingButtonSwitch_Init(...);`。可直接从 README_MINIMAL_EXAMPLE.c 复制正常流程；README_FULL_EXAMPLE.c 展示全部公开 API，其中取消类 API 会以 #if 0 隔离。

**注意：** 所有指针和数组都由调用者拥有；先检查返回值。异步采集、DMA 或回调还在使用 buffer 时，不能读取结果或改写该 buffer。

### `signal_result_t SignalLatchingButtonSwitch_Update(signal_latching_button_switch_t *switch_module, signal_latching_button_switch_event_t *event);`

**它做什么：** 执行该模块公开的功能；具体数据流以本节参数表和本 README 前面的场景说明为准。

**什么时候调用：** 执行该模块公开的功能；具体数据流以本节参数表和本 README 前面的场景说明为准。

| 参数 | 类型 | 初学者解释 |
|---|---|---|
| `switch_module` | `signal_latching_button_switch_t *` | `switch_module`（`signal_latching_button_switch_t *`）是该 API 的输入/输出参数；按本 README 前面的数据单位和边界条件准备。 |
| `event` | `signal_latching_button_switch_event_t *` | `event`（`signal_latching_button_switch_event_t *`）是该 API 的输入/输出参数；按本 README 前面的数据单位和边界条件准备。 |

**返回：** 当前实现中出现的返回/成熟度枚举值：`SIGNAL_RESULT_INVALID_ARGUMENT`、`SIGNAL_RESULT_NOT_INITIALIZED`、`SIGNAL_RESULT_OK`。

**最小调用形状：** `SignalLatchingButtonSwitch_Update(...);`。可直接从 README_MINIMAL_EXAMPLE.c 复制正常流程；README_FULL_EXAMPLE.c 展示全部公开 API，其中取消类 API 会以 #if 0 隔离。

**注意：** 所有指针和数组都由调用者拥有；先检查返回值。异步采集、DMA 或回调还在使用 buffer 时，不能读取结果或改写该 buffer。

### `signal_result_t SignalLatchingButtonSwitch_GetState(const signal_latching_button_switch_t *switch_module, bool *on);`

**它做什么：** 读取最近一次操作保存的状态、结果或配置；先确认前置操作已经成功。

**什么时候调用：** 读取最近一次操作保存的状态、结果或配置；先确认前置操作已经成功。

| 参数 | 类型 | 初学者解释 |
|---|---|---|
| `switch_module` | `const signal_latching_button_switch_t *` | `switch_module`（`const signal_latching_button_switch_t `）是该 API 的输入/输出参数；按本 README 前面的数据单位和边界条件准备。 |
| `on` | `bool *` | `on`（`bool `）是该 API 的输入/输出参数；按本 README 前面的数据单位和边界条件准备。 |

**返回：** 返回 signal_result_t 类型结果；调用者应检查该值。

**最小调用形状：** `SignalLatchingButtonSwitch_GetState(...);`。可直接从 README_MINIMAL_EXAMPLE.c 复制正常流程；README_FULL_EXAMPLE.c 展示全部公开 API，其中取消类 API 会以 #if 0 隔离。

**注意：** 所有指针和数组都由调用者拥有；先检查返回值。异步采集、DMA 或回调还在使用 buffer 时，不能读取结果或改写该 buffer。

### `signal_module_status_t SignalLatchingButtonSwitch_GetModuleStatus();`

**它做什么：** 读取模块当前的验证成熟度或静态状态，不会启动硬件操作。

**什么时候调用：** 读取模块当前的验证成熟度或静态状态，不会启动硬件操作。

**参数：** 无。

**返回：** 返回 signal_module_status_t 类型结果；调用者应检查该值。

**最小调用形状：** `SignalLatchingButtonSwitch_GetModuleStatus(...);`。可直接从 README_MINIMAL_EXAMPLE.c 复制正常流程；README_FULL_EXAMPLE.c 展示全部公开 API，其中取消类 API 会以 #if 0 隔离。

**注意：** 所有指针和数组都由调用者拥有；先检查返回值。异步采集、DMA 或回调还在使用 buffer 时，不能读取结果或改写该 buffer。

### 示例、模块链与验收

- 最小入门：`README_MINIMAL_EXAMPLE.c`，只保留正常入口和结果读取。
- 全功能：`README_FULL_EXAMPLE.c`，以正确顺序展示当前头文件全部公开 API；`Stop` 等非常规路径不会默认执行。
- 模块链：先由上游提供单位、采样率和有效数据，再调用本模块；成功后将输出交给显示、控制、测量或下一步 DSP。硬件资源仍以 SysConfig 合约为唯一来源。
- 文档验收：README/API、两份示例和头文件会由 `tools/validate_beginner_documentation.ps1` 覆盖检查；这只表示文档与源码签名一致，不代替未进行的实板验证。

### 模块链

`上游采集/配置 -> 本模块 -> 检查返回值和结果 -> 测量、显示、控制或下一步 DSP`。只有确认本模块的输出单位和有效状态后，才交给下一模块。

## Hardware / Platform Binding

锁存按键的输入脚由 MSPM0G3507 GPIO 平台适配层绑定。请在 SysConfig 配置真实 pin 和上拉，再按调用周期调用 Update；PC 行为回归入口为 `10_tests/pc/test_signal_library.c`。

