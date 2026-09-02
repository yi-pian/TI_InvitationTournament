# GMT024-01 / ST7789 2.4-inch SPI TFT

README 类型：`FOREIGN_DRIVER_PORT / COMPILE_VERIFIED_DRIVER / COPY_READY`

当前状态：核心驱动和 MSPM0G3507 平台适配已完成，TI Arm Clang 目标源码编译通过；模块的精确供电、偏移和复位脚仍需实物确认，尚未标记 `BOARD_VERIFIED`。

## 0. 可直接复制的文件

复制下面文件到你的 CCS 工程：

- `signal_status.h`：统一状态枚举。
- `signal_tft_st7789.h`、`signal_tft_st7789.c`：不依赖 TI 的 SPI/GPIO 回调式核心。
- `signal_tft_st7789_mspm0g3507.h`、`signal_tft_st7789_mspm0g3507.c`：绑定 `SPI_TFT_INST`、`GPIO_TFT_CTRL_*` 和 `CPUCLK_FREQ`。
- `signal_tft_st7789_font.h`、`signal_tft_st7789_font.c`、`signal_tft_st7789_font_data.inc`：与 ILI9341 相同的 6×12、8×16、12×24、16×32 ASCII 字库，以及“电”“子”示例字模。
- `README_MINIMAL_EXAMPLE.c`：最小上电、清屏、画矩形示例。

平台层沿用 ILI9341 的 SPI profile（SPI1、PB9 SCLK、PB8 MOSI、PB6 硬件 CS、PB15 DC、PB12 BL）。如果你的 SysConfig 名称不同，只改平台层中的宏映射；不要改生成的 `ti_msp_dl_config.c/.h`。复位脚宏存在时会自动控制，否则按模块板自带复位/上电延时工作。

## 1. 模块身份（资料中能确认的内容）

| 项目 | 资料结论 | 证据 |
|---|---|---|
| 模块标注 | `GMT024-01`，7PIN SPI TFT | `2.4TFT_ST/2.4TFT8PIspi-GMT024-01.ino:2-5` |
| 控制器 | `ST7789V2`（其他旧工程写作 ST7789V） | Arduino 资料第 5 行；旧 C 工程初始化函数 |
| 分辨率 | 240×320 | Arduino 资料第 7、35-36 行；旧 C 代码第 1、238-239 行 |
| 像素格式 | 16-bit RGB565，先发送高字节 | Arduino 资料第 7 行及发送函数 |
| 外部引脚 | SCK、SDA/MOSI、RST、AO/DC、CS、BL；资料中的 7PIN 还要确认第 1/2 脚具体用途 | Arduino 资料第 2-4 行 |
| 传输代码 | 原资料是软件 SPI；SCK 空闲低、上升沿取样，初步按 SPI Mode 0 迁移 | Arduino 资料第 80-106 行 |
| 常见命令 | `0x2A` 列地址、`0x2B` 行地址、`0x2C` 写显存 | Arduino 资料第 130-141 行 |

`AO` 在资料中实际承担数据/命令选择功能，正式代码通常命名为 `DC` 或 `D/C`。不要把旧工程的 `PB5/PB6/PB7/PB8/PB9` 当作 MSPM0 引脚；那些是 STM32F103 工程的硬编码接法。

## 2. 建议的 MSPM0G3507 接线角色

先按“角色”接线，实际 GPIO 由当前工程 SysConfig 决定。下表不是固定开发板引脚，也不是对未知模块电平的最终确认。

| TFT 角色 | 连接到 MSPM0 的方式 | 初次检查 |
|---|---|---|
| VCC | 仅在确认模块电源范围后接 3.3 V 或合适电源 | 万用表量供电；禁止凭网上同名模块猜电压 |
| GND | MSPM0 GND | 必须共地 |
| SCK | SysConfig 分配的 SPI SCLK | 初次低速，先确认波形 |
| SDA / MOSI | SysConfig 分配的 SPI TX/MOSI | 这是显示写入数据线，不是两线总线数据脚 |
| CS | 任意不冲突 GPIO，低有效通常更常见 | 需按模块丝印/原理图确认 |
| AO / DC | 任意不冲突 GPIO，低=命令、高=数据是常见约定 | 需用资料或逻辑分析仪确认 |
| RST | 任意 GPIO；上电按低-高时序复位 | 若板上已有复位电路，仍要查资料 |
| BL | GPIO 或固定电源，取决于背光输入极性和电流 | 不要直接用 MSPM0 GPIO 驱动未知大电流 |

