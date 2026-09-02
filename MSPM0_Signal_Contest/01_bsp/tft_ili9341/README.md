# TFT ILI9341：SPI 彩屏

## CCS SysConfig GUI Configuration

### Required resources

需要 `SPI` + `GPIO`；SPI module 名称、`SPI1` 硬件 instance、`DL_SPI_*` DriverLib 名称分别记录。当前驱动还需要 DC/背光两个 GPIO，MISO 当前不参与写屏链路。

### Step 1 - SPI controller and PinMux

GUI Path: 左侧 `Software` -> `Add` -> `SPI` -> 实例 `SPI_TFT` -> `Basic Configuration`；设置 `Controller/Peripheral Mode`、`Word Length`、`Clock Polarity`、`Clock Phase`，再展开 `PinMux Peripheral and Pin Configuration` 分配 SCLK/PICO/CS0。

Action: 进入 `Basic Configuration` -> `Controller/Peripheral Mode` 选择 Controller（主机）、`Word Length` 选择 8 bits、`Clock Polarity`/`Clock Phase` 按 ILI9341 时序选择 Mode 0；再进入 `PinMux Peripheral and Pin Configuration` 设置 SCLK、PICO/MOSI、CS0。已验证 example 的 SPI1/PB9/PB8/PB6 仅作基线，当前接线优先；MISO/POCI 仅在需要读回时配置。

### Step 2 - SPI clock configuration

GUI Path: 左侧 `SPI` -> `SPI_TFT` -> `Basic Configuration` -> `Clock Configuration`（部分 SDK 在 `SPI Controller Advanced Configuration`）-> `Functional Clock Source`、`Clock Divider/Prescaler`、`Baud Rate/SCLK Frequency`。

Action: 在 `Basic Configuration` -> `Clock Configuration`（部分 SDK 在 `SPI Controller Advanced Configuration`）进入 `Functional Clock Source`、`Clock Divider/Prescaler`、`Baud Rate/SCLK Frequency`，核对右侧计算值并满足 ILI9341 的 SCLK 上限。CPU/BUSCLK 不是屏幕 SCLK。

### Step 3 - GPIO and PinMux

GUI Path: 左侧 `Software` -> `Add` -> `GPIO` -> 实例 `GPIO_TFT_CTRL` -> `Group Pins` -> `ADD`；分别创建 `TFT_DC`、`TFT_BLK`，设置 `Direction = Output`、`Initial Value`，再到 `PinMux Peripheral and Pin Configuration` 分配 Port/Pin。

Action: 创建 `TFT_DC`=`PB15` 和 `TFT_BLK`=`PB12`；方向为 output，`TFT_BLK` 初始值为 SET。`Direction`、`Initial Value`、`Assigned Port/Pin` 等 GPIO 字段已由 GPIO 截图和 TFT `.syscfg` 确认。保存后在 PinMux 冲突视图确认 SPI1 与两个 GPIO 不冲突。

### Expected generated symbols

Generate 后核对 `SPI_TFT_INST`、SPI pin macros、`GPIO_TFT_CTRL_PORT`、`GPIO_TFT_CTRL_TFT_DC_PIN`、`GPIO_TFT_CTRL_TFT_BLK_PIN` 和 `CPUCLK_FREQ`。PROJECT_AUDIT 记录 `GUI field -> .syscfg property -> generated symbol`。

保存后点击 Generate，并在 `ti_msp_dl_config.h` 核对 SPI instance、SCLK/PICO/CS0 与 TFT 控制 GPIO 宏。

### Final checklist / Common mistakes / Do not change

- SPI1、PB9/PB8/PB6、PB15/PB12 与 `DL_SPI_*` 名称已分开。
- SPI clock/baud 以 GUI 实际值和屏幕实测为准；不要把 MISO 当 MOSI。
- 不直接编辑 `.syscfg` 或生成文件；不要在资源冲突时擅自换 Pin。

## 0. 什么时候用

当题目需要显示多行数值、菜单、曲线或简单图形时使用；24_C 用它绘制一次静态坐标轴、单位和标签，再只刷新波形矩形和数字字段。只亮灭 LED 或输出串口无需它。当前比赛入口锁定 MSPM0G3507 + SysConfig SPI，使用内置 ASCII 字库和少量示例中文字模；ISR 不得调用 TFT/SPI。

## 1. 30 秒接入路线

你需要复制：`signal_tft_ili9341.c`、`signal_tft_ili9341.h`、`signal_tft_ili9341_font_data.inc`、`signal_tft_ili9341_mspm0g3507.c`、`signal_tft_ili9341_mspm0g3507.h`、`01_bsp/common/signal_status.h`。本模块【需要 SysConfig】。

按第 3 节接线并配置 `SPI_TFT`/`GPIO_TFT_CTRL`，复制第 4 节全部文件，粘贴初始化和绘制代码，先显示纯色与 `MSPM0`，成功后再做界面。

