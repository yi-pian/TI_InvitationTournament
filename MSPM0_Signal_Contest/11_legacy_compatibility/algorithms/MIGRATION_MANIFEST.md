# Algorithm Compatibility Migration Manifest

状态：`FROZEN_COMPATIBILITY`。下表路径相对于 `MSPM0_Signal_Contest/`。

| 原正式路径 | 当前兼容路径 | 新工程默认 |
|---|---|---|
| `03_measurement/mean` | `11_legacy_compatibility/algorithms/03_measurement/mean` | CMSIS Mean |
| `03_measurement/minmax` | `11_legacy_compatibility/algorithms/03_measurement/minmax` | CMSIS Min/Max |
| `03_measurement/vpp` | `11_legacy_compatibility/algorithms/03_measurement/vpp` | CMSIS Min/Max Recipe |
| `03_measurement/rms` | `11_legacy_compatibility/algorithms/03_measurement/rms` | CMSIS RMS |
| `03_measurement/ac_rms` | `11_legacy_compatibility/algorithms/03_measurement/ac_rms` | CMSIS Mean + Offset + RMS Recipe |
| `03_measurement/statistics` | `11_legacy_compatibility/algorithms/03_measurement/statistics` | CMSIS Statistics |
| `03_measurement/adc_to_voltage` | `11_legacy_compatibility/algorithms/03_measurement/adc_to_voltage` | Direct Recipe |
| `04_dsp/remove_dc` | `11_legacy_compatibility/algorithms/04_dsp/remove_dc` | CMSIS Mean + Offset Recipe |
| `04_dsp/fft` | `11_legacy_compatibility/algorithms/04_dsp/fft` | CMSIS CFFT/RFFT |
| `04_dsp/fft_magnitude` | `11_legacy_compatibility/algorithms/04_dsp/fft_magnitude` | CMSIS Complex Magnitude |
| `04_dsp/fir` | `11_legacy_compatibility/algorithms/04_dsp/fir` | CMSIS FIR |
| `04_dsp/iir_biquad` | `11_legacy_compatibility/algorithms/04_dsp/iir_biquad` | CMSIS Biquad |
| `04_dsp/clipping_detect` | `11_legacy_compatibility/algorithms/04_dsp/clipping_detect` | Direct Recipe |
| `04_dsp/peak_detect` | `11_legacy_compatibility/algorithms/04_dsp/peak_detect` | Direct Recipe / contest interpolation when required |
| `05_precision/multi_cycle_average` | `11_legacy_compatibility/algorithms/05_precision/multi_cycle_average` | Direct Recipe |
| `03_measurement/dc_measure` | `11_legacy_compatibility/algorithms/03_measurement/dc_measure` | CMSIS Mean + ADC conversion Recipe |
| `04_dsp/fft_peak` | `11_legacy_compatibility/algorithms/04_dsp/fft_peak` | CMSIS FFT/Magnitude + formal peak/interpolation Recipe |

没有迁移 `04_dsp/correlation`：它不是普通 full correlation，而是具有受限 lag、归一化和时延语义的竞赛专用实现。Convolution 和 Matrix 没有旧自建正式 `.c/.h`，因此无文件可迁移。
