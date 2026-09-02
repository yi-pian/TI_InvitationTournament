# moni02 内置 DAC + DDS：直接数字输入版复制教程

## 1. 最终操作方式

本工程不是按键逐步加减。操作流程是：

```text
A 选择波形 → B 选择参数 → 数字键直接输入 → # 确认 → 更新 DAC
```

例子：

| 目的 | 按键顺序 |
|---|---|
| 输出 5000 Hz | 选中 `FREQ`，按 `5 0 0 0 #` |
| 输出 1.5 Vpp | 选中 `VPP`，按 `1 * 5 #` |
| 方波占空比 25% | 选中 `DUTY`，按 `2 5 #` |
| 锯齿波对称度 60% | 选中 `SYMM`，按 `6 0 #` |

`*` 在输入过程中表示小数点，不是减号。所有参数均为非负数，因此不需要负号。

## 2. 全部按键定义

键盘字符排列来自 `fuyong/70_keypad_usage`：

```text
1  2  3  A
4  5  6  B
7  8  9  C
*  0  #  D
```

空闲状态：

| 按键 | 功能 |
|---|---|
| `A` | `SINE → SQUARE → SAW → SINE` |
| `B` | 选择待输入参数 |
| `0`～`9` | 清除上一次输入文本，开始一轮新输入 |
| `*` | 以 `0.` 开始一轮小数输入 |
| `C` | 恢复全部默认值 |

正在输入时：

| 按键 | 功能 |
|---|---|
| `0`～`9` | 追加数字 |
| `*` | 追加一个小数点；第二个小数点会被拒绝 |
| `D` | 删除最后一位 |
| `C` | 取消本次输入，原输出参数不变 |
| `#` | 确认、检查范围，通过后更新 DAC |
| `A`、`B` | 输入未确认时不切换，防止数值写错参数 |

长按仍只产生一次“新按下”事件，必须松开再按。这是矩阵键盘复用模块的消抖设计。

## 3. 参数单位与范围

输入时直接使用屏幕标注的单位：

| 编辑项 | 输入单位 | 合法范围 | 默认值 |
|---|---|---:|---:|
| `FREQ` | Hz | 100～10000 | 1000 |
| `VPP` | V | 0.2～3.0 | 1.0 |
| `DUTY` | % | 5～95 | 50 |
| `SYMM` | % | 5～100 | 100 |

方波占空比输入 `25`，主程序确认后才换算成复用接口使用的 `0.25f`。锯齿波对称度同理。这样比赛输入时不用自己心算 0～1 比例。

输入超出范围时：

1. 屏幕 `INPUT` 行显示红色 `RANGE ERROR`；
2. 原参数和原波形保持不变；
3. 直接按下一个数字即可开始重新输入。

## 4. 屏幕显示

- `WAVE`：当前波形；
- `F SET`：确认过的请求频率；
- `F OUT`：DMA 整周期量化后的实际输出频率；
- `VPP`：请求峰峰值；
- `DUTY/SYMM`：百分数；
- `INPUT`：当前正在输入的文本、`READY` 或 `RANGE ERROR`；
- `EDIT`：数字确认后会写入的目标参数。

数字输入期间只刷新 `INPUT` 的字符框，不重新生成 DMA 波形；只有按 `#` 且范围正确后才调用 `ApplyWaveform()`。固定标签和其他未变化数值不会重画。

## 5. 数据链和职责边界

```text
signal_matrix_keypad_4x4
        │ 只产生一次新按键字符
        ▼
signal_keypad_number_input
        │ 保存文本、处理小数点/删除/取消/#确认、解析 float
        ▼
moni02 小逻辑
        │ 决定当前单位，检查 Hz/V/% 范围
        ▼
SignalWaveOutput_Start
        │ 波表 → DDS → 整周期 DMA 缓冲
        ▼
SIGNAL_DAC_TIMER → DMA → DAC0 PA15

同一参数状态 → ST7789 屏幕
```

关键点是：键盘扫描、数字字符串解析、DDS、DAC DMA、波表和 TFT 都在 `fuyong` 模块里。`moni02` 自己只决定“这个数字现在代表 Hz、V 还是 %”。

## 6. 复制来源

### 6.1 DDS/DAC 与波形

来源：`fuxian/fuyong/90_dds_usage/modules/`

```text
signal_status.h
signal_dac_dma_mspm0g3507.c/.h
signal_dac_wave_table.c/.h
signal_dds.c/.h
signal_sine.c/.h
signal_square.c/.h
signal_triangle.c/.h
signal_sawtooth.c/.h
signal_arbitrary_wave.c/.h
signal_wave_output_mspm0g3507.c/.h
signal_math.h
```

