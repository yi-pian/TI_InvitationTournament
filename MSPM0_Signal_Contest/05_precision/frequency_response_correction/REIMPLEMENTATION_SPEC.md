# Frequency response correction clean reimplementation specification

状态：`SPECIFICATION_COMPLETE`。类型：`SOURCE_LOST → CLEAN_REIMPLEMENTATION`。

## Correction model

调用者离线标定得到有序 LUT：`frequency_hz / gain_correction_linear / phase_correction_deg`。表内以线性 Hz 或 log Hz 插值；相位沿最短 ±180° 路径插值。表项直接保存“应乘/应加的修正量”：

```text
gain_corrected = gain_measured × gain_correction
phase_corrected = wrap180(phase_measured + phase_correction)
```

- 输入：至少一个严格递增正频率点、正有限 gain correction、有限 phase correction；查询频率和测得幅相。
- 输出：修正结果、实际修正量、插值 fraction 和上下索引。
- 越界策略：明确 REJECT 或 CLAMP；绝不静默外推。
- O(K) 验证/查找、O(1) RAM，无动态内存。

校准值必须来自已知参考链；本模块不“猜”前端误差。NIST 校准手册说明复杂校准曲线可由预测值/图表插值取得逆修正：[NIST calibration of future measurements](https://www.itl.nist.gov/div898/handbook/mpc/section3/mpc366.htm)。

旧 `SignalFrequencyResponseCorrection_Apply` 签名丢失；新 `..._Process` 是 breaking clean API。验证覆盖线性/log 插值、跨 ±180°、clamp/reject、无序/重复表点和非有限值。
