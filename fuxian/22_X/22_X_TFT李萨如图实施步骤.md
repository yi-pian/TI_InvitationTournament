# 22_X TFT 李萨如图实施步骤

## 1. 本次模块选择

在上一轮双 ADC 同步采集基础上，显示设备选择 `MSPM0_Signal_Contest/01_bsp/tft_ili9341/` 的 `tft_ili9341` 模块。它提供 MSPM0G3507 的 SPI 平台绑定、ILI9341 基础图元和内置字库。

李萨如图不是单路波形，因此没有选择 `tft_waveform` 辅助模块；本题只需要 ILI9341 的 `FillRect`、`DrawRect`、`DrawLine` 和 `DrawString`。X 轴来自上一轮 ADC A，Y 轴来自 ADC B。刷新采用局部刷新：标题和蓝色边框只画一次，循环中只清除边框内侧并重画李萨如轨迹。

## 2. 按 README 复制的模块文件

从 `01_bsp/tft_ili9341/` 复制到 `signal_contest_template_final/modules/`：

| 文件 | 说明 |
|---|---|
| `signal_tft_ili9341.c` | ILI9341 通用驱动 |
| `signal_tft_ili9341.h` | ILI9341 API |
| `signal_tft_ili9341_font_data.inc` | 内置 ASCII 字库数据 |
| `signal_tft_ili9341_mspm0g3507.c` | MSPM0G3507 SPI/GPIO 平台绑定 |
| `signal_tft_ili9341_mspm0g3507.h` | 平台初始化入口 |

公共文件 `signal_status.h` 沿用上一轮已复制的文件，不重复修改。所有模块 `.c/.h` 和字库 `.inc` 均保持原样。

## 3. SysConfig 配置

在已有双 ADC、双 DMA、TIMG0 配置上，继续按 ILI9341 README 第 3.2 节增加：

| 外设 | 实例/资源 | 配置 |
|---|---|---|
| SPI | `SPI_TFT` / SPI1 | Controller、8-bit、ILI9341 常用 Mode 0，SCLK=PB9、MOSI=PB8、CS0=PB6 |
| GPIO | `GPIO_TFT_CTRL` | `TFT_DC=PB15` 输出；`TFT_BLK=PB12` 输出且初始值 SET |

本题不配置 MISO/POCI；驱动为只写显示链路。SPI 时钟必须以 SysConfig GUI 的实际 SCLK 计算值为准，不能把 `CPUCLK_FREQ` 当成 SPI SCLK。RESET 按 README 默认由屏模块硬件处理，平台入口使用 `set_reset=NULL`。

与 README 的功能配置无差异：README 的 SPI1/PB9/PB8/PB6、PB15/PB12 是已验证基线，本工程沿用该基线；上一轮 ADC 仍保持 X=`ADC0.2/PA25`、Y=`ADC1.2/PA17`、TIMG0 同步触发。首次 CLI 校验发现 ADC 和 SPI 的自动 PinConfig 内部 `$name` 重复，因此在 `.syscfg` 中为 ADC A/B 和 SPI SCLK/MOSI/MISO/CS0 显式分配了唯一的 `GPIOPinGeneric0`～`GPIOPinGeneric5` 名称；这只解决 SysConfig 对象命名冲突，不改变任何引脚或外设功能。

保存 `.syscfg` 后 Generate，并检查生成头文件中存在：`SPI_TFT_INST`、SPI SCLK/MOSI/CS 宏、`GPIO_TFT_CTRL_PORT`、`GPIO_TFT_CTRL_TFT_DC_PIN`、`GPIO_TFT_CTRL_TFT_BLK_PIN`，以及上一轮 ADC 的所有宏。

## 4. main.c 复制与组合

TFT 初始化代码直接来自 README 的第 5、7、8 节和 `README_MINIMAL_EXAMPLE.c`：

