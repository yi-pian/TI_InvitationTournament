# 统一接口兼容性检查

检查日期：2026-08-21。

| 项目 | 结论 |
|---|---|
| 单路 ADC 名称 | 02、11、20、21、30、50、60、61 使用 `adc_samples`；01 为单点数组 `adc_samples[0]`。 |
| 双路 ADC 名称 | 04、40 使用 `adc_ch1_samples` / `adc_ch2_samples`。 |
| 点数 | 所有帧处理工程使用 `SAMPLE_COUNT` 或其 `SIGNAL_SAMPLE_COUNT` 配置来源。 |
| 采样率 | 所有需要采样率的工程使用 `sample_rate_hz`。 |
| 电压/去 DC/频谱 | 11/20/21/30/50/60/61 使用 `voltage_samples`；11/20/30 使用 `centered_samples`；20 使用 `fft_magnitude`。 |
| 常用最终量 | FFT/过零为 `frequency_hz`，基础量为 `_v`，相位为 `phase_deg` / `delay_s`。 |
| UI 页 | 70/80 使用 `current_page`。 |

组合模拟：`02_adc_dma` 提供 `adc_samples + SAMPLE_COUNT + sample_rate_hz` 后，可直接接入 20、21、30、50、60、61 的 COPY 区。各主题没有为同一含义改用 `raw_buffer`、`input_samples` 或 `adc_data`。

例外：10 的输入是比较器/Timer 边沿，不是 ADC 帧；91 的输入是 DAC code；40 的 `delay_s` 需要外部已知基波频率，已在 `main.c` 注释说明。
