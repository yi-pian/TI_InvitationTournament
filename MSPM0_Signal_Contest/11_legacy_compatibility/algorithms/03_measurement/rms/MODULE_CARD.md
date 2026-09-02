# MODULE CARD

RECOMMENDED_LEVEL: LEVEL_A_DIRECT_RECIPE；旧 API 兼容保留，新工程见 `00_docs/recipes/rms.md`。

MODULE: RMS

CATEGORY: Measurement / Energy

功能：计算包含 DC 的总有效值。

输入：`float voltage_v[]`、`count`。

输出：`rms_v`，单位 V。

是否原地处理：不适用；不修改输入。

依赖：标准 C `sqrtf`、公共算法状态码。

典型用途：任意波形总 RMS、等效加热/功率相关幅度。

不要用于：只想测交流分量但输入有 DC；记录不能代表目标时间段。

计算量：LOW，O(N) 加一次开方；Cortex‑M0+ 为软件浮点。

RAM：O(1)。

Benefits：适用于非正弦波，反映平方能量。

Trade-offs：平方放大异常点影响；总 RMS 会包含 DC。

可连接：`ADC_ToVoltage -> RMS` 或 `RemoveDC -> RMS`。

状态：PC_VERIFIED。2026-08-07 通过带 DC 正弦总 RMS 真值测试；未实板验证。