1. 包含 `signal_tft_ili9341.h` 与 `signal_tft_ili9341_mspm0g3507.h`。
2. 声明 `g_tft`、TFT 状态变量。
3. 在 `SYSCFG_DL_init()` 后调用 `SignalTFTILI9341_MSPM0_Init(&g_tft, TFT_ILI9341_ROTATION_270)`；当前实物接线方向使用该横屏方向，若屏幕方向相反，只改这个旋转参数。
4. 调用 `TFT_ILI9341_FillScreen()` 清黑屏。
5. 调用 `TFT_ILI9341_DrawString()` 显示 `LISSAJOUS`。

变量名 `g_status` 在组合工程中拆为 `g_adc_status` 和 `g_tft_status`，仅为避免双 ADC 与 TFT 两种返回码变量同名。

## 5. README 补全的李萨如代码

原 ILI9341 README 没有 X/Y 双通道李萨如示例，因此已在其 `8.1 双 ADC 李萨如图` 增补应用代码。该段代码未修改任何模块驱动，只完成：

1. 将 12 位 ADC A 原始码线性映射到绘图区 X 坐标。
2. 将 12 位 ADC B 原始码线性映射到绘图区 Y 坐标，并翻转屏幕 Y 方向。
3. 初始化时清除 `280×180` 绘图区并画一次蓝色边框。
4. 每帧只清除 `278×178` 的边框内侧，不重复绘制蓝色边框。
5. 将 1024 点等比例抽取为 280 个屏幕点，逐相邻点调用 `TFT_ILI9341_DrawLine()`；坐标限制在边框内侧，避免黄色轨迹覆盖蓝框。

`main.c` 中的 `Lissajous_MapX()`、`Lissajous_MapY()`、`Lissajous_DrawStaticFrame()`、`Lissajous_DrawFrame()` 即从该 README 补全段复制，仅做了与上一轮变量名和工程缓冲区的组合适配。

## 6. 自己编写的少量逻辑

除 README 中补全的李萨如应用段外，只写了以下组合逻辑：

- 将 TFT 初始化插入上一轮 ADC 初始化之后；
- TFT 初始化后调用一次 `Lissajous_DrawStaticFrame()`；每次双 ADC DMA 完成后只调用 `Lissajous_DrawFrame()` 刷新轨迹；
- 使用 `g_adc_status` 与 `g_tft_status` 分离两类状态。

没有修改任何 TFT 或 ADC 模块 `.c/.h`，没有在 ADC/DMA 中断中执行 TFT/SPI 绘图。

## 7. 比赛现场操作顺序

1. 先按上一轮流程确认双 ADC 已能采集 X/Y。
2. 接 TFT：3.3 V、GND、SCK、MOSI、CS、DC、BLK；RESET 按屏模块要求处理。
3. 在 CCS 打开 `.syscfg`，添加/核对 `SPI_TFT` 和 `GPIO_TFT_CTRL`，确认 PinMux 无冲突。
4. Generate，核对生成宏，不手改生成的 `ti_msp_dl_config.c/.h`。
5. Refresh 工程，确认 5 个 TFT 文件已加入 Build，尤其不要漏掉 `.inc` 字库文件。
6. Build。上板后先确认黑底 `LISSAJOUS` 能显示，再接入 X/Y 信号观察图形。
7. 若白屏/花屏，按 TFT README 顺序检查供电、共地、CS/DC/RESET、SPI Mode 和 SCLK，再降低 SPI 时钟。

## 8. 本次验证记录

- ILI9341 5 个模块文件已从 `01_bsp/tft_ili9341/` 原样复制；SHA-256 与来源文件逐一一致。
- SysConfig CLI（MSPM0 SDK 2.11.00.07、SysConfig 1.28.0）隔离生成通过：`0 error`、`0 warning`；生成结果包含双 ADC、DMA、TIMG0、`SPI_TFT_INST` 和 `GPIO_TFT_CTRL_*` 宏。工具提示的 ADC 低功耗说明、DMA Full Channel 和 SPI STOP/STANDBY 保留提示属于信息项。
- TI ARM Clang 使用 `-Wall -Wextra -Werror -fsyntax-only` 检查 `main.c`、双 ADC 模块、ILI9341 通用驱动、MSPM0 平台绑定和生成的 `ti_msp_dl_config.c`，全部通过。
- 工程 `gmake` 首次使用旧的自动生成 source list，链接提示缺少 TFT 符号；按比赛步骤 Refresh 工程、确认两个 TFT `.c` 纳入 Build 后，用同一 TI ARM Clang 参数完成两个 TFT 对象编译和完整链接，`signal_contest_template_final.out` 生成成功。
- 尚未连接真实开发板和 TFT 做上电、波形输入、刷新率或李萨如形状实测；板卡状态仍为 `NOT_RUN`。

