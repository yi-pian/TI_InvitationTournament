# Reuse Test 02 — Dual Channel

来源 COPY 区：`04/DUAL_ADC_DMA`、`40/PHASE`、`80/TFT_VARIABLE + TFT_LIVE_VALUE`。

`adc_ch1_samples`、`adc_ch2_samples`、`SAMPLE_COUNT`、`sample_rate_hz` 无变量重命名，04 的输出直接进入 40。

`delay_s` 不是模块伪造输出：以 `phase_deg / (360 * reference_frequency_hz)` 明确计算。SysConfig 直接复制 `PROFILE_06_FULL_SIGNAL`，无需改实例或引脚。