精确模块型号、VCC、IO 电平、CS/DC/RST/BL 极性和第 1/2 脚定义仍是 `DATASHEET_REQUIRED`。确认前不要长时间上电或把 5 V 信号送进 MSPM0。

## 3. SPI 与像素数据的迁移基线

以下是从原始软件 SPI 代码得到的“待验证起点”，不是已经验证的 MSPM0 配置：

- SPI controller，8-bit frame，MSB first；
- 初始 `SPI Mode 0`（CPOL=0、CPHA=0）；若逻辑分析仪证明不同，再改 SysConfig；
- 先使用较低时钟，确认初始化和纯色填充后再提速；
- 每个像素按 `RGB565 >> 8`、`RGB565 & 0xFF` 发送高字节再低字节；
- 写区域顺序为：`DC=0` 发送命令，`DC=1` 发送参数/像素；CS 的保持范围必须按控制器时序确认；
- 窗口通常发送 `0x2A` + X 起止两个 16-bit、`0x2B` + Y 起止两个 16-bit、`0x2C` + 连续像素。

当前初始化表来自 `2.4TFT8PIspi-GMT024-01.ino`，已放进 `TFT_ST7789_Init()`；不同模块可能需要替换初始化表、旋转值或 `x_offset/y_offset`。因此这份代码是“可编译的迁移起点”，不是已经替所有 ST7789 模块验证的通用表。

## 4. 最小 API

`TFT_ST7789_Init`、`SetRotation`、`SetAddressWindow`、`DrawPixel`、`FillRect`、`FillScreen`、`DrawLine`、`DrawRect`、`DrawRGB565` 和 `DrawMonoBitmap` 已提供。字库模块另外提供 `DrawChar`、`DrawString`、`DrawInt32`、`DrawFloat` 和 `GetFontMetrics`，接口和字模尺寸与 ILI9341 字库一致。绘图函数使用 RGB565，所有大图片数据由应用静态提供，不使用 `malloc`。

## 5. 最小 bring-up 顺序

1. 找到模块背面丝印、完整型号、原理图或数据手册，记录 VCC、IO 电平、7 个脚位和背光电流。
2. 在 SysConfig 只配置一个 SPI controller 和需要的 GPIO 角色；保存生成文件，不手改 `ti_msp_dl_config.c/.h`。
3. 先写平台层：`spi_write_byte()`、`gpio_write_cs()`、`gpio_write_dc()`、`gpio_write_rst()`、`gpio_write_bl()`。平台层只调用生成宏和 DriverLib。
4. 用资料确定初始化表，执行复位、Sleep Out（常见命令 `0x11`）、等待、Color Mode（常见 `0x3A`）和 Display On（常见 `0x29`）。这些命令值在本模块上板前仍需核对。
5. 只实现三个测试：全屏单色、一个 10×10 矩形、一个像素窗口。确认窗口地址和 RGB565 字节序后，再调用字库函数显示 ASCII 或示例字模。
6. 用逻辑分析仪检查 CS、DC、SCK、MOSI 的相对时序；用万用表检查 VCC、BL 和共地。
7. 通过后再把绘图 API 接入比赛应用，并限制刷新率，避免阻塞 SPI 刷屏影响采样任务。

## 6. 常见现象与优先检查项

| 现象 | 先检查 |
|---|---|
| 完全白屏/黑屏 | VCC/背光、RST、DC、CS、Sleep Out/Display On、模块是否需要偏移 |
| 有颜色但花屏 | SPI mode、MSB first、RGB565 高低字节、CS 是否在整段传输中保持有效 |
| 图像上下/左右反 | `MADCTL (0x36)`、X/Y 窗口和模块实际方向 |
| 只有一条窄带或偏移 | X/Y 起始偏移、240×320 与 320×240 方向、`0x2A/0x2B` 参数 |
| 背光亮但没有像素 | BL 不是显示数据；继续检查 DC、RST、SPI 波形和初始化 |
| MSPM0 复位或异常 | 5 V 逻辑、背光电流超 GPIO 能力、供电跌落、GPIO 复用冲突 |

## 7. 原始资料索引与证据边界