## 2. 输入和输出

- 输入：坐标、颜色 RGB565、ASCII 字符串、整数/浮点数或单色位图。
- 输出：ILI9341 屏幕图像。
- 默认原生尺寸 240×320；旋转 90°/270° 后逻辑尺寸为 320×240。

## 3. 接线、SysConfig 与 Pin

【需要 SysConfig】。示例见 `09_examples/tft_ili9341_lp_mspm0g3507/tft_ili9341.syscfg`。

### 3.1 初学者接线表

| 屏幕常见标识 | 含义 | 示例接 MSPM0G3507 | 说明 |
|---|---|---|---|
| VCC | 模块电源 | 按你的模块规格接 3.3 V；不确定先查模块丝印/原理图 | MCU IO 是 3.3 V 逻辑，禁止把 5 V 逻辑直接灌入 MCU |
| GND | 地 | GND | 必须共地 |
| SCK/CLK | SPI 时钟 | PB9 / SPI1 SCLK | SysConfig 分配 |
| MOSI/SDI | MCU 发给屏幕 | PB8 / SPI1 MOSI | 必需 |
| MISO/SDO | 屏幕返回数据 | 不接 | 当前驱动只写 |
| CS | 片选 | PB6 / SPI1 CS0 | 示例由硬件 CS 控制 |
| DC/RS | 命令/数据选择 | PB15 | 普通 GPIO 输出 |
| RST/RESET | 屏幕复位 | 示例未由 MCU 控制，可按模块要求上拉/接复位 | 当前入口 `set_reset=NULL` |
| LED/BL/BLK | 背光 | PB12 | 普通 GPIO 输出；模块背光电流过大时需驱动管 |

不同商家排针顺序可能完全不同，以丝印和模块原理图为准，不要按“第几个针”盲接。

### 3.2 SysConfig 步骤

1. 左侧 `Add` -> `SPI`，实例名设为 `SPI_TFT`；进入 `Basic Configuration` 依次设置 Controller mode、8-bit word、CPOL/CPHA（ILI9341 常用 Mode 0），再进入 `Clock Configuration` 设置 functional clock、divider/prescaler 与 baud/SCLK。
2. 分配 SCLK=PB9、MOSI=PB8、CS0=PB6；MISO 当前不用。
3. 添加 GPIO group，实例名必须为 `GPIO_TFT_CTRL`，输出名必须包含 `TFT_DC` 和 `TFT_BLK`；示例 PB15/PB12。
4. BLK 初值设高；DC 初值无硬性要求，初始化会切换。
5. Generate 后检查 `SPI_TFT_INST`、`GPIO_TFT_CTRL_PORT`、`GPIO_TFT_CTRL_TFT_DC_PIN`、`...TFT_BLK_PIN` 和 `CPUCLK_FREQ`；再从 GUI 实际显示值核对 `SPI functional clock -> baud divider/control -> SCLK`，不要把 `CPUCLK_FREQ` 当作 SCLK。
6. 换 Pin 时只在 SysConfig 换合法 SPI/GPIO Pin；模块 `.c` 使用生成宏，不需要改物理 Pin 常量。

## 4. 复制哪些文件

从本目录复制全部这些文件到母版 `modules/`：

- `signal_tft_ili9341.c`
- `signal_tft_ili9341.h`
- `signal_tft_ili9341_font_data.inc`
- `signal_tft_ili9341_mspm0g3507.c`
- `signal_tft_ili9341_mspm0g3507.h`

再复制 `01_bsp/common/signal_status.h`。`.inc` 是字库数据，不能漏；不需要复制旧 TFT Platform/Adapter。

## 5. main.c 顶部复制什么

```c
#include "ti_msp_dl_config.h"
#include "signal_tft_ili9341.h"
#include "signal_tft_ili9341_mspm0g3507.h"

static tft_ili9341_t g_tft;
volatile tft_ili9341_status_t g_tft_status;
```

## 6. 比赛参数

| 题目变化 | 修改 |
|---|---|
| 横屏/竖屏 | `TFT_ILI9341_ROTATION_*` |
| 字体大小 | `FONT_6X12 / 8X16 / 12X24 / 16X32` |
| 颜色 | RGB565 常量或 `TFT_ILI9341_RGB565(r,g,b)` |
| 换 SPI/Pin | `.syscfg`，保持实例/输出名称 |
| 屏幕不稳定 | 先降低 SPI 时钟，检查供电、地线、CS/DC/RST |

## 7. 初始化区复制什么

放在 `SYSCFG_DL_init();` 之后：

```c
g_tft_status = SignalTFTILI9341_MSPM0_Init(
    &g_tft, TFT_ILI9341_ROTATION_90);
if (g_tft_status != TFT_ILI9341_OK) {
    while (1) { }
}

g_tft_status = TFT_ILI9341_FillScreen(&g_tft, TFT_ILI9341_BLACK);
```

