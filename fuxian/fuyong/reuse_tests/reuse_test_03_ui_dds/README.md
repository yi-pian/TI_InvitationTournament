# Reuse Test 03 — Keypad + DDS + TFT

来源 COPY 区：`70/KEY_READ + PARAMETER_ADJUST`、`90/DDS_INIT + DDS_SET_FREQUENCY`、`80/TFT_VARIABLE + TFT_LIVE_VALUE`。

使用统一 `frequency_hz` 贯穿按键步进、DDS 输出和 TFT 显示；没有 `input_value`、`target_freq` 或 `display_frequency` 等重命名胶水。按 `*` 降 10 Hz，按 `#` 升 10 Hz。

SysConfig 直接复制 `PROFILE_06_FULL_SIGNAL`；没有实例、引脚或 DMA 配置修改。