- `2.4TFT_ST/2.4TFT8PIspi-GMT024-01.ino`：最完整的 7PIN 标注、软件 SPI、初始化、窗口、纯色/字模/图片例程。
- `2.4TFT_ST/2.4TFT7789V横屏显示/2.4-2.8_ST7789spiORIfastRGB(QQWW)h.c`：旧 ST7789V C 初始化和 RGB565 图片参考。
- `2.4TFT_ST/2.4TFTST7789参考代码/2.4TFT7P.C`：STM32/旧工程示例，含硬编码 PB5-PB9，不可直接用于 MSPM0。
- `2.4TFT_ST/MSPM0_ST7789/`：原资料中的空目录；正式 MSPM0 驱动现位于本知识库目录。

本目录的定位是“可审计的陌生模块整理结果”。完成精确数据手册核对、目标源码编译和真实上板测试后，再逐级把卡片状态升级。

## 8. 下一步升级条件

| 目标状态 | 必须新增的证据 |
|---|---|
| `DOC_VERIFIED` | 精确模块/控制器数据手册或原理图，核对电源、引脚、时序、初始化和偏移 |
| `COMPILE_VERIFIED` | 独立 MSPM0G3507 适配层、公共 API、当前 SDK/编译器目标源码通过编译（本轮已完成） |
| `BOARD_VERIFIED` | 实物接线、纯色/窗口/方向测试、逻辑分析仪或示波器记录 |

## 9. 比赛通用功能：ST7789 分页与局部刷新

ST7789 核心模块已经提供 `FillScreen()`、`FillRect()`、`DrawLine()` 和 `DrawRect()`；
字库模块提供 `DrawString()`、`DrawInt32()` 和 `DrawFloat()`。页面状态、翻页键和“只刷新变化区域”
仍是应用层自写。

下面状态机可以直接放入 `main.c`，矩阵键盘得到稳定字符后调用
`App_HandlePageKey()`：

```c
typedef enum {
    APP_PAGE_WAVEFORM = 0,                        /* 时域波形页。 */
    APP_PAGE_SPECTRUM,                            /* 频谱页。 */
    APP_PAGE_SETTINGS,                            /* 参数页。 */
    APP_PAGE_COUNT                                /* 页数，不是有效页。 */
} app_page_t;

static app_page_t g_page = APP_PAGE_WAVEFORM;     /* 当前页。 */
static bool g_page_dirty = true;                  /* 是否需要重画静态内容。 */

static void App_PageNext(void)
{
    g_page = (app_page_t)(((uint32_t)g_page + 1U) %
        (uint32_t)APP_PAGE_COUNT);                /* 末页后回到首页。 */
    g_page_dirty = true;                          /* 请求一次静态重画。 */
}

static void App_PagePrevious(void)
{
    if (g_page == APP_PAGE_WAVEFORM) {
        g_page = (app_page_t)(APP_PAGE_COUNT - 1U); /* 首页向前回到末页。 */
    } else {
        g_page = (app_page_t)((uint32_t)g_page - 1U); /* 普通向前翻页。 */
    }
    g_page_dirty = true;                          /* 请求一次静态重画。 */
}

static void App_HandlePageKey(char symbol)
{
    if (symbol == 'A') App_PagePrevious();        /* A：上一页。 */
    if (symbol == 'D') App_PageNext();            /* D：下一页。 */
}

static tft_st7789_status_t App_DrawPageStatic(tft_st7789_t *tft,
                                              app_page_t page)
{
    if (page >= APP_PAGE_COUNT) return TFT_ST7789_ERROR_ARGUMENT;
    if (TFT_ST7789_FillScreen(tft, TFT_ST7789_BLACK) != TFT_ST7789_OK) {
        return TFT_ST7789_ERROR_IO;               /* 翻页时整屏只清一次。 */
    }
    if (page == APP_PAGE_WAVEFORM) {
        return TFT_ST7789_DrawRect(tft, 8, 32, 224, 200, TFT_ST7789_BLUE);
    }
    if (page == APP_PAGE_SPECTRUM) {
        return TFT_ST7789_DrawRect(tft, 8, 32, 224, 200, TFT_ST7789_GREEN);
    }
    return TFT_ST7789_DrawRect(tft, 8, 32, 224, 200, TFT_ST7789_YELLOW);
}

static tft_st7789_status_t App_RenderPageDynamic(tft_st7789_t *tft)
{
    /* 只清除波形/曲线矩形；边框和静态刻度不在每帧重画。 */
    if (TFT_ST7789_FillRect(tft, 10, 34, 220, 196, TFT_ST7789_BLACK) !=
        TFT_ST7789_OK) return TFT_ST7789_ERROR_IO;
    /* 此处复制 tft_waveform 的 MapY/DrawLine 结果，画入当前页区域。 */
    return TFT_ST7789_OK;
}
```