入口内部已经绑定 SPI、DC、背光和延时；使用者不需要理解 callback。

## 8. while(1) / 绘制区复制什么

```c
if (g_tft_status == TFT_ILI9341_OK) {
    g_tft_status = TFT_ILI9341_DrawString(
        &g_tft, 8, 8, "MSPM0",
        TFT_ILI9341_FONT_8X16,
        TFT_ILI9341_WHITE, TFT_ILI9341_BLACK,
        false, false);
}

// ===== 这里写你自己的逻辑 =====
// TFT_ILI9341_DrawFloat(...);  显示测量值
// TFT_ILI9341_DrawLine(...);   画波形
```

## 8.1 双 ADC 李萨如图（22_X 可直接复制的应用代码）

当上游已经按 `adc_dual_sync` README 得到 `g_raw_a[]` 与 `g_raw_b[]` 后，不能使用单路波形模块代替 X/Y 平面绘图。将 A 路原始码映射到横坐标、B 路原始码映射到纵坐标，并对每两个相邻点调用 `TFT_ILI9341_DrawLine()`。下面代码只属于应用层，放在 `main.c`，不修改本 TFT 驱动：

```c
#define LISSAJOUS_PLOT_X       (20)
#define LISSAJOUS_PLOT_Y       (30)
#define LISSAJOUS_PLOT_WIDTH   (280)
#define LISSAJOUS_PLOT_HEIGHT  (180)
#define LISSAJOUS_POINT_COUNT  (280U)
#define ADC12_FULL_SCALE       (4095U)
#define LISSAJOUS_INNER_X      (LISSAJOUS_PLOT_X + 1)
#define LISSAJOUS_INNER_Y      (LISSAJOUS_PLOT_Y + 1)
#define LISSAJOUS_INNER_WIDTH  (LISSAJOUS_PLOT_WIDTH - 2)
#define LISSAJOUS_INNER_HEIGHT (LISSAJOUS_PLOT_HEIGHT - 2)

static int32_t Lissajous_MapX(uint16_t sample)
{
    return LISSAJOUS_INNER_X +
        (int32_t)(((uint32_t)sample * (LISSAJOUS_INNER_WIDTH - 1)) /
                  ADC12_FULL_SCALE);
}

static int32_t Lissajous_MapY(uint16_t sample)
{
    return LISSAJOUS_INNER_Y + LISSAJOUS_INNER_HEIGHT - 1 -
        (int32_t)(((uint32_t)sample * (LISSAJOUS_INNER_HEIGHT - 1)) /
                  ADC12_FULL_SCALE);
}

static tft_ili9341_status_t Lissajous_DrawStaticFrame(void)
{
    tft_ili9341_status_t status;

    status = TFT_ILI9341_FillRect(
        &g_tft, LISSAJOUS_PLOT_X, LISSAJOUS_PLOT_Y,
        LISSAJOUS_PLOT_WIDTH, LISSAJOUS_PLOT_HEIGHT,
        TFT_ILI9341_BLACK);
    if (status != TFT_ILI9341_OK) return status;
    return TFT_ILI9341_DrawRect(
        &g_tft, LISSAJOUS_PLOT_X, LISSAJOUS_PLOT_Y,
        LISSAJOUS_PLOT_WIDTH, LISSAJOUS_PLOT_HEIGHT,
        TFT_ILI9341_BLUE);
}

static tft_ili9341_status_t Lissajous_DrawFrame(void)
{
    uint16_t point;
    uint32_t index0;
    uint32_t index1;
    tft_ili9341_status_t status;

    status = TFT_ILI9341_FillRect(
        &g_tft, LISSAJOUS_INNER_X, LISSAJOUS_INNER_Y,
        LISSAJOUS_INNER_WIDTH, LISSAJOUS_INNER_HEIGHT,
        TFT_ILI9341_BLACK);
    if (status != TFT_ILI9341_OK) return status;

    for (point = 0U; point + 1U < LISSAJOUS_POINT_COUNT; ++point) {
        index0 = ((uint32_t)point * (SIGNAL_SAMPLE_COUNT - 1U)) /
                 (LISSAJOUS_POINT_COUNT - 1U);
        index1 = ((uint32_t)(point + 1U) * (SIGNAL_SAMPLE_COUNT - 1U)) /
                 (LISSAJOUS_POINT_COUNT - 1U);
        status = TFT_ILI9341_DrawLine(
            &g_tft, Lissajous_MapX(g_raw_a[index0]),
            Lissajous_MapY(g_raw_b[index0]),
            Lissajous_MapX(g_raw_a[index1]),
            Lissajous_MapY(g_raw_b[index1]), TFT_ILI9341_YELLOW);
        if (status != TFT_ILI9341_OK) return status;
    }
    return TFT_ILI9341_OK;
}
```

