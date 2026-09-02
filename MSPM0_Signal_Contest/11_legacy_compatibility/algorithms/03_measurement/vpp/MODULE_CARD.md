# MODULE CARD

RECOMMENDED_LEVEL: LEVEL_A_DIRECT_RECIPE；旧 API 兼容保留，新工程见 `00_docs/recipes/vpp.md`。

MODULE: Peak-to-Peak (Vpp)

CATEGORY: Measurement / Amplitude

功能：用最高电压减最低电压得到峰峰值。

输入：`float voltage_v[]`、`count`。

输出：`amplitude_vpp`、`min_voltage_v`、`max_voltage_v`，单位 V。

是否原地处理：不适用；不修改输入。

依赖：公共算法状态码。

典型用途：干净周期波形的幅度测量。

不要用于：有异常毛刺、记录未覆盖峰谷、削顶波形仍要推断原始幅度。

计算量：LOW，O(N)。

RAM：O(1)。

Benefits：简单快速，波形不限于正弦。

Trade-offs：极端敏感于单点异常；采样相位会导致低估。

可连接：`ADC_ToVoltage -> Vpp -> display`；有毛刺时改用 `Hampel -> RobustVPP`。

状态：PC_VERIFIED。2026-08-07 已通过 1.0 Vpp 正弦真值测试；未实板验证。
