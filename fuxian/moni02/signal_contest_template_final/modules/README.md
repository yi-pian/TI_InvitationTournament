# modules：moni02 冻结复用副本

本目录只存从 `fuyong` 复制的模块，不在这里编写 moni02 题目逻辑。

## 文件分组

- DDS/DAC：`signal_dac_*`、`signal_dds.*`、`signal_wave_output_mspm0g3507.*`；
- 波形：`signal_sine.*`、`signal_square.*`、`signal_triangle.*`、`signal_sawtooth.*`、`signal_arbitrary_wave.*`、`signal_math.h`；
- 键盘扫描：`signal_matrix_keypad_4x4.*`；
- 直接数字输入：`signal_keypad_number_input.*`；
- TFT：`signal_tft_st7789.*`、`signal_tft_st7789_mspm0g3507.*`、`signal_tft_st7789_font.*`、`signal_tft_st7789_font_data.inc`；
- 公共状态：`signal_status.h`。

在 CCS 中复制新 `.c` 后必须：右键工程 **Refresh** → 检查 **Exclude from Build** → **Clean** → **Build Project**。

若要改通用算法或硬件适配，先改 `fuyong` 并验证，再重新复制；不要只改本目录导致比赛副本与复用库分叉。