## 9. 本次局部刷新修改

- 原实现每帧清除整个 `280×180` 区域并重新调用 `DrawRect`，因此蓝色边框会随轨迹一起刷新，产生闪烁感。
- 现在 `Lissajous_DrawStaticFrame()` 只在 TFT 初始化阶段执行一次；`Lissajous_DrawFrame()` 每帧只清除 `278×178` 内框区域，再绘制黄色轨迹。
- X/Y 映射范围改为内框坐标，轨迹不会覆盖蓝色边框；标题、整屏黑底和蓝框均不再进入采集循环。
- 本次只修改应用层 `main.c`、TFT README 和步骤文档，ILI9341/ADC 模块 `.c/.h` 未修改。

## 10. 非直接复制代码逐行解释

本题原始 ILI9341 README 没有李萨如图和局部刷新代码。这部分应用逻辑先补充到 README 的 `8.1`，再复制到 `main.c`；它不属于 ILI9341 或 ADC 模块驱动。下面按当前 [`main.c`](signal_contest_template_final/main.c) 的行号解释。单个 C 表达式换行书写时，相邻多行共同构成一条语句。

### 10.1 绘图区常量（main.c 第 20～29 行）

| 行号 | 代码 | 作用 |
|---:|---|---|
| 20 | `LISSAJOUS_PLOT_X = 20` | 蓝色边框左边缘的 x 坐标。屏幕左上角是 `(0, 0)`，因此左侧留 20 像素空白。 |
| 21 | `LISSAJOUS_PLOT_Y = 30` | 蓝色边框上边缘的 y 坐标；标题在 y=8，高度为 16 像素，所以图框从 y=30 开始不会压住标题。 |
| 22 | `LISSAJOUS_PLOT_WIDTH = 280` | 蓝框总宽度为 280 像素，边框 x 覆盖 20～299。 |
| 23 | `LISSAJOUS_PLOT_HEIGHT = 180` | 蓝框总高度为 180 像素，边框 y 覆盖 30～209。 |
| 24 | `LISSAJOUS_POINT_COUNT = 280U` | 一帧从 1024 个采样点中等比例取 280 个点；相邻点连线共 279 条，降低 SPI 绘图量。`U` 表示无符号整数。 |
| 25 | `ADC12_FULL_SCALE = 4095U` | 12 位 ADC 的最大原始码是 `2^12 - 1 = 4095`，用于把 ADC 原始码缩放成像素坐标。 |
| 26 | `LISSAJOUS_INNER_X = PLOT_X + 1` | 内框左边界为 x=21，避开 x=20 的蓝色边线。 |
| 27 | `LISSAJOUS_INNER_Y = PLOT_Y + 1` | 内框上边界为 y=31，避开 y=30 的蓝色边线。 |
| 28 | `LISSAJOUS_INNER_WIDTH = PLOT_WIDTH - 2` | 内框宽 278 像素；减去左右各 1 像素蓝色边线。 |
| 29 | `LISSAJOUS_INNER_HEIGHT = PLOT_HEIGHT - 2` | 内框高 178 像素；减去上下各 1 像素蓝色边线。 |

### 10.2 本题会用到的对象和状态变量（main.c 第 31～35 行）

