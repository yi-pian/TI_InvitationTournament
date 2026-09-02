# example02 复制模块

来源均为 `MSPM0_Signal_Contest` 冻结模块，复制到本工程 `modules/`。除下文列出的 `signal_tft_st7789.c` 已确认缺陷修复外，未修改模块 `.c/.h`。

| 文件 | 来源 | 用途 |
|---|---|---|
| `signal_dual_adc_mspm0g3507.c/.h` | `02_acquisition/adc_dual_sync/` | 同步输入/输出测量 |
| `signal_dac_dma_mspm0g3507.c/.h` | `06_generator/dac_dma/` | DAC12 定时 DMA 激励 |
| `signal_wave_output_mspm0g3507.c/.h` | `06_generator/wave_output/` | 三参数统一波形输出封装 |
| `signal_dac_wave_table.c/.h`、`signal_sine.c/.h`、`signal_square.c/.h`、`signal_triangle.c/.h`、`signal_sawtooth.c/.h` | `06_generator/` 对应目录 | 新封装的波表与四种波形依赖 |
| `signal_dds.c/.h` | `06_generator/dds/` | 软件 DDS 波表填充 |
| `signal_frequency_sweep.c/.h` | `06_generator/frequency_sweep/` | 扫频点生成 |
| `signal_phase.c/.h` | `03_measurement/phase/` | 过零相位曲线 |
| `signal_matrix_keypad_4x4.c/.h` | `01_bsp/matrix_keypad_4x4/` | 键盘控制 |
| `signal_tft_st7789.c/.h`、`signal_tft_st7789_mspm0g3507.c/.h` | `12_external_devices/display/st7789/` | ST7789 |
| `signal_status.h`、算法依赖头 | 模块公共依赖 | 状态码和类型 |

example02 的 DAC DMA 使用 `DMA_CH2`，因为双 ADC 已占用 `DMA_CH0/CH1`；这是对 README 已验证资源的本工程差异。新封装不新增 SysConfig 资源，继续沿用 DAC DMA README 的 DAC、Timer、Event、DMA 配置。状态：`BOARD NOT_RUN`。

## ST7789 库同步修复（2026-08）

`signal_tft_st7789.c` 的 `TFT_ST7789_DrawLine()` 已与公共库 `12_external_devices/display/st7789/` 同步：在一次 Bresenham 循环中保存 `e2 = 2 * err`，两个方向条件均使用该快照。旧实现会在部分斜线中误更新 `err`，导致端点不收敛、线条越出绘图区并持续执行。此项是确认后的模块缺陷修复；未改变接口和 SysConfig。