主循环的固定顺序是：读取键盘事件并只改变 `g_page`；若 `g_page_dirty` 为真，调用
`App_DrawPageStatic()` 一次并清零标志；随后每帧只调用 `App_RenderPageDynamic()`。
因此蓝色边框、坐标轴和标题不会跟随波形闪烁。`FillScreen()` 只允许用于上电或翻页，
不允许放在连续刷新路径。

### 9.1 和 ILI9341 的差异

统一手册中的分页示例使用 `TFT_ILI9341_*` 名称；换成 ST7789 时将图形函数替换为
`TFT_ST7789_*`，将字库函数替换为 `TFT_ST7789_DrawString/DrawInt32/DrawFloat`。
两套字库使用同一份 ASCII 点阵资源，字号和显示内容可以保持一致；中文扩展仍需通过
`TFT_ST7789_DrawMonoBitmap()` 传入对应点阵。

## 10. 验证边界

本节代码只完成页面状态和刷新区域的组合逻辑；ST7789 的供电、偏移、旋转、颜色和
实际 SPI 刷新耗时仍需按照本 README 的 bring-up 顺序实板确认。当前状态保持
`BOARD NOT_RUN`。

## 11. example05 组合题补充（比赛现场复制入口）

本节是针对 `example05/signal_contest_template_final` 的补充，不改变任何模块驱动。
比赛时先看本节，再按各模块原 README 逐项复制和配置。所有复制到 `modules/` 的
`.c/.h/.inc` 均视为冻结文件；本题只允许在 `main.c` 写组合逻辑。

### 11.1 本题实际选择的模块

| 题目步骤 | 选择的模块 | 从 README 复制的文件 | 是否需要 SysConfig |
|---|---|---|---|
| 双路电压/电流同步采样 | `02_acquisition/adc_dual_sync` | `signal_dual_adc_mspm0g3507.c/.h`、`signal_status.h` | 是 |
| 1 kHz～100 kHz 激励 | `12_external_devices/dds/ad9833` | `ad9833.c/.h`、`mspm0_blocking_bus.c/.h` | 是 |
| 对数扫频表 | `06_generator/frequency_sweep` | `signal_frequency_sweep.c/.h`、`signal_status.h` | 否 |
| ST7789 屏幕 | `12_external_devices/display/st7789` | `signal_tft_st7789.c/.h`、`signal_tft_st7789_mspm0g3507.c/.h`、`signal_tft_st7789_font.c/.h`、`signal_tft_st7789_font_data.inc` | 是 |

`signal_phase.c/.h`、`signal_dual_adc_phase.c/.h` 和矩阵键盘文件是母版中已有的可选
依赖，本题没有调用它们；本题相位由下面的同步正交相关组合段完成，避免把不同
频率的 PLL 倍频接口带入阻抗测量。母版中多余文件仍保持原样，未修改、未重命名。

注意：双 ADC 驱动头文件的默认 profile 注释把 ADC1 称为 marker channel；example05
按题面把 ADC1.2/PA17 接到待测网络电压前端。这个只是应用层通道语义，必须在接线和
模拟前端资料确认后使用，不能据此修改冻结驱动。

### 11.2 SysConfig 逐项配置

在 CCS 中双击 `signal_contest_template.syscfg`，按以下顺序添加或核对；不要直接编辑
生成的 `Debug/ti_msp_dl_config.c/.h`。

1. 添加 `ADC12` 两个实例，实例名为 `SIGNAL_ADC_A`、`SIGNAL_ADC_B`，分别使用 ADC0/PA25
   和 ADC1/PA17 的 Memory 0、Input Channel 2；Trigger Source=Event、Repeat Mode=Enable，
   Memory 0 result loaded 作为 DMA trigger，打开 DMA done。