调用顺序仍是：TFT 初始化后先调用一次 `Lissajous_DrawStaticFrame()`，再循环执行 `SignalDualADC_Start()` → `SignalDualADC_IsFinished()` → `Lissajous_DrawFrame()`。`Lissajous_DrawFrame()` 只清除蓝色边框内侧并绘制黄色轨迹，不重复绘制标题和蓝色边框。不要在 ADC/DMA ISR 中画 TFT；`g_raw_a[]` 和 `g_raw_b[]` 必须在采集完成后再读取。`SIGNAL_SAMPLE_COUNT=1024` 时抽取为 280 个屏幕点，避免每帧发送 1023 条重复线段。

完整代码见 `README_MINIMAL_EXAMPLE.c`。

## 9. 字库说明

`signal_tft_ili9341_font_data.inc` 内含可打印 ASCII 0x20～0x7E 的 6×12、8×16、12×24、16×32 字体。中文不包含完整 GB2312/Unicode 字库；当前仅提供“电”“子”两个 16×16 示例字模。其他中文/图标需要你把选定字形转换成单色位图，再调用 `TFT_ILI9341_DrawMonoBitmap`，不建议比赛时塞入整套巨大中文字库。

## 10. 结果与连接

常见连接是 `测量/FFT结果 -> 格式化数值 -> TFT DrawInt32/DrawFloat`。画波形时对采样点做横向抽取/缩放，不要每帧无条件全屏清除，否则闪烁且 SPI 开销很大。

## 11. Build 与最小验证

第一步只做黑底白字；第二步测旋转与四角坐标；第三步再画曲线。隔离 COPY TEST：`SysConfig / Compile / Full Link PASS`，Flash 17864 B、SRAM（含栈）597 B；本轮未上屏实测，状态 `BUILD_VERIFIED`。

## 12. 常见错误

- 白屏但背光亮：SPI Mode/时钟、CS、DC、RST 或初始化时序错误。
- `font_data.inc not found`：漏复制 `.inc`。
- 颜色异常：RGB565 数据顺序或屏幕模块兼容性问题。
- 图像方向错：修改 rotation，不要手动交换所有坐标。
- 偶尔花屏：先降 SPI 时钟、缩短线、加强电源去耦并确认共地。

## 13. API Reference

- 比赛入口：`SignalTFTILI9341_MSPM0_Init(tft, rotation)`。
- 屏幕控制：`TFT_ILI9341_Init`、`SetRotation`、`SetBacklight`、`GetWidth/Height`。
- 图形：`DrawPixel`、`FillRect`、`FillScreen`、`DrawLine`、`DrawRect`、`DrawRGB565`。
- 文字：`GetFontMetrics`、`DrawChar`、`DrawString`、`DrawInt32`、`DrawFloat`。
- 位图：`DrawMonoBitmap`；格式是逐行、每行 `ceil(width/8)` 字节、LSB first。
- 高级底层：`WriteCommand`、`WriteData`、`SetAddressWindow`；普通使用无需直接调用。
- `SignalTFTILI9341_GetModuleStatus()`：正式核心驱动成熟度。

## 14. 初学者必须先懂的概念

这一节回答“为什么调用里有这么多参数”。比赛应用层只需要理解这里的概念，不需要重写 ILI9341 寄存器和 SPI 时序。

### 14.1 坐标原点与旋转后的尺寸

逻辑坐标原点在屏幕左上角：

```text
(0,0) ─────────────→ x 增大
  │
  │
  ↓
 y 增大
```

- `ROTATION_0/180` 时逻辑尺寸为 240×320；
- `ROTATION_90/270` 时逻辑尺寸为 320×240；
- `(x, y)` 表示要绘制内容的左上角；
- 坐标和宽高都使用像素作为单位。

修改旋转方向后，先用 `TFT_ILI9341_GetWidth/Height` 确认逻辑尺寸，不要在应用层手工交换所有坐标。

### 14.2 屏幕对象与 `&g_tft`

```c
static tft_ili9341_t g_tft;
```

`g_tft` 是一块屏幕的软件对象，内部保存回调、逻辑宽高、旋转方向和初始化状态。

```c
&g_tft
```

表示 `g_tft` 在 RAM 中的地址。绘图函数接收这个地址，才能知道应该操作哪块屏幕及其当前状态。可以把：

```c
TFT_ILI9341_FillScreen(&g_tft, TFT_ILI9341_BLACK);
```

读成“使用 `g_tft` 这块屏幕，把整屏填成黑色”。

### 14.3 状态返回值

绝大多数函数返回 `tft_ili9341_status_t`：

| 返回值 | 含义 |
|---|---|
| `TFT_ILI9341_OK` | 成功 |
| `TFT_ILI9341_ERROR_ARGUMENT` | 参数无效，例如空指针或错误尺寸 |
| `TFT_ILI9341_ERROR_IO` | SPI/GPIO 等底层通信失败 |
| `TFT_ILI9341_ERROR_NOT_INITIALIZED` | 屏幕还没有成功初始化 |

