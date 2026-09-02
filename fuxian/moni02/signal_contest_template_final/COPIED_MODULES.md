# Copied Modules

复制日期：2026-08-22。目标文件在复制后逐一进行了 SHA-256 比对，模块文件与下面列出的 `fuyong` 来源完全相同。

| 功能 | 原始路径 | 目标文件 | 本题修改 |
|---|---|---|---|
| DDS、DAC DMA、波表、正弦/方波/三角波/锯齿波、统一输出 | `fuxian/fuyong/90_dds_usage/modules/` | `modules/signal_status.h`、`signal_dac_*`、`signal_dds.*`、`signal_sine.*`、`signal_square.*`、`signal_triangle.*`、`signal_sawtooth.*`、`signal_arbitrary_wave.*`、`signal_wave_output_mspm0g3507.*`、`signal_math.h` | 先在复用库增加公共 `SignalWaveOutput_Start()`，再原样复制；目标副本未修改 |
| 4×4 矩阵键盘 | `fuxian/fuyong/70_keypad_usage/modules/` | `modules/signal_matrix_keypad_4x4.c/.h` | 无 |
| 直接数字输入、十进制解析 | `fuxian/fuyong/70_keypad_usage/modules/` | `modules/signal_keypad_number_input.c/.h` | 先在复用库补齐，再原样复制；目标副本未修改 |
| ST7789、MSPM0 平台层、英文字库 | `fuxian/fuyong/80_tft_usage/modules/` | `modules/signal_tft_st7789*` 与 `signal_tft_st7789_font_data.inc` | 无 |
| DAC/DMA/Timer、TFT SPI/GPIO、键盘 GPIO 硬件配置 | `fuxian/fuyong/90_dds_usage/signal_contest_template.syscfg` | `signal_contest_template.syscfg` | 整文件复制，无手改 |
| DDS 初始化代码 | `fuxian/fuyong/90_dds_usage/main.c` 的 `DDS_INIT` | `main.c` 的 `DDS_INIT` | 输出缓存由 512 点改为 1024 点，以覆盖 100 Hz |
| 新按键读取 | `fuxian/fuyong/70_keypad_usage/main.c` 的 `KEY_READ` | `main.c` 的 `KEY_READ` | 无 |
| TFT 初始化与静态文字结构 | `fuxian/fuyong/80_tft_usage/main.c` | `main.c` 的 `TFT_INIT`、`TFT_STATIC_TEXT` | 只替换题目文字和坐标 |
| 局部字符框刷新方法 | `fuxian/moni01/signal_contest_template_final/main.c` 的 `App_ClearText()`、`App_DrawStaticUi()` | `main.c` 的 `MONI02_LOCAL_REFRESH` | 按本题七个动态字段拆分并增加脏标志 |

`main.c` 中 `MONI02_*` 标记块只负责选择当前参数、赋予 Hz/V/% 单位、范围检查和屏幕排版，不包含键盘文本解析或底层驱动；逐行解释见 `MONI02_DAC_DDS_KEYPAD_GUIDE.md`。