2. 给 A/B 各添加 DMA，名称 `SIGNAL_ADC_A_DMA`、`SIGNAL_ADC_B_DMA`，分别 DMA_CH0/CH1，
   Source/Destination=Half Word，Fixed-to-Block，Single。
3. 添加 `TIMER` `SIGNAL_DUAL_ADC_TIMER`（TIMG0），Periodic；ZERO_EVENT 发布到 Event 1/2，
   两个 ADC 分别订阅 1/2。
4. 不添加 DAC12、DAC DMA 或 `SIGNAL_DAC_TIMER`；AD9833 的 DDS、DAC、MCLK 均在外部模块上，
   MCU 只通过 SPI 写频率控制字。
5. 保留 `SPI_TFT`（SPI1，PB9=SCLK、PB8=MOSI、PB6=CS0，8 bit、MSB first、Mode 0）和
   `GPIO_TFT_CTRL`（PB15=`TFT_DC`、PB12=`TFT_BLK`，BLK 初始 SET）。
6. 新增 `SPI_AD9833`，硬件选 SPI0：PA12=`SCLK`、PA9=`MOSI/PICO`、Controller、8 bit、
   MSB first、MOTO3、Mode 2（CPOL=1、CPHA=0）、方向 PICO、4 MHz；AD9833 不返回数据，
   不接 MISO 和硬件 CS。再新增 GPIO 组 `GPIO_AD9833`，输出 `AD9833_FSYNC`，初始 `SET`，
   本工程取 PA8（BoosterPack 排针 4）；接到 AD9833 的 FSYNC。AD9833 模块的 MCLK 来自其板载/外部晶振，不接 MCU。

与原双 ADC README 的差异只有采样速度：原示例常用 10 us（100 kSPS），本题把
`SIGNAL_DUAL_ADC_TIMER` 设为 2 us（500 kSPS），以便 100 kHz 仍有约 5 点/周期。AD9833
要求 Mode 2（CPOL=1、CPHA=0），故将其独立的 `SPI_AD9833` 固定为 Mode 2；ST7789 的
`SPI_TFT` 保持 Mode 0。两块模块不共用 SCLK/MOSI，也不需要运行时切换 SPI 模式。保存后
Generate，只核对生成头文件中的 `SPI_TFT_INST`、`SPI_AD9833_INST`、`GPIO_AD9833_*`、`*_DMA_CHAN_ID` 和
`*_LOAD_VALUE` 宏。

### 11.3 从 README 复制到 main.c 的固定代码

以下代码形状直接来自模块 README，比赛时按顺序粘贴；变量名只做本题通道语义适配。

```c
const signal_dual_adc_config_t adc_config = {
    SIGNAL_SAMPLE_RATE_HZ, CPUCLK_FREQ, 65536U
};
SYSCFG_DL_init();
SignalDualADC_Init(&adc_config);
SignalDualADC_Start(g_raw_current, g_raw_voltage,
                    SIGNAL_SAMPLE_COUNT);
while (!SignalDualADC_IsFinished()) { __NOP(); }
```

AD9833 的初始化、扫频设频和恢复 ST7789 SPI 的调用形状如下：

```c
DL_GPIO_setPins(GPIO_AD9833_PORT, GPIO_AD9833_AD9833_FSYNC_PIN);
AD9833_Init(SPI_AD9833_INST, GPIO_AD9833_PORT, GPIO_AD9833_AD9833_FSYNC_PIN);
SignalFrequencySweep_Generate(&sweep_config, g_frequency_hz, 32U);
AD9833_SetOutput(SPI_AD9833_INST, GPIO_AD9833_PORT,
    GPIO_AD9833_AD9833_FSYNC_PIN, SIGNAL_AD9833_MCLK_HZ,
    (uint32_t)frequency_hz, 0U, AD9833_WAVE_SINE);
```

`AD9833_SetOutput()` 返回 `false` 时显示 `AD9833 ERROR` 并跳过该点。输入频率必须是整数
Hz，因此应用将对数表的 float 频点按四舍五入转换为 `uint32_t` 后，回写到测量频率数组。
AD9833 无实际频率回读 API；其 28 bit FTW 量化误差远小于本题扫频步进，应用以该整数请求值
作为相位相关、拟合和显示的频率。`SIGNAL_AD9833_MCLK_HZ` 必须改成实物模块 MCLK，25 MHz
只是常见模块的默认值。