初学阶段推荐保存并检查返回值：

```c
g_tft_status = TFT_ILI9341_FillScreen(&g_tft, TFT_ILI9341_BLACK);
if (g_tft_status != TFT_ILI9341_OK) {
    while (1) { }
}
```

### 14.4 字体占用空间

| 字体 | 单字符宽度 | 单字符高度 |
|---|---:|---:|
| `FONT_6X12` | 6 | 12 |
| `FONT_8X16` | 8 | 16 |
| `FONT_12X24` | 12 | 24 |
| `FONT_16X32` | 16 | 32 |

例如 `"FREQ:"` 有 5 个字符，使用 8×16 字体时约占 `5×8=40` 像素宽、16 像素高。安排下一行 y 坐标时必须留出字体高度。

### 14.5 RGB565 颜色

预定义颜色可以直接使用。需要自定义颜色时：

```c
uint16_t color = TFT_ILI9341_RGB565(255U, 128U, 0U);
```

`red`、`green`、`blue` 都按常用的 `0～255` 输入，宏会转换为屏幕使用的 16-bit RGB565。

## 15. 静态内容、动态内容与局部刷新

### 15.1 先分清哪些内容会变化

- 静态内容：标题、标签、单位、边框，上电后画一次；
- 动态内容：频率、幅度、测量值、输入数字，数据变化后再画。

不要在高速 `while (1)` 中反复重画所有标签。

### 15.2 为什么数字变短会残留

如果先显示 `1000000`，再直接在相同位置显示 `10`，新字符串只覆盖前两个字符，后面的旧字符可能仍留在屏幕上。

正确顺序是：

```text
用背景色擦除数值区域
        ↓
在同一位置画新数值
```

【ILLUSTRATIVE SNIPPET】下面是通用写法，变量名和坐标需要按应用修改：

```c
status = TFT_ILI9341_FillRect(
    &g_tft, 80, 72, 152, 16, TFT_ILI9341_BLACK);
if (status == TFT_ILI9341_OK) {
    status = TFT_ILI9341_DrawInt32(
        &g_tft, 80, 72, display_value,
        TFT_ILI9341_FONT_8X16,
        TFT_ILI9341_GREEN, TFT_ILI9341_BLACK, false);
}
```

`FillRect` 的宽度要能覆盖旧值可能占用的最大宽度。

即使 `transparent_background=false`，它也只会绘制“新字符串实际包含的字符格”，无法自动擦掉更长旧字符串超出的部分，所以数值可能变短时仍建议先 `FillRect`。

### 15.3 dirty 按需刷新

如果主循环很快，持续画屏会占用 SPI、降低按键和采样响应，还可能闪烁。可以增加 dirty 标志。

【ILLUSTRATIVE SNIPPET】通用结构：

```c
static volatile bool g_tft_dirty = true;

static void App_SetDisplayValue(int32_t new_value)
{
    g_display_value = new_value;
    g_tft_dirty = true;
}

static tft_ili9341_status_t App_RefreshDisplay(void)
{
    tft_ili9341_status_t status;

    if (!g_tft_dirty) {
        return TFT_ILI9341_OK;
    }

    status = TFT_ILI9341_FillRect(
        &g_tft, 80, 72, 152, 16, TFT_ILI9341_BLACK);
    if (status == TFT_ILI9341_OK) {
        status = TFT_ILI9341_DrawInt32(
            &g_tft, 80, 72, g_display_value,
            TFT_ILI9341_FONT_8X16,
            TFT_ILI9341_GREEN, TFT_ILI9341_BLACK, false);
    }

    if (status == TFT_ILI9341_OK) {
        g_tft_dirty = false;
    }
    return status;
}
```

主循环只需：

```c
if (g_tft_dirty) {
    g_tft_status = App_RefreshDisplay();
}
```

正确的数据流应是：

```text
按键/测量/算法产生新值
    → 更新业务变量
    → dirty=true
    → 主循环局部刷新
    → 成功后 dirty=false
```

屏幕只是业务变量的显示结果。控制 DDS、DAC 或其他硬件时，不要反过来从屏幕字符串中取值。

### 15.4 为什么不在按键回调里直接画屏

推荐让按键代码只负责修改变量和设置 dirty，让主循环统一绘图。这样：

- 按键逻辑与显示逻辑分离；
- 所有坐标和颜色集中在显示函数；
- 以后换页面、换显示设备时更容易修改；
- 避免在中断或严格时序代码里执行耗时 SPI 绘图。

## 16. 常用函数逐参数说明

### 16.1 MSPM0G3507 比赛初始化入口

```c
tft_ili9341_status_t SignalTFTILI9341_MSPM0_Init(
    tft_ili9341_t *tft,
    tft_ili9341_rotation_t rotation);
```

