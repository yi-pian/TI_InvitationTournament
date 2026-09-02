# 22_X 矩阵键盘 PLL 倍频控制实施步骤（固定 GPIO 一函数版）

## 0. 本步目标和本次修订

目标：用 4x4 矩阵键盘的数字键 `1` 到 `5` 选择 PLL 倍频数；TFT 左上角 `PLL:` 后显示
当前数值；GPIO `A/B/C/D` 控制外部模拟开关和倍频器。

本次经用户明确允许，修改现有 `matrix_keypad_4x4` 模块的 `.c/.h`。此模块库面向固定引脚
的 MSPM0G3507，因此将行列 GPIO、初始化、消抖、鬼键过滤和字符转换封装进模块；应用层
每约 5 ms 只调用一次函数取得按键字符。

未修改双路同步 ADC、DMA、采样 Timer、ILI9341、李萨如图形和 `.syscfg`。不手工编辑任何
`Debug/ti_msp_dl_config.*`、`.mk`、`.o`、`.out` 或链接文件。

## 1. 按比赛步骤选择模块

1. 输入器件是 4x4 矩阵键盘，选择
   `MSPM0_Signal_Contest/01_bsp/matrix_keypad_4x4`。
2. 该模块原有部分负责 4 行 x 4 列扫描、3 次消抖、按下/释放事件和鬼键判断。
3. 本次新增固定 GPIO 接口 `SignalMatrixKeypad4x4_ReadNewSymbol()`；仅在
   `__MSPM0G3507__` 目标下可用。
4. `A/B/C/D` 是本题专用四路输出，直接使用 SysConfig 生成的 GPIO 宏，不另复制 GPIO 模块。

## 2. 先补齐模块 README

先修改模块 README，再修改工程：

- 新增第 10.1 节“MSPM0G3507 固定引脚的一函数最小用法”，给出 `SYSCFG_DL_init()` 后
  启动 1 ms SysTick、每 5 ms 调用 `SignalMatrixKeypad4x4_ReadNewSymbol(&symbol)` 的
  最小闭环代码。
- 旧的应用层行驱动、列读取、延时、初始化与 `pressed_mask` 遍历示例标为“旧版低层适配
  记录（不要复制）”。
- 第 18.3 节给出当前 `main.c` 应复制的 PLL 示例。
- 说明固定键盘引脚若改变，必须同步改 SysConfig、模块 MSPM0G3507 平台代码和 README；
  不可只在 `main.c` 临时改 pin。

README 与工程配置一致。本次没有添加 README 未说明的 SysConfig 设置。

## 3. 复制模块文件到工程

| 来源 | 工程目标 | SHA-256 |
|---|---|---|
| `01_bsp/matrix_keypad_4x4/signal_matrix_keypad_4x4.c` | `signal_contest_template_final/modules/signal_matrix_keypad_4x4.c` | `561B76E2EEA585D6739B25D5F869B96919BA3948B8F95F97795BF5B865691918` |
| `01_bsp/matrix_keypad_4x4/signal_matrix_keypad_4x4.h` | `signal_contest_template_final/modules/signal_matrix_keypad_4x4.h` | `F08E8B6B89F8047AA8CC6F6561D89A0BB0605B5771C60E73F9AF469DD9BC8203` |

两个哈希已逐项一致。`signal_status.h` 是工程已有依赖，不重复复制。

## 4. SysConfig 配置

这次一函数重构**没有改 SysConfig**，完全沿用 README 第 5.2 节与原工程已经生成的配置。

### 4.1 GPIO_KEYPAD

| 键盘线 | 生成名称 | 引脚 | 设置 |
|---|---|---|---|
| R1 | `KEYPAD_R1` | PB16 | Output，初始高 |
| R2 | `KEYPAD_R2` | PB0 | Output，初始高 |
| R3 | `KEYPAD_R3` | PB7 | Output，初始高 |
| R4 | `KEYPAD_R4` | PB17 | Output，初始高 |
| C1 | `KEYPAD_C1` | PB18 | Input，内部上拉 |
| C2 | `KEYPAD_C2` | PB13 | Input，内部上拉 |
| C3 | `KEYPAD_C3` | PB20 | Input，内部上拉 |
| C4 | `KEYPAD_C4` | PB4 | Input，内部上拉 |

四行未选中时均为高；扫描一行时该行拉低；按键闭合后相应列读低。这和 README 完全一致。

### 4.2 GPIO_PLL_CTRL

| 名称 | 引脚 | 方向 | 初始值 |
|---|---|---|---|
| `PLL_A` | PA12 | Output | 0 |
| `PLL_B` | PA13 | Output | 0 |
| `PLL_C` | PA14 | Output | 0 |
| `PLL_D` | PA15 | Output | 0 |

这四脚是本题外部倍频器的专用输出，且不与 ADC、TFT SPI、SWD 和键盘 GPIO 冲突。
生成头文件应有以下宏，模块便利接口直接使用键盘宏：