ST7789 的初始化和 8×16 字库调用同样来自 README：

```c
SignalTFTST7789_MSPM0_Init(&g_tft, TFT_ST7789_ROTATION_270, 0U, 0U);
TFT_ST7789_DrawString(&g_tft, 4, 2, "IMPEDANCE METER",
    TFT_ST7789_FONT_8X16, TFT_ST7789_CYAN, TFT_ST7789_BLACK, false, false);
TFT_ST7789_DrawFloat(&g_tft, 76, 42, voltage_rms, 3U,
    TFT_ST7789_FONT_8X16, TFT_ST7789_YELLOW, TFT_ST7789_BLACK, false);
```

### 11.4 先补 README、再复制的题目组合段

原模块 README 只覆盖“采样、输出、显示”最小闭环，没有覆盖阻抗公式，所以本节补充
以下应用代码后，`main.c` 再逐段复制：

1. 去直流后 RMS：`Vrms=sqrt(sum(v²)/N)`、`Irms=sqrt(sum(i²)/N)`。
2. 阻抗：`|Z|=Vrms/Irms`，`Zre=|Z|cos(phi)`，`Zim=|Z|sin(phi)`。
3. 相位：对同一频率的电压、电流分别做正交相关，取 `atan2(-imag,real)` 后相减并环绕到
   `[-180,180]`。这段只使用已完成 DMA 的数组，不进入 ADC 中断。
4. 串联模型拟合：对有效点拟合 `Zre=R`、`Zim=wL-1/(wC)`；相位正负用于 R/L/C 判断。
5. 谐振与 Q：`f0=1/(2π√(LC))`（无拟合 L/C 时取最小 `|Z|` 点），在
   `|Z|=√2R` 两侧线性插值求 `BW=fH-fL`，`Q=f0/BW`。

相位和一帧 RMS/阻抗的最小组合代码如下；这是本题先补到 README、再粘贴到
`main.c` 的应用层代码，不属于任何冻结模块：

```c
static float App_PhaseDegrees(const uint16_t *raw, float f, float fs, float mean)
{
    float real_part = 0.0f, imag_part = 0.0f;
    float step = 6.28318530718f * f / fs;
    for (uint32_t i = 0U; i < SIGNAL_SAMPLE_COUNT; ++i) {
        float x = (float)raw[i] - mean;
        float angle = step * (float)i;
        real_part += x * cosf(angle);
        imag_part += x * sinf(angle);
    }
    return atan2f(-imag_part, real_part) * 57.2957795131f;
}

/* DMA 完成后执行，不在 ADC/DMA ISR 中执行浮点运算。 */
vrms = sqrtf(sum_v2 / (float)SIGNAL_SAMPLE_COUNT);
irms = sqrtf(sum_i2 / (float)SIGNAL_SAMPLE_COUNT);
z_abs = (irms > 1.0e-9f) ? vrms / irms : 0.0f;
z_re = z_abs * cosf(phi * 0.01745329252f);
z_im = z_abs * sinf(phi * 0.01745329252f);
```

这些是题目要求的少量组合逻辑，不修改任何模块文件；`main.c` 中对应函数为
`App_PhaseDegrees`、`App_MeasureFrame`、`App_FitSeriesModel`、`App_DrawAnalysis`。
显示全部使用 `TFT_ST7789_FONT_8X16`，未引入其他字库。

### 11.5 逐行复制顺序

1. 先复制 include、静态 ADC 缓冲区和 TFT 对象。
2. 复制 `App_Text`、`App_LabelFloat`、`App_DrawStaticUi`，先让屏幕显示固定标题。
3. 复制 ADC `Start -> IsFinished` 闭环，再加入均值/RMS/阻抗换算；先只显示一帧。
4. 复制 AD9833 README 的 `Init`、`SetOutput` 和 `SignalFrequencySweep_Generate`，再复制
   `App_RunSweepPoint`，确认屏幕上的频率从
   1.00 kHz 逐点到 100.00 kHz。
5. 复制拟合、谐振、带宽、Q 和 `App_DrawAnalysis`，扫频结束后显示最终结果。
6. 最后复制 SysTick 保持时间和自动重扫逻辑；主循环不在中断内做浮点计算或 SPI 刷屏。

任何一步 Build 失败，先回到该模块 README 检查实例名、include 和返回码，不改模块 `.c/.h`。