| 参数 | 含义 |
|---|---|
| `tft` | 屏幕对象地址，通常传 `&g_tft` |
| `rotation` | `ROTATION_0/90/180/270` 之一 |

该入口内部绑定 SysConfig 生成的 SPI、DC、背光和延时。锁定 MSPM0G3507 的新工程优先使用此入口。

### 16.2 全屏与矩形

```c
TFT_ILI9341_FillScreen(tft, color)
```

| 参数 | 含义 |
|---|---|
| `tft` | 屏幕对象地址 |
| `color` | 整屏填充颜色，RGB565 |

```c
TFT_ILI9341_FillRect(tft, x, y, width, height, color)
```

| 参数 | 含义 |
|---|---|
| `tft` | 屏幕对象地址 |
| `x`、`y` | 矩形左上角 |
| `width`、`height` | 矩形宽高，像素 |
| `color` | 填充颜色 |

### 16.3 字符串

```c
TFT_ILI9341_DrawString(
    tft, x, y, text, font,
    foreground, background,
    transparent_background, wrap)
```

| 参数 | 含义 |
|---|---|
| `tft` | 屏幕对象地址 |
| `x`、`y` | 字符串左上角 |
| `text` | 以 `\0` 结尾的 ASCII 字符串 |
| `font` | 四种内置字体之一 |
| `foreground` | 字符笔画颜色 |
| `background` | 字符背景颜色 |
| `transparent_background` | `true` 不画字符空白像素；`false` 画背景色 |
| `wrap` | 到达右边界后是否自动换行 |

动态数值附近通常用 `transparent_background=false`，行为更可预测；但旧字符串更长时仍要先擦除整个数值区域。

### 16.4 整数

```c
TFT_ILI9341_DrawInt32(
    tft, x, y, value, font,
    foreground, background,
    transparent_background)
```

| 参数 | 含义 |
|---|---|
| `tft` | 屏幕对象地址 |
| `x`、`y` | 数字左上角 |
| `value` | 要显示的 `int32_t` 整数 |
| `font` | 字体 |
| `foreground`、`background` | 数字颜色和背景颜色 |
| `transparent_background` | 是否跳过空白像素 |

`uint32_t` 业务变量在确认不会超过 `INT32_MAX` 后，可显式转换为 `(int32_t)value`。

### 16.5 浮点数

```c
TFT_ILI9341_DrawFloat(
    tft, x, y, value, decimal_places,
    font, foreground, background,
    transparent_background)
```

| 参数 | 含义 |
|---|---|
| `tft` | 屏幕对象地址 |
| `x`、`y` | 数字左上角 |
| `value` | 要显示的 `float` |
| `decimal_places` | 小数点后保留位数 |
| `font` | 字体 |
| `foreground`、`background` | 数字颜色和背景颜色 |
| `transparent_background` | 是否跳过空白像素 |

例如 `value=2.5f`、`decimal_places=2U` 时显示 `2.50`。

## 17. 全部公开 API 与参数索引

真正的参数顺序始终以 `signal_tft_ili9341.h` 为准。本节用于离线快速定位。

### 17.1 屏幕控制

| 函数 | 参数 | 用途 |
|---|---|---|
| `TFT_ILI9341_Init` | `tft, config, rotation` | 通用回调式初始化；MSPM0G3507 普通应用优先用专用入口 |
| `TFT_ILI9341_SetRotation` | `tft, rotation` | 运行时改变方向和逻辑宽高 |
| `TFT_ILI9341_SetBacklight` | `tft, on` | `true` 开背光，`false` 关背光 |
| `TFT_ILI9341_GetWidth` | `tft` | 返回当前逻辑宽度 |
| `TFT_ILI9341_GetHeight` | `tft` | 返回当前逻辑高度 |

`config` 是 `tft_ili9341_config_t`，字段含义：

| 字段 | 含义 |
|---|---|
| `context` | 传给所有回调的用户上下文 |
| `write` | SPI/总线写数据回调，必需 |
| `set_cs` | 软件片选回调；硬件 CS 时可空 |
| `set_dc` | 命令/数据选择 GPIO 回调，必需 |
| `set_reset` | 硬复位 GPIO 回调，可选 |
| `set_backlight` | 背光控制回调，可选 |
| `delay_ms` | 毫秒延时回调，必需 |
| `lock`、`unlock` | 多任务共享 SPI 时的可选同步回调 |

MSPM0G3507 专用入口已经创建并绑定这些字段，普通比赛应用不用自己填写。

### 17.2 基础图形

| 函数 | 参数 | 用途 |
|---|---|---|
| `DrawPixel` | `tft, x, y, color` | 画一个像素 |
| `FillRect` | `tft, x, y, width, height, color` | 填充矩形 |
| `FillScreen` | `tft, color` | 填充整屏 |
| `DrawLine` | `tft, x0, y0, x1, y1, color` | 两点间画线 |
| `DrawRect` | `tft, x, y, width, height, color` | 画空心矩形边框 |
| `DrawRGB565` | `tft, x, y, width, height, pixels` | 显示 RGB565 像素数组 |

