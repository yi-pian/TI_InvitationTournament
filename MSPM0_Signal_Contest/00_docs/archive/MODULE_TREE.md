# Complete module tree

```text
MSPM0_Signal_Contest/
├─ 00_docs/
├─ 01_bsp/
│  ├─ common
│  ├─ system_clock  gpio  uart  timer  dma  adc
│  └─ dac  vref  opa  gpamp  comparator
├─ 02_acquisition/
│  ├─ adc_basic  adc_timer_trigger  adc_dma  adc_dual_sync
│  ├─ adc_continuous  adc_pingpong_dma  adc_ring_buffer
│  └─ trigger_capture  timer_capture
├─ 03_measurement/
│  ├─ adc_to_voltage  dc_measure  mean  minmax  vpp  rms  ac_rms
│  ├─ frequency_zero_cross  frequency_interpolation
│  └─ frequency_timer_capture  duty  phase
├─ 04_dsp/
│  ├─ remove_dc  mean_filter  median_filter  fir_filter
│  ├─ rect_window  hann_window  fft  fft_magnitude  fft_peak
│  └─ harmonic  thd  correlation
├─ 05_precision/
│  ├─ zero_cross_linear_interpolation  multi_cycle_average
│  ├─ fft_parabolic_interpolation  window_gain_correction
│  ├─ coherent_sampling  multi_bin_energy  sine_fit_3param
│  ├─ adc_gain_offset_calibration  channel_delay_calibration
│  └─ jacobsen_interpolation  quinn_interpolation
│     macleod_interpolation  czt  frequency_response_correction
├─ 06_generator/
│  ├─ dac_dc  dac_wave_table  dac_dma  dds  sine  square
│  └─ triangle  sawtooth  arbitrary_wave  frequency_sweep  am_modulation
├─ 07_signal_frontend/
│  ├─ opa_buffer  opa_noninverting_pga  opa_inverting  opa_dac_bias
│  └─ opa_to_adc  gpamp_buffer  gpamp_gain
│     comparator_zero_cross  comparator_threshold
├─ 08_applications/
│  ├─ oscilloscope  frequency_meter  spectrum_analyzer
│  ├─ harmonic_thd_analyzer  dual_channel_phase_meter  dds_generator
│  └─ sweep_analyzer  waveform_capture_replay  signal_analyzer
├─ 09_examples/
│  ├─ adc_dma_demo  adc_dma_onboard_selftest
│  └─ adc_buffer_uart_dump
├─ 10_tests/
│  ├─ pc
│  └─ ticlang
└─ tools/
   ├─ pc
   ├─ generate_module_docs.ps1
   └─ validate_ticlang.ps1
```

正式模块共 87 个：81 个 `BUILD_VERIFIED`、1 个 `BOARD_VERIFIED`、5 个高级预留接口 `DRAFT`、0 个 `CONTEST_VERIFIED`。顶层目录中若存在早期的空占位目录，它们不包含源文件、不计入 87 个正式模块，也不应加入 CCS 工程。
