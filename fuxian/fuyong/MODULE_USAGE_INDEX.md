# 模块反向索引

| 模块 | 主题工程 | COPY 区 |
|---|---|---|
| `signal_adc_dma` | 02 | `ADC_DMA` |
| `signal_dual_adc_mspm0g3507` | 04、11、20、21、30、40、50、60、61 | acquisition/common |
| `signal_timer_capture_mspm0g3507` | 10 | `TIMER_CAPTURE` |
| `signal_zero_cross` / interpolation | 11 | `ZERO_CROSS_*` |
| `signal_window` / gain correction | 20 | `FFT_COMMON` |
| `signal_fft_parabolic_interpolation` | 20 | `FFT_PEAK_INTERPOLATION` |
| `signal_harmonic` / `signal_thd` | 20 | `FFT_HARMONICS` / `FFT_THD` |
| `signal_snr` / `signal_sfdr` | 20 | `FFT_SNR_SFDR` |
| `signal_tft_st7789*` | 21、80 | time plot / TFT blocks |
| `signal_dual_adc_phase` | 40 | `PHASE` |
| `signal_median_filter` / `signal_hampel` / `signal_mad` | 50 | matching blocks |
| `signal_robust_peak_to_peak` / `signal_robust_rms` | 50 | matching blocks |
| `signal_sine_fit_3param` / 4param | 60 | matching blocks |
| `signal_lock_in` | 61 | `LOCK_IN` |
| `signal_matrix_keypad_4x4` | 70 | `KEY_READ` |
| `signal_wave_output_mspm0g3507` + DDS/DAC dependencies | 90 | `DDS_*` |
