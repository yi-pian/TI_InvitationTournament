# Copied Modules

本表按比赛现场“先选模块、再复制冻结文件”记录。AD9833 是本题的激励模块；原 DAC/DDS 文件保留在母版目录，但已排除出 Debug Build。所有复制到 `modules/` 的 `.c/.h/.inc` 均未修改。

| 模块 | 原始路径 | 复制内容 | 本题修改 |
|---|---|---|---|
| 双 ADC 同步采集 | `MSPM0_Signal_Contest/02_acquisition/adc_dual_sync/` | `signal_dual_adc_mspm0g3507.c/.h`、`signal_status.h` | 未修改；A 解释为电流、B 解释为电压 |
| AD9833 外部 DDS | `MSPM0_Signal_Contest/12_external_devices/dds/ad9833/` | `ad9833.c/.h` | 未修改；main 使用 SysConfig 生成的独立 `SPI_AD9833_INST` |
| AD9833 公共总线 | `MSPM0_Signal_Contest/12_external_devices/00_common/` | `mspm0_blocking_bus.c/.h` | 未修改；由 AD9833 驱动调用 |
| 对数扫频 | `MSPM0_Signal_Contest/06_generator/frequency_sweep/` | `signal_frequency_sweep.c/.h` | 未修改；32 点、1 kHz～100 kHz |
| ST7789 与 8×16 字库 | `MSPM0_Signal_Contest/12_external_devices/display/st7789/` | `signal_tft_st7789.c/.h`、平台层、字体 `.c/.h/.inc` | 未修改；全部数值固定 8×16 |

## 冻结文件核验

`ad9833.c/.h` 和 `mspm0_blocking_bus.c/.h` 已用 SHA-256 与原始路径逐一比对，四个哈希完全一致。禁止编辑 `modules/` 下任何模块源文件；题目组合只写在 `main.c`、`signal_config.h`、`.syscfg` 和文档中。

## 旧母版文件

`signal_dac_dma_mspm0g3507.c`、`signal_dac_wave_table.c`、`signal_dds.c`、`signal_sine.c`、`signal_square.c`、`signal_triangle.c`、`signal_sawtooth.c`、`signal_wave_output_mspm0g3507.c` 仅是母版遗留文件，不是本题选择模块。`.cproject` 已写入 Debug 的 source exclusion；CCS Refresh 后应确认它们显示为 **Excluded from Build**，不删除、不改内容。这样重新生成 `Debug/modules/subdir_vars.mk` 时会只编译 AD9833、双 ADC、扫频和 ST7789 组合所需源文件。