本次先在复用库增加了统一接口：

```c
SignalWaveOutput_Start(type, frequency_hz, vpp_v,
                       offset_v, shape_fraction);
```

旧的正弦、方波、锯齿波包装函数仍保留，旧工程不受影响。

### 6.2 矩阵键盘扫描

来源：`fuxian/fuyong/70_keypad_usage/modules/`

```text
signal_matrix_keypad_4x4.c
signal_matrix_keypad_4x4.h
```

`main.c` 的 `ReadKeypad()` 复制自 `70_keypad_usage` 的 `KEY_READ`。

### 6.3 通用直接数字输入

原来的 `70_keypad_usage` 只能保存数字字符，不能真正完成小数、确认和解析。本次先在 `fuyong/70_keypad_usage/modules/` 新增：

```text
signal_keypad_number_input.c
signal_keypad_number_input.h
```

然后原样复制到 `moni02/modules/`，目标副本没有单独修改。公共接口为：

```c
SignalKeypadNumberInput_Init(...);
SignalKeypadNumberInput_HandleKey(...);
SignalKeypadNumberInput_GetValue(...);
SignalKeypadNumberInput_GetText(...);
SignalKeypadNumberInput_IsActive(...);
```

这个模块不依赖 MSPM0 引脚，以后串口收到字符或别的键盘产生同样字符，也可以复用。

### 6.4 TFT

来源：`fuxian/fuyong/80_tft_usage/modules/`

```text
signal_tft_st7789.c/.h
signal_tft_st7789_mspm0g3507.c/.h
signal_tft_st7789_font.c/.h
signal_tft_st7789_font_data.inc
```

### 6.5 SysConfig

`signal_contest_template.syscfg` 整文件复制自 `fuyong/90_dds_usage`，没有手改生成的 `Debug/ti_msp_dl_config.c/.h`。

关键实例和引脚：

| 功能 | 实例/引脚 |
|---|---|
| DAC 输出 | DAC0，PA15 |
| DAC DMA | `SIGNAL_DAC_DMA`，DMA_CH1 |
| DAC 定时器 | `SIGNAL_DAC_TIMER`，TIMG6 |
| TFT SPI | SCLK PB9、MOSI PB8、CS PB6 |
| TFT 控制 | DC PB15、BLK PB12 |
| 键盘行 | PB16、PB0、PB7、PB17 |
| 键盘列 | PB18、PB13、PB20、PB4，内部上拉 |

注意 DAC 是 **PA15**，TFT DC 是 **PB15**，端口字母不同。

## 7. 比赛时从空工程复制

### 第一步：复制 SysConfig 源文件

复制：

```text
fuyong/90_dds_usage/signal_contest_template.syscfg
```

不要复制 `Debug` 里的生成文件。

### 第二步：复制模块

依次复制第 6 节列出的 DDS/DAC、键盘扫描、数字输入、TFT 文件到新工程 `modules/`。

### 第三步：复制集中参数

复制 `signal_config.h`。题目改变时只改这些宏：

```c
MONI02_FREQUENCY_MIN_HZ
MONI02_FREQUENCY_MAX_HZ
MONI02_VPP_MIN_V
MONI02_VPP_MAX_V
MONI02_SQUARE_DUTY_MIN/MAX
MONI02_SAW_SYMMETRY_MIN/MAX
```

最低频率还决定 DMA 缓冲容量：

```text
最少点数 = DAC 更新率 / 最低频率
100000 / 100 = 1000 点
```

因此本工程使用 1024 点。照搬 512 点示例会导致 100 Hz 返回越界。

### 第四步：复制 main 的模块接线区

按注释标记复制：

1. `[复制改参：DDS_INIT]`；
2. `[复制：KEY_READ]`；
3. `[复制：TFT_INIT]`；
4. `[复制改参：TFT_STATIC_TEXT]`；
5. 数字输入对象 `g_number_input`；
6. 最后复制 `main()` 的初始化和 5 ms 键盘轮询框架。

`MONI02_PARAMETER_LOGIC` 是题目胶水，只需根据新题目的单位和范围修改 `CommitConfirmedNumber()`。

### 第五步：让 CCS 纳入新文件

1. 右键工程 → **Refresh**；
2. 展开 `modules`，确认所有 `.c` 都出现；
3. 确认没有 **Exclude from Build**；
4. **Project → Clean**；
5. **Build Project**。

链接出现 `undefined symbol SignalKeypadNumberInput_*` 时，优先检查新 `.c` 是否纳入构建，不要把 `.c` 直接 `#include` 进 `main.c`。

