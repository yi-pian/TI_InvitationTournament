# 4×4 矩阵键盘模块

## CCS SysConfig GUI Configuration

### Required resources

只需要一个或多个 `GPIO` group，共 4 个 row output 与 4 个 column input。时钟状态：`NO_INDEPENDENT_PERIPHERAL_CLOCK`；扫描/settle/debounce 都是应用层时间。

### Step 1 - Configure four rows

GUI Path: 左侧 `Software` -> `Add` -> `GPIO` -> 实例 -> `Group Pins` -> 连续 `ADD` 4 个 pin；每行设置唯一 `Name`、`Direction = Output`、`Initial Value`、`IO Structure`，再在 `PinMux Peripheral and Pin Configuration` 分配 row0..row3 的实际 Port/Pin。

### Step 2 - Configure four columns

GUI Path: 左侧 `GPIO` -> 同一或另一 group -> `Group Pins` -> `ADD` 4 个 pin -> 每个 pin 的 `Digital IOMUX Features` -> `Internal Resistor`。列线按电路选择 `Pull-up` 或 `Pull-down`；常见“行拉低、列读低”扫描使用 `Pull-up`，并让 callback 的 active level 与此一致。

### Step 3 - PinMux and clock

GUI Path: 每个 pin -> `Assigned Port` / `Assigned Port Segment` / `Assigned Pin` -> `PinMux Peripheral and Pin Configuration`。Action: 逐个核对 8 根线无重复/冲突。Clock: `NO_INDEPENDENT_PERIPHERAL_CLOCK`；row settle 和 debounce 是应用延时，不为 GPIO 创建 Timer。概念边界见[共享时钟教材](../../00_docs/MSPM0_SYSCONFIG_CLOCK_TIMER_ADC_DAC_BEGINNER_GUIDE.md)。

### Expected generated symbols

核对 group `*_PORT` 以及 4 个 row、4 个 column 的 `*_PIN`/`*_IOMUX`。如果跨 GPIO port，不能假设只有一个 `*_PORT`。PROJECT_AUDIT 逐 pin 建立 GUI 到生成宏的映射。

### Final checklist / Common mistakes / Do not change

- 4 output + 4 input 均存在，列电阻和逻辑极性一致。
- 八个 PinMux 无重复；扫描 cadence 没有被称为 GPIO clock。
- 不直接编辑 `.syscfg`/生成文件，不照搬不存在的 pin 表。

## 你真的需要这个模块吗？

### MSPM0G3507 比赛推荐方式

继续使用本模块。4 行轮流驱动、4 列读取、稳定时间、按键映射和消抖属于复杂扫描流程；直接写 8 个 GPIO 不会减少比赛代码。SysConfig 仍负责 4 个输出行与 4 个上拉输入列。

## 1. 模块作用

扫描由 4 根行线和 4 根列线组成的 16 键矩阵键盘，并提供消抖后的按下、释放和当前按键掩码。默认键面为：

```text
1 2 3 A
4 5 6 B
7 8 9 C
* 0 # D
```

小白解释：16 个按键并没有各占一个 GPIO，而是像表格一样共用 4 行、4 列。MCU 每次只“点亮/选中”一行，再读取 4 列，重复四次就知道哪个交叉点被按下。

## 2. 输入

- `drive_row(context, row, active)`：选择或释放某一行；模块只说“active”，平台回调决定 active 对应低电平。
- `read_column(context, column, &active)`：读取某一列当前是否和被选中的行导通。
- 可选微秒延时回调与 `settle_us`，用于切换行后等待电平稳定。
- `debounce_scans`：同一个 raw 按键状态连续出现多少次才确认。
- 可选 16 字节 row-major keymap。

## 3. 输出

`SignalMatrixKeypad4x4_Scan()` 每次返回：

- `raw_mask`：本次直接扫描到的 16-bit 状态；bit0=R1C1，bit15=R4C4；
- `stable_mask`：通过消抖后的当前状态；
- `pressed_mask`：本次新确认按下的键；
- `released_mask`：本次新确认松开的键；
- `ghost_possible`：无二极管键盘上，多行多列同时导通，可能出现“鬼键”。

`GetFirstPressed()` 可把当前第一个稳定按键直接转为 `'0'..'9'、A..D、*、#`。

## 4. 依赖

- `01_bsp/common/signal_status.h`。
- 8 个可用数字 GPIO：4 个输出行、4 个带上拉的输入列。
- 不依赖中断、Timer 或 DMA；扫描周期由应用主循环/调度器提供。

## 5. SysConfig 设置