参数补充：

- `x0,y0` 是线段起点，`x1,y1` 是终点；
- `pixels` 指向至少 `width×height` 个 `uint16_t` 像素；
- `color`、`foreground`、`background` 都是 RGB565。

### 17.3 文字与位图

| 函数 | 参数 | 用途 |
|---|---|---|
| `GetFontMetrics` | `font, width, height` | 把字体宽高写入两个输出变量 |
| `DrawMonoBitmap` | `tft, x, y, width, height, bitmap, bitmap_size, foreground, background, transparent_background` | 显示单色字模/图标 |
| `DrawChar` | `tft, x, y, character, font, foreground, background, transparent_background` | 显示一个 ASCII 字符 |
| `DrawString` | `tft, x, y, text, font, foreground, background, transparent_background, wrap` | 显示字符串 |
| `DrawInt32` | `tft, x, y, value, font, foreground, background, transparent_background` | 显示整数 |
| `DrawFloat` | `tft, x, y, value, decimal_places, font, foreground, background, transparent_background` | 显示浮点数 |

参数补充：

- `width`、`height` 在 `GetFontMetrics` 中是输出指针，通常传 `&char_width`、`&char_height`；
- `bitmap` 是单色位图数组地址；
- `bitmap_size` 是数组字节数；
- `character` 是一个字符，例如 `'A'`；
- `text` 是字符串，例如 `"FREQ"`。

### 17.4 高级底层接口

| 函数 | 参数 | 用途 |
|---|---|---|
| `WriteCommand` | `tft, command` | 发送一个 ILI9341 命令字节 |
| `WriteData` | `tft, data, length` | 发送 `length` 字节数据 |
| `SetAddressWindow` | `tft, x0, y0, x1, y1` | 设置后续像素写入区域 |

普通应用层不要直接使用这三个函数；画文字、数字、线条和位图时让上层 API 调用它们。

### 17.5 模块状态

```c
SignalTFTILI9341_GetModuleStatus(void)
```

无参数，返回模块成熟度信息，不表示某一次屏幕绘图是否成功。单次操作结果仍看 `tft_ili9341_status_t`。

## 18. 从空白 main.c 独立写显示代码的顺序

1. 按第 3 节配置 SysConfig 和接线；
2. 按第 4 节复制文件；
3. 包含两个 TFT 头文件；
4. 声明 `g_tft` 和状态变量；
5. 在 `SYSCFG_DL_init()` 后初始化 TFT；
6. 先验证黑底白字，不要直接写复杂界面；
7. 把标题、标签、单位作为静态内容画一次；
8. 把测量值或设置值作为动态内容；
9. 数值变化时设置 dirty；
10. 刷新时先局部擦除，再画新值；
11. 不在 ADC ISR、定时器 ISR 或严格时序函数中执行大块 SPI 绘图；
12. Build 报错时先按 `.h` 核对函数名、参数数量和类型。

离线查阅路线：

```text
不知道接线/SysConfig/复制文件
→ README.md

不知道初始化入口
→ signal_tft_ili9341_mspm0g3507.h

不知道绘图函数参数
→ signal_tft_ili9341.h

需要最小完整调用顺序
→ README_MINIMAL_EXAMPLE.c
```

## 24_C 成功案例：静态布局与局部刷新

ILI9341 驱动负责像素、文字和基础图元；24_C 的应用层只在初始化或模式切换时绘制一次静态布局（标题、X-Y 坐标轴、单位、网格和字段标签），运行中仅刷新波形矩形区域和变化的数字字段。不要在采样/DMA ISR 中调用任何 TFT/SPI API。

```text
TFT_Init -> DrawStaticLayout()
         -> main: DrawWaveform(snapshot) + UpdateNumericFields()
```

波形区域清除使用固定矩形或背景色；数字字段先清除一个固定宽度，再绘制新字符串，避免新值较短时残留旧字符。波形和数字使用独立的低频 UI 调度，不必每帧 `FillScreen`；SPI 阻塞时间应计入主循环预算，不能影响双 ADC DMA 的持续采集。

本案例的 SysConfig 资源仍是 `SPI_TFT`、`GPIO_TFT_CTRL/TFT_DC` 和 `TFT_BLK`。驱动文件只复制一次，`tft_waveform` 通过它调用基础画线接口，不要复制第二份 ILI9341 驱动或字库。

## 17. 统一 API 教程（已按当前头文件核对）

本节由当前公开头文件、实现中实际出现的状态码和正式模块注册表生成。它补充前文的场景教程；函数签名变化时必须重新运行 `tools/upgrade_formal_beginner_docs.ps1`，不要手工保留旧 API。

