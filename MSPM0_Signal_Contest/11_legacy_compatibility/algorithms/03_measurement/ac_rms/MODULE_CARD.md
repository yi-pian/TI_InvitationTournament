# MODULE CARD

RECOMMENDED_LEVEL: LEVEL_A_DIRECT_RECIPE；旧 API 兼容保留，新工程见 `00_docs/recipes/ac_rms.md`。

MODULE: AC RMS

CATEGORY: Measurement / Energy

功能：先估计平均 DC，再计算去均值后的交流 RMS；不改写输入。

输入：`float voltage_v[]`、`count`。

输出：`mean_voltage_v`、`ac_rms_v`，单位 V。

是否原地处理：不适用；两遍只读扫描。

依赖：标准 C `sqrtf`、公共算法状态码。

典型用途：带中点偏置的 ADC 正弦/任意波形交流有效值。

不要用于：要保留 DC 功率的总 RMS；记录太短导致均值不是实际 DC。

计算量：LOW，O(2N) 加一次开方。

RAM：O(1)。

Benefits：无需额外 `4*N` RemoveDC 输出 buffer；同时返回 DC。

Trade-offs：两遍扫描；有限记录上的均值估计会把部分超低频当 DC 删除。

可连接：`ADC_ToVoltage -> AC_RMS -> result`。

状态：PC_VERIFIED。2026-08-07 通过 1.65 V 偏置正弦真值测试；未实板验证。
