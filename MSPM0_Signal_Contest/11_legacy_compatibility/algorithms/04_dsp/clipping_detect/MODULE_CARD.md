# MODULE CARD

RECOMMENDED_LEVEL: LEVEL_A_DIRECT_RECIPE；旧 API 兼容保留，新工程见 `00_docs/recipes/clipping_detect.md`。

MODULE: Clipping Detect

CATEGORY: DSP / Data Quality

功能：统计达到低/高电压阈值的样本，判断是否可能削顶。

输入：`voltage_v[]`、`count`、`low_limit_v`、`high_limit_v`。

输出：低/高削顶点数、总点数、比例和标志。

是否原地处理：不适用；不修改输入。

依赖：公共算法状态码。

典型用途：测量前质量门控、提示调整前端增益/量程。

不要用于：把“没有点碰阈值”当作绝对没有模拟削顶；阈值未按真实前端量程设置。

计算量：LOW，O(N)。

RAM：O(1)。

Benefits：在 Vpp/RMS/FFT 前及早发现明显限幅。

Trade-offs：阈值法；噪声和真实贴轨波形都可能触发；不能恢复被截掉的波形。

可连接：`ADC_ToVoltage -> ClippingDetect -> quality gate`。

状态：PC_VERIFIED。2026-08-07 通过阈值点数与比例测试；未实板验证。
