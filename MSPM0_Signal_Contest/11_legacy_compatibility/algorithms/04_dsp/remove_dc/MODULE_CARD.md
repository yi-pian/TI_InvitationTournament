# MODULE CARD

RECOMMENDED_LEVEL: LEVEL_A_DIRECT_RECIPE；旧 API 兼容保留，新工程见 `00_docs/recipes/remove_dc.md`。

MODULE: Remove DC

CATEGORY: DSP / Signal Conditioning

功能：从每个电压样本减去整段平均值，使输出均值接近 0。

输入：`float input_voltage_v[]`、`count`。

输出：`float output_centered_v[]`、`removed_mean_v`，单位 V。

是否原地处理：YES，允许输入输出指向同一数组。

依赖：公共算法状态码。

典型用途：过零、FFT、交流 RMS 前删除中点偏置。

不要用于：DC 测量；需要保留真实 DC 频谱；把缓慢趋势误当常量 DC 的场景。

计算量：LOW，O(2N)。

RAM：内部 O(1)；非原地输出 `4*N` 字节，原地为 0 额外数组。

Benefits：消除常量偏置，避免 FFT 的 DC 峰和错误零阈值。

Trade-offs：删除所有常量分量；有限记录的均值可能受非整数周期影响；不能去线性趋势。

可连接：`ADC_ToVoltage -> RemoveDC -> RMS / ZeroCross / Window / FFT`。

状态：PC_VERIFIED。2026-08-07 通过带 DC 正弦测试和输出均值检查；未实板验证。