```c
GPIO_KEYPAD_PORT
GPIO_KEYPAD_KEYPAD_R1_PIN ... GPIO_KEYPAD_KEYPAD_C4_PIN
GPIO_PLL_CTRL_PORT
GPIO_PLL_CTRL_PLL_A_PIN ... GPIO_PLL_CTRL_PLL_D_PIN
```

## 5. PLL 真值表和接线

| 键盘键 / TFT | A | B | C | D | 含义 |
|---:|---:|---:|---:|---:|---|
| 1 | 0 | 0 | 0 | 0 | D 关，持续 1 倍频 |
| 2 | 0 | 1 | 1 | 1 | D 开，`ABC=011`，2 倍频 |
| 3 | 1 | 0 | 1 | 1 | D 开，`ABC=101`，3 倍频 |
| 4 | 0 | 0 | 1 | 1 | D 开，`ABC=001`，4 倍频 |
| 5 | 1 | 1 | 0 | 1 | D 开，`ABC=110`，5 倍频 |

外部电路按 `A/B/C` 位序接线。键盘排线的物理顺序不一定是 R1-R4、C1-C4；接线前用
万用表通断档逐键确认，且不能把矩阵行或列直接短接 3.3 V 或 GND。

## 6. README 复制到 main.c 的代码

从 README 第 18.3 节复制到
`signal_contest_template_final/main.c`：

1. `#include "signal_matrix_keypad_4x4.h"`。
2. `APP_KEYPAD_SCAN_PERIOD_MS`、`g_keypad_status` 和 PLL 显示状态变量。
3. `App_SetPLLMultiplier()`、`App_ProcessPLLKey()`、`SysTick_Handler()` 和
   `App_DrawPLLMultiplier()`。
4. `SYSCFG_DL_init()` 后的 `App_SetPLLMultiplier(1U);`。
5. 主循环用显示版本号判断，仅局部重画 `PLL:` 后的一位数字。

README 最小示例的 `matrix_keypad_4x4_MinimalExample_Start()` 也给出
`SysTick_Config(CPUCLK_FREQ / 1000U)`。本工程已经有自己的 `SysTick_Handler()`，所以从
README 复制时只采用其扫描逻辑，不能在同一工程再定义第二个同名中断函数。

不再复制旧版的 `App_KeypadDriveRow()`、`App_KeypadReadColumn()`、`App_DelayUs()`、
`keypad_config`、`g_keypad`、`SignalMatrixKeypad4x4_Init()` 或
`App_KeypadGetNewSymbol()`；它们已经移入模块。

当前 SysTick 中复制的核心代码：

```c
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
```

## 7. 非复制代码的逐行解释

### 7.1 main.c：本题专用的字符到倍频映射

```c
static void App_ProcessPLLKey(char symbol)
{
    if ((symbol >= '1') && (symbol <= '5')) {
        App_SetPLLMultiplier((uint8_t)(symbol - '0'));
    }
}
```

| 行 | 说明 |
|---|---|
| 1 | 定义仅在本文件使用的函数；输入是模块已确认的按键字符。 |
| 2 | 函数体开始。 |
| 3 | 只允许题目要求的 `'1'` 到 `'5'`；其他 11 个键没有 PLL 含义，忽略。 |
| 4 | ASCII 数字连续，`'4' - '0'` 得到整数 4；转为 `uint8_t` 后交给 PLL 输出函数。 |
| 5 | 结束 `if`。 |
| 6 | 结束函数。 |

`App_SetPLLMultiplier()` 是本题专用输出：先清 A/B/C/D，再按真值表置高所需 pin。先清零
会让 D 暂时为 0，切换时模拟开关先关闭，避免开关打开时短暂经过错误的 ABC 编码；随后保存
当前倍频数并递增显示版本号。

### 7.2 main.c：每 5 ms 调模块的逐行说明

| 行 | 对应代码 | 说明 |
|---:|---|---|
| 1 | `static uint8_t milliseconds;` | 静态变量只初始化一次，记住已过去的 1 ms SysTick 次数。 |
| 2 | `char symbol;` | 保存模块返回的新按键字符。 |
| 4 | `++milliseconds;` | 每次 1 ms 中断加一。 |
| 5 | `if (...) return;` | 未满 5 ms 立即退出，不做矩阵扫描。 |
| 6 | `milliseconds = 0U;` | 满 5 ms 后清零，开始下一段计时。 |
| 7 | `ReadNewSymbol(&symbol)` | 模块自行初始化、扫描 4 行 4 列、等待 5 us、3 次消抖、过滤鬼键，并将可信新键写入 `symbol`。 |
| 8 | `if (... == OK)` | 只在得到可信新字符时处理；`NO_DATA` 表示无新键、按住旧键、消抖中或可能鬼键。 |
| 9 | `App_ProcessPLLKey(symbol);` | 将 `'1'` 到 `'5'` 转成本题 ABCD 输出。 |

中断内不调用 TFT/SPI；TFT 仍在主循环局部更新数字，因此蓝色边框不会因按键扫描而刷新。

### 7.3 模块 .c：固定 GPIO 平台代码逐行说明

新增静态对象与配置（模块 `.c` 约第 200-259 行）：