## 8. 自己写的代码逐行解释

行号对应当前 `main.c`；以后若注释移动，以 `[自己写 START/END]` 为准。

### 8.1 `ApplyWaveform()`：第 139～158 行

| 行段 | 解释 |
|---:|---|
| 141 | 先给形状参数 0.5；正弦会忽略它。 |
| 143～147 | 方波选择独立占空比，锯齿波选择独立对称度。 |
| 149～151 | 把波形、Hz、Vpp、1.65 V 偏置和形状一次交给复用接口。 |
| 152～154 | 模块错误时返回，错误码保留在 `g_contest_status`。 |
| 156～157 | 读取实际频率，供 `F OUT` 显示。 |

### 8.2 文字映射：第 241～261 行

- `GetWaveName()` 把枚举映射成 `SINE/SQUARE/SAW`；
- `GetEditName()` 把编辑枚举映射成 `FREQ/VPP/DUTY/SYMM`；
- 这里只决定屏幕文字，不控制硬件。

### 8.3 固定界面与局部刷新：第 189～235、269～386 行

| 行段 | 解释 |
|---:|---|
| 189～235 | `DrawStaticText()` 上电清屏一次，并一次性画标题、帮助、标签和单位。动态循环不再调用 `FillScreen()`。 |
| 269～273 | `ClearText()` 直接参考 `moni01/App_ClearText()`，只清 `字符数×8×16` 的小矩形。 |
| 275～310 | 波形、设置频率、实际频率和 Vpp 各有独立字段函数。 |
| 311～340 | 形状标签会在 `SHAPE/DUTY/SYMM` 间变化，所以只清形状这一行的标签框和值框。 |
| 343～361 | 输入数字时只清并重画 `INPUT` 的 128×16 像素框。 |
| 364～370 | 编辑项只清并重画自己的字符框。 |
| 373～386 | `DrawDirtyFields()` 检查位掩码，只调用被标脏的字段函数。 |

原先导致整页闪烁的 `FillRect(&g_tft, 0, 28, 320, 168, ...)` 已删除。现在只有上电时整屏清一次。

### 8.4 `IsValueInRange()`：第 394～397 行

只判断 `minimum <= value <= maximum`。它不偷偷把错误值裁剪到边界，因为直接输入时，用户输入 50000 Hz 应明确看到错误，而不是悄悄输出 10000 Hz。

### 8.5 波形和编辑项：第 399～421 行

- `SelectNextWave()` 实现三种波形循环；
- 从锯齿波回正弦时，如果正在编辑形状，就回到频率，因为正弦没有形状参数；
- `SelectNextParameter()` 对正弦只循环两项，对方波/锯齿波循环三项。

### 8.6 `CommitConfirmedNumber()`：第 427～475 行

这是本题最重要的少量逻辑：

| 行段 | 解释 |
|---:|---|
| 429 | 建立一个局部 `entered_value`，不借用频率或 Vpp 变量做输入缓存。 |
| 431～434 | 从 `fuyong` 数字输入模块读取已经解析的 `float`。 |
| 436～443 | 编辑频率时按 Hz 检查 100～10000，通过后写 `g_frequency_hz`。 |
| 445～452 | 编辑 Vpp 时按 V 检查 0.2～3.0。 |
| 454～462 | 方波输入单位是 %；先检查 5～95，再除以 100 转成模块接口的 0～1。 |
| 464～472 | 锯齿波同样把 5～100% 转成 0.05～1.00。 |
| 474 | 当前组合不允许写形状时返回失败。 |

比赛换题时，通常只需要改这个函数的单位解释和 `signal_config.h` 范围，不要改数字解析模块。

### 8.7 `ResetParameters()`：第 477～487 行

逐项恢复波形参数和编辑项，并重新初始化数字输入对象。方波占空比与锯齿波对称度一直是两个独立变量，切换波形不会互相覆盖。

### 8.8 `HandleKey()` 与脏标志：第 489～556 行

| 行段 | 解释 |
|---:|---|
| 489～498 | `GetEditedFieldDirtyMask()` 把当前参数映射为频率、Vpp 或形状的小区域标志。 |
| 502 | 判断当前字符是不是数字。 |
| 508～512 | 已在输入，或空闲时按数字/小数点，就把字符交给复用数字输入模块。 |
| 514～520 | 缓冲已满只标脏 `INPUT`；无事件则一个像素也不刷新。 |
| 521～528 | `#` 确认成功后返回“更新DDS + INPUT + 当前参数字段”，范围错误只刷新 `INPUT`。 |
| 530～533 | 数字追加、删除或取消只返回 `UI_DIRTY_INPUT`。 |
| 538～549 | `A/B` 只标记确实可能发生文字或颜色变化的字段。 |
| 551～553 | 恢复默认值时标记全部动态字段，但仍逐字段清除，不清整页。 |

