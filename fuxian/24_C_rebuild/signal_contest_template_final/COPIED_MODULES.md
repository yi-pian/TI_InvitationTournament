# 24_C 模块复制记录

全部文件于 2026-08-18 从 `MSPM0_Signal_Contest` 集成库复制，库内正式 `.c/.h` 未修改。

| 模块 | 集成库目录 | 复制内容 |
|---|---|---|
| 双路同步 ADC | `02_acquisition/adc_dual_sync` | MSPM0G3507 `.c/.h` |
| Timer Capture | `02_acquisition/timer_capture` | core + MSPM0 platform `.c/.h` |
| Harmonic | `04_dsp/harmonic` | `.c/.h` |
| Multi-bin energy | `05_precision/multi_bin_energy` | `.c/.h` |
| ST7789 | `12_external_devices/display/st7789` | core、MSPM0 platform、font `.c/.h/.inc` |
| ST7789 波形 | `01_bsp/tft_waveform_st7789` | `signal_tft_waveform_st7789.c/.h` |
| 公共依赖 | `01_bsp/common`、`03_measurement/common` | `signal_status.h`、`signal_types.h`、`signal_algorithm_status.h` |