| 行号 | 代码 | 作用 |
|---:|---|---|
| 31 | `static uint16_t g_raw_a[...]` | 上一轮双 ADC 模块经 DMA 写入的 A 路采样缓冲区；本题将它作为李萨如图的 X 数据。`uint16_t` 足够保存 12 位 ADC 原始码。 |
| 32 | `static uint16_t g_raw_b[...]` | B 路 DMA 采样缓冲区；本题将它作为李萨如图的 Y 数据。相同下标的 A/B 样本由同一次 Timer 触发取得。 |
| 33 | `static tft_ili9341_t g_tft` | ILI9341 屏幕对象，保存 SPI/GPIO 回调、旋转方向、宽高和初始化状态；所有绘图 API 的第一个参数都是 `&g_tft`。 |
| 34 | `volatile signal_result_t g_adc_status` | 保存双 ADC 模块调用结果，用来判断初始化、启动采样是否成功。`volatile` 沿用模块示例的状态变量写法。 |
| 35 | `volatile tft_ili9341_status_t g_tft_status` | 保存 TFT API 返回值；不是像素数据，而是 `OK`、参数错误或通信错误等状态。 |

`static` 表示这些对象只在 `main.c` 内可见，避免与其他 `.c` 文件同名；并不表示“数值不变”。

### 10.3 X 坐标映射（main.c 第 37～42 行）

| 行号 | 代码 | 作用 |
|---:|---|---|
| 37 | `Lissajous_MapX(uint16_t sample)` | 定义函数：输入一个 A 路 ADC 原始码 `sample`，返回对应的屏幕 x 坐标。返回类型为 `int32_t`，匹配绘图 API 的有符号坐标参数。 |
| 39 | `return LISSAJOUS_INNER_X +` | 先从内框左边界 x=21 开始，保证轨迹不画到蓝框。 |
| 40 | `(uint32_t)sample * (INNER_WIDTH - 1)` | 将 ADC 原始码乘以可用横向跨度 277。先转换为 `uint32_t`，避免 16 位乘法溢出。 |
| 40～41 | `/ ADC12_FULL_SCALE` | 再除以 4095，完成线性缩放：输入 0 得到 x=21；输入 4095 得到 x=298。 |
| 42 | `}` | X 映射函数结束。 |

### 10.4 Y 坐标映射（main.c 第 44～49 行）

| 行号 | 代码 | 作用 |
|---:|---|---|
| 44 | `Lissajous_MapY(uint16_t sample)` | 定义函数：输入一个 B 路 ADC 原始码，返回对应的屏幕 y 坐标。 |
| 46 | `INNER_Y + INNER_HEIGHT - 1 -` | 屏幕 y 轴向下增加，因此先取内框底部 y=208，再减去缩放值，使 ADC 码越大，显示位置越靠上。 |
| 47～48 | `sample * (INNER_HEIGHT - 1) / 4095` | 将 0～4095 缩放为 0～177 的纵向位移。输入 0 得 y=208；输入 4095 得 y=31。 |
| 49 | `}` | Y 映射函数结束。 |

### 10.5 一次性静态边框（main.c 第 51～65 行）

| 行号 | 代码 | 作用 |
|---:|---|---|
| 51 | `Lissajous_DrawStaticFrame(void)` | 定义只在上电初始化后调用一次的函数；`void` 表示没有输入参数。 |
| 53 | `tft_ili9341_status_t status` | 函数内部临时变量，用于接收每次 TFT 操作的结果。 |
| 55～58 | `TFT_ILI9341_FillRect(... PLOT_X, PLOT_Y, PLOT_WIDTH, PLOT_HEIGHT, BLACK)` | 把整个图框区域清为黑色，为第一次显示准备干净背景；该操作只执行一次。 |
| 59 | `if (status != ...OK) return status` | 如果清除失败，立即把错误返回给 `main`，不再继续绘制。 |
| 61～64 | `TFT_ILI9341_DrawRect(..., BLUE)` | 在刚清除的区域外沿画蓝色空心矩形，即静态边框。 |
| 65 | `}` | 静态边框函数结束；`DrawRect` 的返回值直接作为函数返回值。 |

### 10.6 每帧局部刷新和轨迹绘制（main.c 第 67～93 行）