### 8.9 `main()`：第 559～595 行

| 行段 | 解释 |
|---:|---|
| 562 | 初始化复制来的 SysConfig 硬件。 |
| 567～568 | 初始化数字输入、DDS、TFT，然后输出默认波形。 |
| 573～574 | 固定内容画一次，再用全部动态脏标志画初值。 |
| 583 | 每约 5 ms 扫描键盘一次，保证三次消抖有真实时间间隔。 |
| 584～585 | 只处理一次新按下事件并得到脏标志。 |
| 587～590 | 只有确认值、切换波形或复位才重启 DDS；成功后只追加 `F OUT` 脏标志。 |
| 592 | 根据位掩码刷新对应字符框。数字输入通常只有 `UI_DIRTY_INPUT`。 |

主循环不使用 `__WFI()`，因为矩阵键盘需要持续轮询；没有唤醒中断时使用 `__WFI()` 可能让下一次扫描永远不发生。

## 9. DDS 实际频率为什么可能不同

DMA 缓冲使用整数点数：

```text
N = round(100000 / F_SET)
F_OUT = 100000 / N
```

例如输入 `3000#`：`N=33`，实际约 3030.3 Hz，因此屏幕同时显示 `F SET` 和 `F OUT`。

- 100 Hz 使用 1000 点/周期；
- 10 kHz 只有 10 点/周期；
- 高频时 5% 占空比/对称度输入不一定产生可区分的离散波形，这是 100 kHz 更新率的分辨率限制，不是数字输入失败。

## 10. Vpp 和 DAC 安全范围

固定偏置为 1.65 V：

```text
Vmin = 1.65 - Vpp/2
Vmax = 1.65 + Vpp/2
```

输入 `3#` 后约为 0.15～3.15 V，仍在 0～3.3 V 参考范围。复用模块会再次检查上下界。

Vpp 是请求值，不是 ADC 回测值。示波器先用高阻输入；50 Ω 负载、外部运放增益和饱和都会改变真实幅值。

## 11. 常见故障

### 数字按下后波形没有立刻变化

这是正确的。必须按 `#` 确认，避免输入 `5000` 的过程中依次输出 5、50、500、5000 Hz。

### 输入 1.5 V 不知道怎么打小数点

`*` 就是小数点：`1 * 5 #`。也可以用 `* 5 #` 输入 0.5。

### 屏幕显示 RANGE ERROR

检查当前 `EDIT` 项和单位。占空比输入 `25`，不是 `0.25`；频率输入 Hz，不是 kHz。错误不会修改原输出，直接重新按数字即可。

### 想修改参数却写进了另一个参数

输入前看 `EDIT`。输入过程中 `A/B` 被忽略，必须确认或取消后才能换项目。

### 链接提示 `SignalKeypadNumberInput_*` 未定义

确认 `signal_keypad_number_input.c` 已复制，并在 CCS 中 Refresh、取消 Exclude from Build、Clean Build。

### TFT 正常但没有 DAC 波形

示波器测 PA15，不是 PB15。CCS Expressions 中 `g_contest_status=0` 表示最近一次模块调用成功。

## 12. 验证记录

- 新数字输入模块先添加到 `fuyong/70_keypad_usage`，再原样复制；
- 31 个目标复用文件与来源逐一做 SHA-256 比对；
- SysConfig 1.28 严格生成通过；
- TI Arm Clang 5.1.1、C11、`-Wall -Werror` 全模块编译和链接通过；
- Flash 使用约 `0x8620`，SRAM 使用约 `0x0E61`（含栈）；
- 实板、按键和示波器仍为 `NOT_RUN`，需要烧录后验证。

## 13. 上板检查顺序

1. 烧录后确认默认显示 `SINE / 1000.0 Hz / 1.0 Vpp / READY`；
2. PA15 接示波器高阻探头并共地；
3. `5000#`，确认只有按 `#` 后输出才改变；
4. 按 `B` 到 Vpp，输入 `1*5#`；
5. 按 `A` 到方波、按 `B` 到 DUTY，输入 `25#`；
6. 切到锯齿波的 SYMM，输入 `60#`；
7. 分别测试越界值 `50#`、`20000#`、`4#`，确认显示错误且原波形不变；
8. 测试 `D` 删除和 `C` 取消。
