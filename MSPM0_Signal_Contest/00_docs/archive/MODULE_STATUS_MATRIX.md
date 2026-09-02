# Module status matrix

状态只表示现有证据。`BUILD_VERIFIED` = GCC PC 回归构建通过，且 87 个正式源文件已被 TI Arm Clang 5.1.1.LTS `-Wall -Werror` 编译并纳入整库链接；这不等于外设已在它自己的 SysConfig 资源组合中实板工作。

总计：87 个正式模块 = 81 BUILD_VERIFIED + 1 BOARD_VERIFIED + 5 DRAFT + 0 CONTEST_VERIFIED。

## MODULE_STATUS_BOARD_VERIFIED

| 模块 | 证据 | 未覆盖 |
|---|---|---|
| `02_acquisition/adc_dma` | LP-MSPM0G3507 TMP6131，N=256..4096，各100帧，500/500、WFE、哨兵、FCC 通过 | PA25 动态输入、100/200/500 kSPS 动态验证、比赛完整链 |

## MODULE_STATUS_BUILD_VERIFIED

| 层 | 模块 |
|---|---|
| BSP | system_clock, gpio, uart, timer, dma, adc, dac, vref, opa, gpamp, comparator |
| Acquisition | adc_basic, adc_timer_trigger, adc_dual_sync, adc_continuous, adc_pingpong_dma, adc_ring_buffer, trigger_capture, timer_capture |
| Measurement | adc_to_voltage, dc_measure, mean, minmax, vpp, rms, ac_rms, frequency_zero_cross, frequency_interpolation, frequency_timer_capture, duty, phase |
| DSP | remove_dc, mean_filter, median_filter, fir_filter, rect_window, hann_window, fft, fft_magnitude, fft_peak, harmonic, thd, correlation |
| Precision | zero_cross_linear_interpolation, multi_cycle_average, fft_parabolic_interpolation, window_gain_correction, coherent_sampling, multi_bin_energy, sine_fit_3param, adc_gain_offset_calibration, channel_delay_calibration |
| Generator | dac_dc, dac_wave_table, dac_dma, dds, sine, square, triangle, sawtooth, arbitrary_wave, frequency_sweep, am_modulation |
| Frontend | opa_buffer, opa_noninverting_pga, opa_inverting, opa_dac_bias, opa_to_adc, gpamp_buffer, gpamp_gain, comparator_zero_cross, comparator_threshold |
| Applications | oscilloscope, frequency_meter, spectrum_analyzer, harmonic_thd_analyzer, dual_channel_phase_meter, dds_generator, sweep_analyzer, waveform_capture_replay, signal_analyzer |

## MODULE_STATUS_DRAFT

| 模块 | 当前行为 | 升级条件 |
|---|---|---|
| jacobsen_interpolation | 返回 NOT_SUPPORTED | 公式、边界、误差向量和参考结果 |
| quinn_interpolation | 返回 NOT_SUPPORTED | 同上 |
| macleod_interpolation | 返回 NOT_SUPPORTED | 同上 |
| czt | 返回 NOT_SUPPORTED | 实现、速度、32 KB 内存预算、参考频谱 |
| frequency_response_correction | 返回 NOT_SUPPORTED | 校准表格式、插值、外推与实测数据 |

## MODULE_STATUS_CONTEST_VERIFIED

当前没有。只有完整题目链在目标范围、目标采样率、真实信号与输出约束下通过后才能升级。