新手详细配置：[PinMux / GPIO 教程](../../00_docs/SYSCONFIG_BEGINNER_GUIDE.md#pinmux)。本模块需要 4 个行输出和 4 个带上拉的列输入；八个 Pin 必须与 R1..R4/C1..C4 接线表逐一对应。现场速查见 [Quick Reference](../../00_docs/SYSCONFIG_QUICK_REFERENCE.md)。

### 5.1 通用接线

先找到键盘排针的 `R1 R2 R3 R4 C1 C2 C3 C4`。便宜薄膜键盘的 8-pin 排列不一定是“前四行、后四列”，没有丝印时必须用万用表通断档逐键确认，不能猜。

| 键盘线 | MSPM0 GPIO 设置 | 上电初值 |
|---|---|---|
| R1、R2、R3、R4 | Digital Output | 高电平（inactive） |
| C1、C2、C3、C4 | Digital Input + internal pull-up | 输入，高电平表示未按 |

本模块推荐 active-low 扫描：未选中的四行均为高；扫描某行时只把它拉低；按键按下后，对应列被拉低。平台回调的映射是：

```c
drive_row(active=true)  -> GPIO clear（输出低）
drive_row(active=false) -> GPIO set（输出高）
read_column(pin is low) -> active=true
```

### 5.2 LP-MSPM0G3507 可直接照抄的示例分配

下面选用 40-pin BoosterPack 排针上八个普通数字脚，并避开 TFT 示例占用的 PB9/PB8/PB6/PB15/PB12：

| 键盘 | MSPM0G3507 | LaunchPad 40-pin 位置 | SysConfig 名建议 |
|---|---|---:|---|
| R1 | PB16 | 11 | `KEYPAD_R1` |
| R2 | PB0 | 12 | `KEYPAD_R2` |
| R3 | PB7 | 14 | `KEYPAD_R3` |
| R4 | PB17 | 18 | `KEYPAD_R4` |
| C1 | PB18 | 19 | `KEYPAD_C1` |
| C2 | PB13 | 35 | `KEYPAD_C2` |
| C3 | PB20 | 36 | `KEYPAD_C3` |
| C4 | PB4 | 40 | `KEYPAD_C4` |

这只是当前 LP-MSPM0G3507 的易接线示例，不是模块源码要求。若你的工程已经使用某个脚，可在 SysConfig 换成其他空闲 GPIO，并同步修改平台回调中的生成宏。不要占用 PA19/PA20（SWD）、PA10/PA11（默认调试串口）、PA21/PA23（VREF）等特殊资源。

### 5.3 物理连接步骤

1. USB 断电；
2. 键盘 R1..R4 分别接上表四个 row GPIO；
3. C1..C4 分别接四个 column GPIO；
4. 纯机械矩阵键盘通常没有 VCC/GND 脚，不要把任意一根行列线接 3.3 V 或 GND；
5. 上电前用万用表确认行列之间只有按键按下时导通。

## 6. 初始化方法

1. `SYSCFG_DL_init()` 先把行设为高输出、列设为上拉输入；
2. 准备两个平台回调 `drive_row/read_column`；
3. 推荐 `settle_us=5`、`debounce_scans=3`；
4. 调用 `SignalMatrixKeypad4x4_Init()`，它会先释放全部四行。

keymap 指针必须在模块使用期间一直有效；传 `NULL` 就使用默认 4×4 键面。

## 7. 调用方法

在主循环或 5 ms 周期任务里调用一次 `SignalMatrixKeypad4x4_Scan()`。不要在一个死循环里无延时疯狂扫描，否则 `debounce_scans=3` 可能只代表几十微秒，不是真正的按键消抖。

```text
每 5 ms 扫一次 × debounce_scans=3 ≈ 15 ms 后确认
```

优先看 `event.pressed_mask` 做“一次按下只触发一次”；看 `stable_mask` 做“按住持续有效”；看 `released_mask` 做松开动作。

## 8. 参数修改方法

- 改 GPIO：只改 `.syscfg` 与平台回调的行/列 pin 表。
- 改消抖：改应用配置的 `debounce_scans` 或实际扫描周期。
- 改键面：传入 16 字节 row-major 数组，例如第一行四个字符放在索引 0..3。
- 改有效电平：只改平台回调对 high/low 与 active 的映射，不改模块扫描状态机。
- 改键盘尺寸：本模块固定 4×4；不要在 `.c` 里改常量伪装成 3×4，另做清晰迁移评估。

## 9. 与其他模块如何连接

常见链路：

```text
4×4 Keypad → stable/pressed event → Application config/menu
                                      ├→ DDS frequency/amplitude
                                      ├→ analyzer mode/range
                                      └→ TFT page/cursor
```

键盘输出的是 UI 事件，不直接进入 ADC/FFT 数据 buffer。应用读取事件后修改集中配置或切换页面即可。

## 10. 最小示例

~~~c
#include <stdbool.h>
#include <stddef.h>

#include "ti_msp_dl_config.h"
#include "signal_matrix_keypad_4x4.h"

static signal_matrix_keypad_4x4_t g_keypad;

static signal_result_t App_KeypadDriveRow(
    void *context, uint8_t row, bool active)
{
    uint32_t pin;

    (void)context;
    switch (row) {
    case 0U: pin = GPIO_KEYPAD_KEYPAD_R1_PIN; break;
    case 1U: pin = GPIO_KEYPAD_KEYPAD_R2_PIN; break;
    case 2U: pin = GPIO_KEYPAD_KEYPAD_R3_PIN; break;
    case 3U: pin = GPIO_KEYPAD_KEYPAD_R4_PIN; break;
    default: return SIGNAL_RESULT_INVALID_ARGUMENT;
    }

    if (active) {
        DL_GPIO_clearPins(GPIO_KEYPAD_PORT, pin);
    } else {
        DL_GPIO_setPins(GPIO_KEYPAD_PORT, pin);
    }
    return SIGNAL_RESULT_OK;
}

static signal_result_t App_KeypadReadColumn(
    void *context, uint8_t column, bool *active)
{
    uint32_t pin;

    (void)context;
    if (active == NULL) return SIGNAL_RESULT_INVALID_ARGUMENT;
    switch (column) {
    case 0U: pin = GPIO_KEYPAD_KEYPAD_C1_PIN; break;
    case 1U: pin = GPIO_KEYPAD_KEYPAD_C2_PIN; break;
    case 2U: pin = GPIO_KEYPAD_KEYPAD_C3_PIN; break;
    case 3U: pin = GPIO_KEYPAD_KEYPAD_C4_PIN; break;
    default: return SIGNAL_RESULT_INVALID_ARGUMENT;
    }

    *active = (DL_GPIO_readPins(GPIO_KEYPAD_PORT, pin) == 0U);
    return SIGNAL_RESULT_OK;
}

static void App_DelayUs(void *context, uint32_t microseconds)
{
    (void)context;
    delay_cycles((CPUCLK_FREQ / 1000000U) * microseconds);
}

static const signal_matrix_keypad_4x4_config_t g_keypad_config = {
    .context = NULL,
    .drive_row = App_KeypadDriveRow,
    .read_column = App_KeypadReadColumn,
    .delay_us = App_DelayUs,
    .settle_us = 5U,
    .debounce_scans = 3U,
    .keymap = NULL,
};

static bool App_KeypadGetNewSymbol(
    const signal_matrix_keypad_4x4_event_t *event, char *symbol)
{
    uint8_t key_index;

    if ((event == NULL) || (symbol == NULL)) return false;
    if (event->ghost_possible || (event->pressed_mask == 0U)) return false;
    for (key_index = 0U; key_index < SIGNAL_MATRIX_KEYPAD_4X4_KEY_COUNT;
         ++key_index) {
        if ((event->pressed_mask & (uint16_t)(UINT16_C(1) << key_index)) == 0U) {
            continue;
        }
        return SignalMatrixKeypad4x4_GetKey(&g_keypad, key_index, symbol) ==
            SIGNAL_RESULT_OK;
    }
    return false;
}

static signal_result_t App_KeypadInit(void)
{
    return SignalMatrixKeypad4x4_Init(&g_keypad, &g_keypad_config);
}

/* 在 main() 中，且只在 SYSCFG_DL_init() 完成后调用一次：
 * if (App_KeypadInit() != SIGNAL_RESULT_OK) while (1) { }
 */

/* 放在每 5 ms 执行一次的位置，例如主循环的 5 ms 任务。 */
static void App_KeypadTask5ms(void)
{
    signal_matrix_keypad_4x4_event_t event;
    char symbol;

    if (SignalMatrixKeypad4x4_Scan(&g_keypad, &event) != SIGNAL_RESULT_OK) {
        return;
    }
    if (App_KeypadGetNewSymbol(&event, &symbol)) {
        /* symbol 是刚确认按下的字符。 */
    }
}
~~~

上面的行驱动、列读取和延时回调是通用 API 接到任意平台 GPIO 时所需的最小适配层。
它们不是可省略的占位符。对本 README 第 5.2 节固定引脚的 MSPM0G3507 工程，当前模块
已把这部分封装为第 10.1 节的一函数接口；应用不再复制回调。若改变这 8 根固定键盘线，
必须同步修改 SysConfig、本模块的 MSPM0G3507 平台部分和 README。不要为 16 个按键分别
写 16 个 GPIO 判断；模块会轮流调用四行和四列回调完成 16 个交叉点扫描。

### 10.1 MSPM0G3507 固定引脚的一函数最小用法

本模块库固定使用本 README 第 5.2 节的 `GPIO_KEYPAD` 引脚和 active-low 扫描方式，
因此 MSPM0G3507 工程不必再在 `main.c` 编写行驱动、列读取、延时、初始化、
`pressed_mask` 遍历或鬼键过滤。最小闭环还必须包含定时调用：保存 SysConfig、执行
`SYSCFG_DL_init()` 后，启动 1 ms SysTick，并每约 5 ms 调用下面一个函数：

~~~c
#include "ti_msp_dl_config.h"
#include "signal_matrix_keypad_4x4.h"

void matrix_keypad_4x4_MinimalExample_Start(void)
{
    if (SysTick_Config(CPUCLK_FREQ / 1000U) != 0U) {
        while (1) { }
    }
}

void SysTick_Handler(void)
{
    static uint8_t milliseconds;
    char symbol;

    ++milliseconds;
    if (milliseconds < 5U) return;
    milliseconds = 0U;

    if (SignalMatrixKeypad4x4_ReadNewSymbol(&symbol) == SIGNAL_RESULT_OK) {
        /* symbol 是本次新确认按下的 '1'、'5'、'A'、'#' 等字符。 */
    }
}
~~~

`SIGNAL_RESULT_NO_DATA` 不是错误，只表示本次没有新按键、出现可能鬼键，或按键仍在
消抖中。函数内部第一次调用时自动创建并初始化一个键盘对象；调用者不能同时再对该
内部对象调用 `SignalMatrixKeypad4x4_Init()` 或 `SignalMatrixKeypad4x4_Scan()`。

`SysTick_Handler` 是 Cortex-M 固定中断函数名；一个工程只能定义一次。若工程已有该函数，
不要再复制第二个函数定义，只把示例中从 `static uint8_t milliseconds;` 到末尾的扫描逻辑
合并进已有的 `SysTick_Handler`。`matrix_keypad_4x4_MinimalExample_Start()` 必须在
`SYSCFG_DL_init()` 后调用一次。

该便利接口只在 `__MSPM0G3507__` 目标编译，直接使用 SysConfig 生成的
`GPIO_KEYPAD_*` 宏。若某个工程改变了这 8 根固定键盘线，必须同时修改本模块平台部分、
README 和 SysConfig；不要在应用 `main.c` 临时改 pin。

## 11. 常见错误

- 八根线顺序猜错：表现为按一个键却读到另一个键，先用万用表确定 R/C。
- 列输入没有上拉：未按键时电平漂浮，出现随机按键。
- 多行同时拉低：失去矩阵定位能力；回调必须让未选行保持高。
- 把“电气低电平”直接当 API 的 false：本 API 的 `active=true` 表示选中/按下，和物理高低分开。
- 在 GPIO 中断里直接扫描：列电平取决于当前行，普通轮询更简单可靠。
- 只看 `stable_mask`：按住时主循环会反复触发；一次动作应看 `pressed_mask`。
- 三个或更多键同时按出现鬼键：普通无二极管矩阵无法从软件完全消除；看到 `ghost_possible=true` 时不要执行危险组合命令。

## 12. RAM 占用

动态分配 0；模块状态约几十字节，事件结构 10 B 左右，keymap 默认存放在 Flash。没有 N 点数组和大 buffer。

## 13. Flash 占用

代码是固定 4×4 扫描、消抖和 bit mask 操作，预计为小型模块；最终数值以 TI Arm Clang 应用 `.map` 为准。本模块不链接数学库。

## 14. CPU 计算量估计

每次完整扫描执行 4 次行选择、16 次列读取、4 次行释放，外加最多 `4 × settle_us` 等待。按 5 us 稳定时间计算，纯等待约 20 us；5 ms 扫一次时 CPU/总线负担很低。

## 15. 当前验证状态

`MODULE_STATUS_BUILD_VERIFIED`：PC 模拟矩阵/消抖测试、TI Arm Clang `-Wall -Werror` 编译和 44 模块外设库聚合链接均已通过。尚未连接真实 4×4 键盘，因此不是 `BOARD_VERIFIED`。

## 16. 以后实板验证步骤

1. 断电接八根线；万用表确认 R/C 顺序。
2. 先临时打印每次 `raw_mask`，逐个按下 16 键，确认 bit0..bit15 与键面一一对应。
3. 以 5 ms 扫描、3 次消抖测试短按、长按和快速连按。
4. 同时按同一行、同一列和矩形三个角，观察 `ghost_possible`。
5. 与 TFT 同时使用时检查引脚无重叠、屏幕 SPI 刷新不会让键盘扫描长时间饿死。
6. 保存 16 键检查表和异常记录后才可升级 `BOARD_VERIFIED`。

## 17. 统一 API 教程（已按当前头文件核对）

本节由当前公开头文件、实现中实际出现的状态码和正式模块注册表生成。它补充前文的场景教程；函数签名变化时必须重新运行 `tools/upgrade_formal_beginner_docs.ps1`，不要手工保留旧 API。

遵循仓库的 [Beginner README 标准](../../00_docs/BEGINNER_README_STANDARD.md)：先用最小示例完成一次正常数据流，再按需要阅读全功能示例和本节 API 细节。

### 什么时候用 / 什么时候不要用

当题目需要“matrix_keypad_4x4”目录对应的公开功能，并且输入数据、单位和硬件资源满足前文约束时使用本模块。若只需要更简单的上游功能、输入尚未准备好，或需要不同的数据模型/外设资源，应先选择相邻模块而不是强行调用本 API。

### 输入 / 输出

输入由各 API 的只读数组、配置、频率/阈值和平台对象组成；输出写入 result/output/buffer 参数或由 Get API 返回。调用者负责数组容量、生命周期和物理单位；失败返回时输出不是有效结果。

### 调用顺序

```text
SignalMatrixKeypad4x4_Init -> SignalMatrixKeypad4x4_GetKey -> SignalMatrixKeypad4x4_GetFirstPressed -> SignalMatrixKeypad4x4_GetModuleStatus -> SignalMatrixKeypad4x4_Scan
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

### `signal_result_t SignalMatrixKeypad4x4_Init(signal_matrix_keypad_4x4_t *keypad, const signal_matrix_keypad_4x4_config_t *config);`

**它做什么：** 根据调用者提供的配置或对象完成一次初始化；通常在 `SYSCFG_DL_init()` 之后且仅调用一次。

**什么时候调用：** 根据调用者提供的配置或对象完成一次初始化；通常在 `SYSCFG_DL_init()` 之后且仅调用一次。

| 参数 | 类型 | 初学者解释 |
|---|---|---|
| `keypad` | `signal_matrix_keypad_4x4_t *` | `keypad`（`signal_matrix_keypad_4x4_t `）是该 API 的输入/输出参数；按本 README 前面的数据单位和边界条件准备。 |
| `config` | `const signal_matrix_keypad_4x4_config_t *` | 调用者填写的配置对象。先阅读该类型的成员；它控制本次初始化或处理方式。 |

**返回：** 返回 signal_result_t 类型结果；调用者应检查该值。

**最小调用形状：** `SignalMatrixKeypad4x4_Init(...);`。可直接从 README_MINIMAL_EXAMPLE.c 复制正常流程；README_FULL_EXAMPLE.c 展示全部公开 API，其中取消类 API 会以 #if 0 隔离。

**注意：** 所有指针和数组都由调用者拥有；先检查返回值。异步采集、DMA 或回调还在使用 buffer 时，不能读取结果或改写该 buffer。

### `signal_result_t SignalMatrixKeypad4x4_Scan(signal_matrix_keypad_4x4_t *keypad, signal_matrix_keypad_4x4_event_t *event);`

**它做什么：** 执行该模块公开的功能；具体数据流以本节参数表和本 README 前面的场景说明为准。

**什么时候调用：** 执行该模块公开的功能；具体数据流以本节参数表和本 README 前面的场景说明为准。

| 参数 | 类型 | 初学者解释 |
|---|---|---|
| `keypad` | `signal_matrix_keypad_4x4_t *` | `keypad`（`signal_matrix_keypad_4x4_t *`）是该 API 的输入/输出参数；按本 README 前面的数据单位和边界条件准备。 |
| `event` | `signal_matrix_keypad_4x4_event_t *` | `event`（`signal_matrix_keypad_4x4_event_t *`）是该 API 的输入/输出参数；按本 README 前面的数据单位和边界条件准备。 |

**返回：** 当前实现中出现的返回/成熟度枚举值：`SIGNAL_RESULT_INVALID_ARGUMENT`、`SIGNAL_RESULT_NOT_INITIALIZED`、`SIGNAL_RESULT_OK`。

**最小调用形状：** `SignalMatrixKeypad4x4_Scan(...);`。可直接从 README_MINIMAL_EXAMPLE.c 复制正常流程；README_FULL_EXAMPLE.c 展示全部公开 API，其中取消类 API 会以 #if 0 隔离。

**注意：** 所有指针和数组都由调用者拥有；先检查返回值。异步采集、DMA 或回调还在使用 buffer 时，不能读取结果或改写该 buffer。

### `signal_result_t SignalMatrixKeypad4x4_GetKey(const signal_matrix_keypad_4x4_t *keypad, uint8_t key_index, char *symbol);`

**它做什么：** 读取最近一次操作保存的状态、结果或配置；先确认前置操作已经成功。

**什么时候调用：** 读取最近一次操作保存的状态、结果或配置；先确认前置操作已经成功。

| 参数 | 类型 | 初学者解释 |
|---|---|---|
| `keypad` | `const signal_matrix_keypad_4x4_t *` | `keypad`（`const signal_matrix_keypad_4x4_t `）是该 API 的输入/输出参数；按本 README 前面的数据单位和边界条件准备。 |
| `key_index` | `uint8_t` | 索引或通道号；范围由相应数组长度、FFT bin 数或当前硬件配置决定。 |
| `symbol` | `char *` | `symbol`（`char `）是该 API 的输入/输出参数；按本 README 前面的数据单位和边界条件准备。 |

**返回：** 返回 signal_result_t 类型结果；调用者应检查该值。

**最小调用形状：** `SignalMatrixKeypad4x4_GetKey(...);`。可直接从 README_MINIMAL_EXAMPLE.c 复制正常流程；README_FULL_EXAMPLE.c 展示全部公开 API，其中取消类 API 会以 #if 0 隔离。

**注意：** 所有指针和数组都由调用者拥有；先检查返回值。异步采集、DMA 或回调还在使用 buffer 时，不能读取结果或改写该 buffer。

### `signal_result_t SignalMatrixKeypad4x4_GetFirstPressed(const signal_matrix_keypad_4x4_t *keypad, char *symbol, uint8_t *key_index);`

**它做什么：** 读取最近一次操作保存的状态、结果或配置；先确认前置操作已经成功。

**什么时候调用：** 读取最近一次操作保存的状态、结果或配置；先确认前置操作已经成功。

| 参数 | 类型 | 初学者解释 |
|---|---|---|
| `keypad` | `const signal_matrix_keypad_4x4_t *` | `keypad`（`const signal_matrix_keypad_4x4_t `）是该 API 的输入/输出参数；按本 README 前面的数据单位和边界条件准备。 |
| `symbol` | `char *` | `symbol`（`char `）是该 API 的输入/输出参数；按本 README 前面的数据单位和边界条件准备。 |
| `key_index` | `uint8_t *` | 索引或通道号；范围由相应数组长度、FFT bin 数或当前硬件配置决定。 |

**返回：** 返回 signal_result_t 类型结果；调用者应检查该值。

**最小调用形状：** `SignalMatrixKeypad4x4_GetFirstPressed(...);`。可直接从 README_MINIMAL_EXAMPLE.c 复制正常流程；README_FULL_EXAMPLE.c 展示全部公开 API，其中取消类 API 会以 #if 0 隔离。

**注意：** 所有指针和数组都由调用者拥有；先检查返回值。异步采集、DMA 或回调还在使用 buffer 时，不能读取结果或改写该 buffer。

### `signal_module_status_t SignalMatrixKeypad4x4_GetModuleStatus();`

**它做什么：** 读取模块当前的验证成熟度或静态状态，不会启动硬件操作。

**什么时候调用：** 读取模块当前的验证成熟度或静态状态，不会启动硬件操作。

**参数：** 无。

**返回：** 返回 signal_module_status_t 类型结果；调用者应检查该值。

**最小调用形状：** `SignalMatrixKeypad4x4_GetModuleStatus(...);`。可直接从 README_MINIMAL_EXAMPLE.c 复制正常流程；README_FULL_EXAMPLE.c 展示全部公开 API，其中取消类 API 会以 #if 0 隔离。

**注意：** 所有指针和数组都由调用者拥有；先检查返回值。异步采集、DMA 或回调还在使用 buffer 时，不能读取结果或改写该 buffer。

### 示例、模块链与验收

- 最小入门：`README_MINIMAL_EXAMPLE.c`，只保留正常入口和结果读取。
- 全功能：`README_FULL_EXAMPLE.c`，以正确顺序展示当前头文件全部公开 API；`Stop` 等非常规路径不会默认执行。
- 模块链：先由上游提供单位、采样率和有效数据，再调用本模块；成功后将输出交给显示、控制、测量或下一步 DSP。硬件资源仍以 SysConfig 合约为唯一来源。
- 文档验收：README/API、两份示例和头文件会由 `tools/validate_beginner_documentation.ps1` 覆盖检查；这只表示文档与源码签名一致，不代替未进行的实板验证。

### 模块链

`上游采集/配置 -> 本模块 -> 检查返回值和结果 -> 测量、显示、控制或下一步 DSP`。只有确认本模块的输出单位和有效状态后，才交给下一模块。

## Hardware / Platform Binding

矩阵键盘的行输出、列输入和扫描延时由 MSPM0G3507 GPIO 平台适配层提供。SysConfig 必须为 4 行和 4 列选择实际 GPIO，并确保没有与 ADC、TFT 或其他模块冲突；行为回归入口为 `10_tests/pc/test_signal_library.c`。

## 18. 22_X PLL 1-5 倍频选择（4×4 矩阵键盘）

本节是 `22_X` 的应用示例。经用户明确授权，当前版本已修改本模块 `.c/.h`，将第 5.2
节固定 MSPM0G3507 GPIO 的扫描、消抖、鬼键过滤和字符读取封装为一函数接口；应用只把
该接口接到 TFT 和外部倍频电路。

### 18.1 本题 SysConfig 对象和引脚

先按本 README 的第 5 节增加键盘 GPIO group，再增加一个外部控制 GPIO group：

| SysConfig group | Name | 引脚 | 设置 |
|---|---|---|---|
| `GPIO_KEYPAD` | `KEYPAD_R1` | PB16 | Output，Initial Value = Set |
| `GPIO_KEYPAD` | `KEYPAD_R2` | PB0 | Output，Initial Value = Set |
| `GPIO_KEYPAD` | `KEYPAD_R3` | PB7 | Output，Initial Value = Set |
| `GPIO_KEYPAD` | `KEYPAD_R4` | PB17 | Output，Initial Value = Set |
| `GPIO_KEYPAD` | `KEYPAD_C1` | PB18 | Input，Internal Resistor = Pull-up |
| `GPIO_KEYPAD` | `KEYPAD_C2` | PB13 | Input，Internal Resistor = Pull-up |
| `GPIO_KEYPAD` | `KEYPAD_C3` | PB20 | Input，Internal Resistor = Pull-up |
| `GPIO_KEYPAD` | `KEYPAD_C4` | PB4 | Input，Internal Resistor = Pull-up |
| `GPIO_PLL_CTRL` | `PLL_A` | PA12 | Output，Initial Value = Cleared |
| `GPIO_PLL_CTRL` | `PLL_B` | PA13 | Output，Initial Value = Cleared |
| `GPIO_PLL_CTRL` | `PLL_C` | PA14 | Output，Initial Value = Cleared |
| `GPIO_PLL_CTRL` | `PLL_D` | PA15 | Output，Initial Value = Cleared |

保存并重新生成后，本节代码使用生成的 `GPIO_KEYPAD_*` 与
`GPIO_PLL_CTRL_*` 宏。不要手写 `ti_msp_dl_config.c/.h`。

键盘使用默认键面，只接受数字键 `1` 到 `5`；其余按键不改变当前倍频数。
ABCD 的定义按题目中从左到右的 `ABC` 位序：

| 数字键 / 显示 | A | B | C | D | 电路状态 |
|---:|---:|---:|---:|---:|---|
| 1 | 0 | 0 | 0 | 0 | 模拟开关关，持续 1 倍频 |
| 2 | 0 | 1 | 1 | 1 | 模拟开关开，`ABC=011` |
| 3 | 1 | 0 | 1 | 1 | 模拟开关开，`ABC=101` |
| 4 | 0 | 0 | 1 | 1 | 模拟开关开，`ABC=001` |
| 5 | 1 | 1 | 0 | 1 | 模拟开关开，`ABC=110` |

### 18.2 旧版低层适配记录（不要复制）

以下代码是便利接口加入前的低层适配记录；它需要应用层自行保管键盘对象和 GPIO
回调。现在固定 MSPM0G3507 引脚的工程应使用本节末尾的第 18.3 节，不要复制本节代码。

在已有 `#include` 后加入：

~~~c
#include <stdbool.h>
#include <stddef.h>

#include "signal_matrix_keypad_4x4.h"
~~~

在全局变量区加入以下代码。`SysTick_Handler` 每 1 ms 进入一次，但仅在每第
5 次执行一次完整扫描；因此即使主循环正在发送较多 TFT 像素，键盘的 3 次
消抖仍约为 15 ms。中断不调用任何 TFT API。

~~~c
#define APP_KEYPAD_SCAN_PERIOD_MS  (5U)

static signal_matrix_keypad_4x4_t g_keypad;
static volatile signal_result_t g_keypad_status;
static volatile uint8_t g_pll_multiplier = 1U;
static volatile uint8_t g_pll_display_revision = 1U;
static uint8_t g_pll_displayed_revision = 1U;

static signal_result_t App_KeypadDriveRow(
    void *context, uint8_t row, bool active)
{
    uint32_t pin;

    (void)context;
    switch (row) {
    case 0U: pin = GPIO_KEYPAD_KEYPAD_R1_PIN; break;
    case 1U: pin = GPIO_KEYPAD_KEYPAD_R2_PIN; break;
    case 2U: pin = GPIO_KEYPAD_KEYPAD_R3_PIN; break;
    case 3U: pin = GPIO_KEYPAD_KEYPAD_R4_PIN; break;
    default: return SIGNAL_RESULT_INVALID_ARGUMENT;
    }

    if (active) {
        DL_GPIO_clearPins(GPIO_KEYPAD_PORT, pin);
    } else {
        DL_GPIO_setPins(GPIO_KEYPAD_PORT, pin);
    }
    return SIGNAL_RESULT_OK;
}

static signal_result_t App_KeypadReadColumn(
    void *context, uint8_t column, bool *active)
{
    uint32_t pin;

    (void)context;
    if (active == NULL) return SIGNAL_RESULT_INVALID_ARGUMENT;
    switch (column) {
    case 0U: pin = GPIO_KEYPAD_KEYPAD_C1_PIN; break;
    case 1U: pin = GPIO_KEYPAD_KEYPAD_C2_PIN; break;
    case 2U: pin = GPIO_KEYPAD_KEYPAD_C3_PIN; break;
    case 3U: pin = GPIO_KEYPAD_KEYPAD_C4_PIN; break;
    default: return SIGNAL_RESULT_INVALID_ARGUMENT;
    }

    *active = (DL_GPIO_readPins(GPIO_KEYPAD_PORT, pin) == 0U);
    return SIGNAL_RESULT_OK;
}

static void App_DelayUs(void *context, uint32_t microseconds)
{
    (void)context;
    delay_cycles((CPUCLK_FREQ / 1000000U) * microseconds);
}

static void App_SetPLLMultiplier(uint8_t multiplier)
{
    uint32_t set_pins = 0U;
    const uint32_t all_pins = GPIO_PLL_CTRL_PLL_A_PIN |
        GPIO_PLL_CTRL_PLL_B_PIN | GPIO_PLL_CTRL_PLL_C_PIN |
        GPIO_PLL_CTRL_PLL_D_PIN;

    switch (multiplier) {
    case 2U:
        set_pins = GPIO_PLL_CTRL_PLL_B_PIN | GPIO_PLL_CTRL_PLL_C_PIN |
            GPIO_PLL_CTRL_PLL_D_PIN;
        break;
    case 3U:
        set_pins = GPIO_PLL_CTRL_PLL_A_PIN | GPIO_PLL_CTRL_PLL_C_PIN |
            GPIO_PLL_CTRL_PLL_D_PIN;
        break;
    case 4U:
        set_pins = GPIO_PLL_CTRL_PLL_C_PIN | GPIO_PLL_CTRL_PLL_D_PIN;
        break;
    case 5U:
        set_pins = GPIO_PLL_CTRL_PLL_A_PIN | GPIO_PLL_CTRL_PLL_B_PIN |
            GPIO_PLL_CTRL_PLL_D_PIN;
        break;
    default:
        multiplier = 1U;
        break;
    }

    DL_GPIO_clearPins(GPIO_PLL_CTRL_PORT, all_pins & ~set_pins);
    DL_GPIO_setPins(GPIO_PLL_CTRL_PORT, set_pins);
    g_pll_multiplier = multiplier;
    ++g_pll_display_revision;
}

static bool App_KeypadGetNewSymbol(
    const signal_matrix_keypad_4x4_event_t *event, char *symbol)
{
    uint8_t key_index;

    if ((event == NULL) || (symbol == NULL)) return false;
    if (event->ghost_possible || (event->pressed_mask == 0U)) return false;
    for (key_index = 0U; key_index < SIGNAL_MATRIX_KEYPAD_4X4_KEY_COUNT;
         ++key_index) {
        if ((event->pressed_mask & (uint16_t)(UINT16_C(1) << key_index)) == 0U) {
            continue;
        }
        return SignalMatrixKeypad4x4_GetKey(&g_keypad, key_index, symbol) ==
            SIGNAL_RESULT_OK;
    }
    return false;
}

static void App_ProcessPLLKey(char symbol)
{
    if ((symbol >= '1') && (symbol <= '5')) {
        App_SetPLLMultiplier((uint8_t)(symbol - '0'));
    }
}

void SysTick_Handler(void)
{
    static uint8_t milliseconds;
    signal_matrix_keypad_4x4_event_t event;
    char symbol;

    ++milliseconds;
    if (milliseconds < APP_KEYPAD_SCAN_PERIOD_MS) return;
    milliseconds = 0U;
    g_keypad_status = SignalMatrixKeypad4x4_Scan(&g_keypad, &event);
    if ((g_keypad_status == SIGNAL_RESULT_OK) &&
        App_KeypadGetNewSymbol(&event, &symbol)) {
        App_ProcessPLLKey(symbol);
    }
}

static tft_ili9341_status_t App_DrawPLLMultiplier(uint8_t multiplier)
{
    tft_ili9341_status_t status;

    status = TFT_ILI9341_FillRect(
        &g_tft, 48, 8, 8, 16, TFT_ILI9341_BLACK);
    if (status != TFT_ILI9341_OK) return status;
    return TFT_ILI9341_DrawInt32(
        &g_tft, 48, 8, multiplier, TFT_ILI9341_FONT_8X16,
        TFT_ILI9341_WHITE, TFT_ILI9341_BLACK, false);
}
~~~

将 `SYSCFG_DL_init()` 后的初始化代码按下面顺序加入。`keypad_config` 是传给已复制模块
的配置。此时先不启动 SysTick，避免初始化 TFT 的过程中产生键盘事件。

~~~c
    const signal_matrix_keypad_4x4_config_t keypad_config = {
        .context = NULL,
        .drive_row = App_KeypadDriveRow,
        .read_column = App_KeypadReadColumn,
        .delay_us = App_DelayUs,
        .settle_us = 5U,
        .debounce_scans = 3U,
        .keymap = NULL,
    };

    g_keypad_status = SignalMatrixKeypad4x4_Init(&g_keypad, &keypad_config);
    if (g_keypad_status != SIGNAL_RESULT_OK) while (1) { }
    App_SetPLLMultiplier(1U);
~~~

显示初始 `PLL:` 标签后，使用下面一行代替写死的数字；该函数只擦除并重画
数字所在的 8×16 像素区域，不会刷新蓝色边框或李萨如区域：

~~~c
        g_tft_status = App_DrawPLLMultiplier(g_pll_multiplier);
        g_pll_displayed_revision = g_pll_display_revision;
~~~

完成 `Lissajous_DrawStaticFrame()` 且确认 `g_tft_status` 正常后，再加入下面一行，
用 CMSIS 的 SysTick 配置 1 ms 节拍：

~~~c
    if (SysTick_Config(CPUCLK_FREQ / 1000U) != 0U) while (1) { }
~~~

最后，在主循环中绘制李萨如图前或后加入以下代码。`displayed_revision` 是
主循环自己的已显示版本号；如果中断在 TFT 绘制期间改变倍频数，版本号不同会
在下一轮继续局部刷新，不会丢失更新。

~~~c
        uint8_t revision = g_pll_display_revision;

        if (revision != g_pll_displayed_revision) {
            g_tft_status = App_DrawPLLMultiplier(g_pll_multiplier);
            if (g_tft_status != TFT_ILI9341_OK) while (1) { }
            g_pll_displayed_revision = revision;
        }
~~~

注意：`App_SetPLLMultiplier()` 会先清零未使用的控制脚、再置位目标脚；切换瞬间
外部电路可能短暂看见 `D=0`，这是有意让模拟开关先关断，避免把不完整的 ABC
码送入已开启的模拟开关。若外部倍频器要求无毛刺切换，必须由其数据手册确定
锁存/使能时序，不能在本模块 `.c/.h` 中猜测。

### 18.3 当前版本：只调用模块读取按键字符

本模块已在 MSPM0G3507 目标下固定 `GPIO_KEYPAD` 的 R1-R4、C1-C4 和 5 us/3 次
消抖配置。应用只保留本题的 PLL 映射，键盘扫描、行列 GPIO、消抖、鬼键过滤和
`symbol` 转换都在模块内完成。

在已有 include 中保留：

~~~c
#include "signal_matrix_keypad_4x4.h"
~~~

在全局变量和函数区复制下面代码：

~~~c
#define APP_KEYPAD_SCAN_PERIOD_MS  (5U)

static volatile signal_result_t g_keypad_status;
static volatile uint8_t g_pll_multiplier = 1U;
static volatile uint8_t g_pll_display_revision = 1U;
static uint8_t g_pll_displayed_revision = 1U;

static void App_SetPLLMultiplier(uint8_t multiplier)
{
    uint32_t set_pins = 0U;
    const uint32_t all_pins = GPIO_PLL_CTRL_PLL_A_PIN |
        GPIO_PLL_CTRL_PLL_B_PIN | GPIO_PLL_CTRL_PLL_C_PIN |
        GPIO_PLL_CTRL_PLL_D_PIN;

    switch (multiplier) {
    case 2U:
        set_pins = GPIO_PLL_CTRL_PLL_B_PIN | GPIO_PLL_CTRL_PLL_C_PIN |
            GPIO_PLL_CTRL_PLL_D_PIN;
        break;
    case 3U:
        set_pins = GPIO_PLL_CTRL_PLL_A_PIN | GPIO_PLL_CTRL_PLL_C_PIN |
            GPIO_PLL_CTRL_PLL_D_PIN;
        break;
    case 4U:
        set_pins = GPIO_PLL_CTRL_PLL_C_PIN | GPIO_PLL_CTRL_PLL_D_PIN;
        break;
    case 5U:
        set_pins = GPIO_PLL_CTRL_PLL_A_PIN | GPIO_PLL_CTRL_PLL_B_PIN |
            GPIO_PLL_CTRL_PLL_D_PIN;
        break;
    default:
        multiplier = 1U;
        break;
    }

    DL_GPIO_clearPins(GPIO_PLL_CTRL_PORT, all_pins & ~set_pins);
    DL_GPIO_setPins(GPIO_PLL_CTRL_PORT, set_pins);
    g_pll_multiplier = multiplier;
    ++g_pll_display_revision;
}

static void App_ProcessPLLKey(char symbol)
{
    if ((symbol >= '1') && (symbol <= '5')) {
        App_SetPLLMultiplier((uint8_t)(symbol - '0'));
    }
}

void SysTick_Handler(void)
{
    static uint8_t milliseconds;
    char symbol;

    ++milliseconds;
    if (milliseconds < APP_KEYPAD_SCAN_PERIOD_MS) return;
    milliseconds = 0U;
    g_keypad_status = SignalMatrixKeypad4x4_ReadNewSymbol(&symbol);
    if (g_keypad_status == SIGNAL_RESULT_OK) {
        App_ProcessPLLKey(symbol);
    }
}

static tft_ili9341_status_t App_DrawPLLMultiplier(uint8_t multiplier)
{
    tft_ili9341_status_t status;

    status = TFT_ILI9341_FillRect(
        &g_tft, 48, 8, 8, 16, TFT_ILI9341_BLACK);
    if (status != TFT_ILI9341_OK) return status;
    return TFT_ILI9341_DrawInt32(
        &g_tft, 48, 8, multiplier, TFT_ILI9341_FONT_8X16,
        TFT_ILI9341_WHITE, TFT_ILI9341_BLACK, false);
}
~~~

`SYSCFG_DL_init()` 后只保留本题输出的初始值，不再创建 `g_keypad` 或
`keypad_config`：

~~~c
    App_SetPLLMultiplier(1U);
~~~

TFT 初始标题、`App_DrawPLLMultiplier()`、`SysTick_Config()` 和主循环中的显示版本
比较沿用第 18.2 节的现有代码。第一次 5 ms SysTick 调用会由模块自动初始化键盘；
`SIGNAL_RESULT_NO_DATA` 代表无新键、消抖中或可能鬼键，应用不需要动作。

## 19. 比赛通用功能代码

本 README 已提供固定引脚扫描、消抖、鬼键过滤和 `ReadNewSymbol()`；没有把“频率/采样率
数字预输入、退格、确认、取消、溢出保护”硬塞进键盘驱动，因为这些规则属于题目参数。
请复制统一手册
[CONTEST_FUNCTIONAL_CODE_COOKBOOK.md](../../00_docs/CONTEST_FUNCTIONAL_CODE_COOKBOOK.md)
第 5 节。预输入缓冲只保存编辑中的文本，按 `#` 且解析成功后才改真实参数；`*` 退格，
`D` 取消。屏幕翻页状态机见手册第 4 节，键盘模块本身只输出稳定字符。

## 20. 可直接复制：数字键盘预输入

扫描、消抖、鬼键过滤和 `ReadNewSymbol()` 属于本模块；下面的编辑缓冲属于应用层，
适合输入频率、采样率或阈值。数字输入期间不改硬件，只有 `#` 解析成功后才提交；`*`
退格，`D` 取消。这样键盘不会把半截数字直接送进 ADC 或 DDS。

```c
#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>

#define APP_INPUT_CAPACITY   (8U)                 /* 7 位数字 + 结尾 '\0'。 */
#define APP_INPUT_MAX_VALUE  (200000U)            /* 题目允许的最大整数。 */

static char g_input_buffer[APP_INPUT_CAPACITY];   /* 正在编辑的文本。 */
static size_t g_input_length;                     /* 已输入字符数。 */
static uint32_t g_input_old_value;                /* 取消时恢复的旧值。 */
static uint32_t g_target_value = 100000U;         /* 仅确认后改变。 */
static bool g_input_active;                       /* 是否处于编辑态。 */

static bool App_ParseUint32(const char *text,
                            size_t length,
                            uint32_t *value)
{
    size_t i;                                     /* 当前字符下标。 */
    uint32_t result = 0U;                         /* 逐位累积的结果。 */

    if ((text == NULL) || (value == NULL) || (length == 0U)) {
        return false;                             /* 空输入或空指针无效。 */
    }
    for (i = 0U; i < length; ++i) {
        uint32_t digit;                           /* 当前数字。 */
        if ((text[i] < '0') || (text[i] > '9')) return false;
        digit = (uint32_t)(text[i] - '0');
        if (result > (APP_INPUT_MAX_VALUE - digit) / 10U) return false;
        result = result * 10U + digit;            /* 追加一位并检查溢出。 */
    }
    *value = result;                               /* 返回解析值。 */
    return true;                                  /* 格式和范围均正确。 */
}

static void App_InputBegin(void)
{
    g_input_length = 0U;                          /* 清掉旧预输入。 */
    g_input_buffer[0] = '\0';                     /* 保持 C 字符串有效。 */
    g_input_old_value = g_target_value;           /* 记录取消恢复值。 */
    g_input_active = true;                        /* 进入编辑态。 */
}

static void App_InputAppendDigit(char symbol)
{
    if ((symbol < '0') || (symbol > '9')) return;  /* 非数字键忽略。 */
    if (!g_input_active) App_InputBegin();        /* 首个数字自动开始。 */
    if (g_input_length + 1U >= APP_INPUT_CAPACITY) return;
                                                   /* 预留字符串结束符。 */
    g_input_buffer[g_input_length++] = symbol;    /* 追加当前数字。 */
    g_input_buffer[g_input_length] = '\0';        /* 立即封口。 */
}

static void App_InputBackspace(void)
{
    if (!g_input_active || (g_input_length == 0U)) return;
    --g_input_length;                              /* 删除最后一位。 */
    g_input_buffer[g_input_length] = '\0';        /* 截断字符串。 */
}

static void App_InputCommit(void)
{
    uint32_t value;                                /* 本次待提交值。 */
    if (!g_input_active) return;                   /* 没有编辑内容。 */
    if (App_ParseUint32(g_input_buffer, g_input_length, &value)) {
        g_target_value = value;                   /* 仅成功时改变参数。 */
        g_input_active = false;                   /* 退出编辑态。 */
    }
}

static void App_InputCancel(void)
{
    if (!g_input_active) return;                   /* 非编辑态不处理。 */
    g_target_value = g_input_old_value;            /* 恢复旧参数。 */
    g_input_length = 0U;
    g_input_buffer[0] = '\0';
    g_input_active = false;                        /* 退出编辑态。 */
}

static void App_HandleInputKey(char symbol)
{
    if ((symbol >= '0') && (symbol <= '9')) App_InputAppendDigit(symbol);
    if (symbol == '*') App_InputBackspace();       /* *：退格。 */
    if (symbol == '#') App_InputCommit();          /* #：确认。 */
    if (symbol == 'D') App_InputCancel();          /* D：取消。 */
}
```

主循环或低频任务先调用模块的 `SignalMatrixKeypad4x4_ReadNewSymbol()`，得到 `symbol`
后再调用 `App_HandleInputKey(symbol)`。显示预输入时清除固定宽度区域再画
`g_input_buffer`；不要在键盘扫描中断里刷新 TFT。`g_target_value`、缓冲区和解析函数
全部是题目自写代码，逐行作用已写在注释中。

