# fuyong 教学函数依赖索引

| 能力 | 上游数据 | COPY 函数 | 现有模块 / 库 | 不应重复执行的计算 |
|---|---|---|---|---|
| 滤波 | `voltage_samples[]` V | 15 的三类滤波 COPY 区 | Median、Hampel、CMSIS-DSP | 同一帧只选一条滤波链 |
| 双通道显示 | 两路 ADC code 或 V | `Waveform_ConvertToVoltage` → `Waveform_Draw` | 双 ADC、ST7789 | 同一帧只做一次 code→V |
| 频谱显示 | 已有 `fft_magnitude[]` | `Spectrum_Draw` | ST7789、math | 只显示，绝不再次 FFT |
| 触发提取 | `adc_samples[]` code | `Trigger_Capture` | `signal_trigger_capture` | 每帧仅一次边沿搜索/提取 |
| 自动量程 | `voltage_samples[]` V | `AutoRange_Update` | math | 可复用显示前已得到的峰值 |
| ADC 校准 | V 数组、两点参考 | `Calibration_ApplyADC` | gain/offset calibration | 系数确定后每帧仅应用一次 |
| 延迟校准 | 已测相位、Hz | `Calibration_ApplyDelay` | channel delay calibration | 固定系数应复用，不必每帧重算 |

所有函数都留在各工程 `main.c`，没有为这些教学能力新增 Feature/Core/Context 或修改原有模块源码。