| 行号 | 代码 | 作用 |
|---:|---|---|
| 67 | `Lissajous_DrawFrame(void)` | 定义每次双 ADC DMA 采集完成后调用的动态绘制函数。 |
| 69 | `uint16_t point` | 280 点循环计数器；16 位足够表示 0～279。 |
| 70 | `uint32_t index0` | 当前线段起点在 1024 点 ADC 缓冲区中的下标。 |
| 71 | `uint32_t index1` | 当前线段终点在 1024 点 ADC 缓冲区中的下标。 |
| 72 | `status` | 保存局部清除和画线 API 的返回状态。 |
| 74～77 | `FillRect(... INNER_X, INNER_Y, INNER_WIDTH, INNER_HEIGHT, BLACK)` | 只擦除 278×178 的内框旧黄色轨迹；不擦标题和蓝色边框，因此静态内容不刷新。 |
| 78 | 错误判断 | 内框擦除失败时立刻返回错误。 |
| 80 | `for (point = 0; point + 1 < POINT_COUNT; ++point)` | `point` 从 0 到 278，共画 279 条线；`point + 1` 始终不越过最后一个显示点 279。 |
| 81～82 | `index0 = point * 1023 / 279` | 将当前显示点 `point` 等比例映射到 ADC 数组下标；第一个点为 0，最后一个起点接近 1019。 |
| 83～84 | `index1 = (point + 1) * 1023 / 279` | 计算下一显示点的 ADC 数组下标；最后一条线的终点恰好是 1023。 |
| 85 | `TFT_ILI9341_DrawLine(` | 开始画一条黄色线段。 |
| 86 | `MapX(g_raw_a[index0])` | 线段起点 x：读取 A 路起点样本并映射到屏幕横坐标。 |
| 87 | `MapY(g_raw_b[index0])` | 线段起点 y：读取同一时刻 B 路样本并映射到屏幕纵坐标。 |
| 88 | `MapX(g_raw_a[index1])` | 线段终点 x：读取下一时刻 A 路样本。 |
| 89 | `MapY(g_raw_b[index1]), YELLOW` | 线段终点 y：读取下一时刻 B 路样本，颜色固定为黄色。A/B 配对形成 `(X, Y)` 平面轨迹，而不是两条随时间变化的曲线。 |
| 90 | 错误判断 | 任意一条线发送失败就停止本帧并返回错误。 |
| 91 | `}` | 一帧 279 条线全部完成，结束 `for` 循环。 |
| 92 | `return TFT_ILI9341_OK` | 所有局部清除和画线均成功，向 `main` 报告成功。 |
| 93 | `}` | 动态绘制函数结束。 |

### 10.7 与复制代码组合的两处调用（main.c 第 113～141 行）

| 行号 | 代码 | 作用 |
|---:|---|---|
| 113～121 | TFT 初始化、黑屏、`DrawString("LISSAJOUS")` | 这部分来自 TFT README；`ROTATION_270` 是当前工程实际使用的横屏方向。标题只在初始化阶段发送一次。 |
| 123～125 | `g_tft_status = Lissajous_DrawStaticFrame()` | 本题组合代码：在标题画好后立刻画一次蓝框。条件判断保证前一个 TFT 操作成功才继续。 |
| 126 | `if (g_tft_status != ...OK) while (1) {}` | 沿用 README 的失败停机写法；避免初始化失败时继续使用未准备好的屏幕。 |
| 130～135 | `SignalDualADC_Start`、等待 `SignalDualADC_IsFinished` | 来自上一轮双 ADC README：DMA 完成前不能读取 `g_raw_a/g_raw_b`。 |
| 140 | `g_tft_status = Lissajous_DrawFrame()` | 本题组合代码：仅在两路 DMA 都完成后调用动态绘图函数。 |
| 141 | 错误判断 | 若内框刷新或画线出错，停止循环，避免继续输出不可信显示。 |

因此，本题每一帧执行的实际顺序是：`采集 1024 对 A/B 样本` → `等待 DMA 完成` → `擦除内框旧轨迹` → `从 1024 对样本抽取 280 对` → `画 279 条黄色线`。蓝框、标题和整屏背景不在这个循环里。