遵循仓库的 [Beginner README 标准](../../00_docs/BEGINNER_README_STANDARD.md)：先用最小示例完成一次正常数据流，再按需要阅读全功能示例和本节 API 细节。

### 什么时候用 / 什么时候不要用

当题目需要“tft_ili9341”目录对应的公开功能，并且输入数据、单位和硬件资源满足前文约束时使用本模块。若只需要更简单的上游功能、输入尚未准备好，或需要不同的数据模型/外设资源，应先选择相邻模块而不是强行调用本 API。

### 输入 / 输出

输入由各 API 的只读数组、配置、频率/阈值和平台对象组成；输出写入 result/output/buffer 参数或由 Get API 返回。调用者负责数组容量、生命周期和物理单位；失败返回时输出不是有效结果。

### 调用顺序

```text
SignalTFTILI9341_GetModuleStatus
```

`Init` 一般仅一次；`Set/Configure` 仅在参数变化且模块空闲时执行；`Start/Process/Generate` 是每帧或每次任务入口；`Get/Is` 用于读取已完成的结果；`Stop` 只在需要取消时调用。若本模块没有其中某类 API，以实际列出的函数为准。

### SysConfig 边界

本模块需要 SysConfig。先在 CCS 的 .syscfg 添加并核对：SPI；再按前文的模块专用 GUI 步骤选择实际 pin/instance。保存后让 CCS 重新生成配置，核对生成宏；不要直接修改 	i_msp_dl_config.c/.h，也不要照抄示例 pin 或 DMA/Event 编号。

### 参数分级

- 【比赛必须会】输入/输出数组、`count/length/capacity`、采样率/频率、阈值/增益以及本 README 前文标出的 pin。它们直接影响题目范围、RAM、时间轴或物理单位。
- 【出问题再理解】Timer 时钟、DMA 通道、Event 路由、参考源和 IRQ。它们属于硬件链路，必须与 SysConfig 生成结果一致。
- 【以后进阶】多缓冲、运行时重配置、回调调度和 ISR 优化。先用最小示例完成一帧闭环，再处理吞吐或延迟。

### 常见错误 FAQ

- 参数错误：先检查指针非空、count/capacity 的单位是元素数、频率/阈值单位与上游一致。
- 硬件无结果：不要修改生成文件；回到 SysConfig 核对 pin、instance、时钟、Timer、DMA 和 Event 的完整链路。
- 结果异常：确认上一轮异步采集已经完成，真实 Fs/N/参考电压已传到算法，且没有在 DMA 使用期间改写 buffer。

### `signal_module_status_t SignalTFTILI9341_GetModuleStatus();`

**它做什么：** 读取模块当前的验证成熟度或静态状态，不会启动硬件操作。

**什么时候调用：** 读取模块当前的验证成熟度或静态状态，不会启动硬件操作。

**参数：** 无。

**返回：** 返回 signal_module_status_t 类型结果；调用者应检查该值。

**最小调用形状：** `SignalTFTILI9341_GetModuleStatus(...);`。可直接从 README_MINIMAL_EXAMPLE.c 复制正常流程；README_FULL_EXAMPLE.c 展示全部公开 API，其中取消类 API 会以 #if 0 隔离。

**注意：** 所有指针和数组都由调用者拥有；先检查返回值。异步采集、DMA 或回调还在使用 buffer 时，不能读取结果或改写该 buffer。

### 示例、模块链与验收

- 最小入门：`README_MINIMAL_EXAMPLE.c`，只保留正常入口和结果读取。
- 全功能：`README_FULL_EXAMPLE.c`，以正确顺序展示当前头文件全部公开 API；`Stop` 等非常规路径不会默认执行。
- 模块链：先由上游提供单位、采样率和有效数据，再调用本模块；成功后将输出交给显示、控制、测量或下一步 DSP。硬件资源仍以 SysConfig 合约为唯一来源。
- 文档验收：README/API、两份示例和头文件会由 `tools/validate_beginner_documentation.ps1` 覆盖检查；这只表示文档与源码签名一致，不代替未进行的实板验证。

### 模块链

`上游采集/配置 -> 本模块 -> 检查返回值和结果 -> 测量、显示、控制或下一步 DSP`。只有确认本模块的输出单位和有效状态后，才交给下一模块。

## 19. 比赛通用功能代码

本 README 第 15 节已经说明静态内容、动态字段和 dirty-region 局部刷新；“按键翻页的
页面状态机”没有放进底层驱动，因为页面数量和按键含义属于具体题目的应用。请复制
[CONTEST_FUNCTIONAL_CODE_COOKBOOK.md](../../00_docs/CONTEST_FUNCTIONAL_CODE_COOKBOOK.md)
第 4 节的页面枚举、翻页键和主循环模板。`FillScreen()` 只在首次进入页面或翻页时用，
波形和数字字段每次只清自己的固定矩形。

