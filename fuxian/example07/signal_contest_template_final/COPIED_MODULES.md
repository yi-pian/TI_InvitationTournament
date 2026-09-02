# example07 模块复制清单

复制来源为仓库 `MSPM0_Signal_Contest` 的对应模块目录，以及已完成同类母版中的冻结副本：

- `signal_dual_adc_mspm0g3507.c/.h`
- `signal_dac_dma_mspm0g3507.c/.h`
- `signal_dac_wave_table.c/.h`、`signal_sine.c/.h`、`signal_dds.c/.h`
- `signal_frequency_sweep.c/.h`
- `signal_lock_in.c/.h`
- `signal_frequency_response_correction.c/.h`
- `signal_status.h`、`signal_algorithm_status.h`、`signal_math.h`
- `signal_matrix_keypad_4x4.c/.h`
- `signal_tft_st7789.c/.h`、`signal_tft_st7789_mspm0g3507.c/.h`
- `signal_tft_st7789_font.c/.h`、`signal_tft_st7789_font_data.inc`

以上模块源码原样复制。双 ADC 的正确来源必须是集成库 `MSPM0_Signal_Contest/02_acquisition/adc_dual_sync`：该版本与 22_X 一致，已在 `SignalDualADC_Init()` 打开 DMA_CH0/DMA_CH2 完成中断；example07 已同步为该版本。先前问题源于误复制旧 example05 副本，而不是集成库缺少修复。README 中缺少 example07 组合题说明的部分已补充到 `modules/README.md` 和步骤文档。