| 代码 | 说明 |
|---|---|
| `static signal_matrix_keypad_4x4_t g_mspm0g3507_keypad;` | 模块内部保存扫描、候选和稳定状态，应用不再需要 `g_keypad`。 |
| `static bool ..._initialized;` | 记录是否自动初始化，防止每 5 ms 清掉消抖历史。 |
| `mspm0g3507_keypad_drive_row(...)` | 行号 0-3 对应 R1-R4；`active=true` 清零选中行，`false` 拉高释放。 |
| `mspm0g3507_keypad_read_column(...)` | 列号 0-3 对应 C1-C4；上拉输入读 0 时将 `*active` 设 true。 |
| `mspm0g3507_keypad_delay_us(...)` | 按 SysConfig 的 `CPUCLK_FREQ` 换算 `delay_cycles`。 |
| `.settle_us = 5U` | 每次选行后等待 5 us。 |
| `.debounce_scans = 3U` | 同一状态连续 3 次才确认；5 ms 周期下约 15 ms。 |
| `.keymap = NULL` | 采用默认键面：`1 2 3 A / 4 5 6 B / 7 8 9 C / * 0 # D`。 |

新增 `SignalMatrixKeypad4x4_ReadNewSymbol()`（模块 `.c` 约第 261-290 行）：

| 行 | 说明 |
|---:|---|
| 1-5 | 建立本次扫描事件、返回值和按键位置变量。 |
| 7 | `symbol` 为空时返回参数错误，避免写空地址。 |
| 8 | 第一次调用才自动初始化。 |
| 9-10 | 使用固定 GPIO 配置初始化内部对象；初始化会先将四行释放为高。 |
| 11 | 初始化失败直接交还错误。 |
| 12 | 标记完成初始化，让后续扫描保留消抖状态。 |
| 15 | 调用原有通用扫描函数完成 4x4 扫描。 |
| 16 | GPIO 或扫描错误直接交还应用。 |
| 17-19 | 有可能鬼键或没有新确认按键时，返回 `SIGNAL_RESULT_NO_DATA`。 |
| 21-25 | 从位置 0 到 15 找第一位 `pressed_mask`；未置位则继续。 |
| 26-27 | 调用原有 `GetKey()` 将位置转换为键面字符，写入 `*symbol`。 |
| 29 | 理论保护返回，表示无可交付新字符。 |

头文件和源文件都以 `#if defined(__MSPM0G3507__)` 限制这一接口。PC 单元测试仅编译通用
算法；两个 README 示例在非 MSPM0G3507 下提供空函数，避免 PC 编译引用不存在的便利接口。

## 8. 刷新和时序

- ADC 保持 500 kSPS、1024 点，DMA 采集约 2.048 ms，未修改。
- SysTick 每 1 ms 进入一次，每累计 5 次调用键盘模块。
- 单次扫描最多 4 次 5 us 行稳定等待；消抖为连续 3 次扫描。
- `SIGNAL_RESULT_NO_DATA` 不是硬件故障，只表示没有可交付的新字符。
- 只有显示版本不同才重画 `(48, 8)` 的单个数字；蓝色边框和李萨如区域不全屏刷新。

## 9. 验证记录

| 项目 | 结果 |
|---|---|
| SysConfig 资源与生成宏 | PASS，沿用 GPIO_KEYPAD / GPIO_PLL_CTRL，本次未改配置；静态检查另报原有 `.syscfg` 缺少 `@versions` 元数据的警告，未发现引脚冲突。 |
| 源/工程模块哈希 | PASS，`.c/.h` 两项一致，见第 3 节。 |
| `main.c` 与工程键盘模块语法检查 | PASS，TI Arm Clang，`-Wall -Wextra -Werror -fsyntax-only`。 |
| README 示例在 MSPM0G3507 下的语法检查 | PASS。 |
| PC CMake 构建 | 示例平台条件编译已修正；仓库测试仍因既有 `test_signal_library.c` 找不到 `signal_ac_rms.h` 失败，与本键盘改动无关。 |
| CCS 完整链接 | 未执行。先前的 `tmp/pll_keypad_build` 与 `tmp/pll_keypad_syscfg_check` 会被 CCS 误纳入链接并导致重复内存段；删除前需另行明确授权。 |
| 实板键盘、外部 PLL 与 TFT | NOT_RUN。 |

## 10. 赛场复现清单

1. 选择 `matrix_keypad_4x4`，先补齐 README 固定 GPIO 一函数示例。
2. 复制模块 `.c/.h` 到工程 `modules/`，核对哈希一致。
3. 按第 4 节核对 SysConfig 的 8 根键盘线和 4 根 PLL 输出；保存生成，不手改 Debug 文件。
4. 从 README 第 18.3 节复制 `main.c` 代码；应用只保留 PLL 真值表和字符到倍频的专用逻辑。
5. CCS Refresh 后确认 `modules/signal_matrix_keypad_4x4.c` 未 Exclude from Build，再 Build。
6. 下载后按 1-5，观察 TFT 的 `PLL:` 数字；测 PA12-PA15，应符合第 5 节。
7. 若字符错乱，先核对 R/C 排线与列内部上拉，不先修改键面或 SysConfig 宏。
