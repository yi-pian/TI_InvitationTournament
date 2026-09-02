# example08 复制模块记录

按 README 先选择、后复制到 `modules/`。除已证实存在 Bresenham 无限循环缺陷的 `signal_tft_st7789.c` 外，所有下列 `.c/.h/.inc` 均未修改；该单文件已获授权修复并与集成库同步。其余修改仅在组合层 `main.c`、`signal_config.h`、SysConfig 和步骤文档。

| 文件 | 原始路径 | 用途 |
|---|---|---|
| `signal_status.h`、`signal_algorithm_status.h` | `01_bsp/common`、`03_measurement/common` | 公共状态 |
| `signal_opa.c/.h` | `01_bsp/opa` | OPA 增益计算 |
| `signal_opa_noninverting_pga.c/.h` | `07_signal_frontend/opa_noninverting_pga` | 保留的 PGA 预算模块（本版未启用，避免带 1.65 V 偏置的信号饱和） |
| `signal_opa_inverting.c/.h` | `07_signal_frontend/opa_inverting` | 保留的反相 PGA 预算模块（本版未启用，主采集采用 TI Buffer Quick Profile） |
| `signal_opa_to_adc.c/.h` | `07_signal_frontend/opa_to_adc` | ADC 量程检查 |
| `signal_gpamp.c/.h`、`signal_gpamp_buffer.c/.h` | `01_bsp/gpamp`、`07_signal_frontend/gpamp_buffer` | GPAMP 单位增益缓冲预算 |
| `signal_comparator.c/.h`、`signal_comparator_threshold.c/.h`、`signal_comparator_zero_cross.c/.h` | `01_bsp/comparator`、`07_signal_frontend/comparator_*` | 比较器门限/过零预算 |
| `signal_dual_adc_mspm0g3507.c/.h` | `02_acquisition/adc_dual_sync` | Timer/Event/ADC/DMA 同步采样 |
| `signal_dual_adc_phase.c/.h` | `fuxian/22_X/.../modules` | 相位算法 |
| `signal_matrix_keypad_4x4.c/.h` | `01_bsp/matrix_keypad_4x4` | 键盘扫描/消抖 |
| `signal_tft_st7789*.c/.h`、`signal_tft_st7789_font_data.inc` | `12_external_devices/display/st7789` | LCD 与字库 |

复制后对新增的 14 个 GPAMP/比较器/OPA 文件完成 SHA-256 比对，来源与副本一致。

## 例外：ST7789 缺陷修复

`signal_tft_st7789.c` 的 `TFT_ST7789_DrawLine()` 在更新 `err` 后错误地再次读取它来决定 Y 步进；部分斜率会越过终点并永不退出。只增加了 `e2` 快照，两个条件均以 `e2` 判断。修改已同步到 `MSPM0_Signal_Contest/12_external_devices/display/st7789/signal_tft_st7789.c`；二者 SHA-256 一致。模块 API、头文件、平台适配和其他模块均未修改。
