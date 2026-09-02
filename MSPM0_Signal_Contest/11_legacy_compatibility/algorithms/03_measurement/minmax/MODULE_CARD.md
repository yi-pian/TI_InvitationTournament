# MODULE CARD

RECOMMENDED_LEVEL: LEVEL_A_DIRECT_RECIPE；旧 API 兼容保留，新工程见 `00_docs/recipes/minmax.md`。

MODULE: MinMax

CATEGORY: Measurement / Basic

功能：找出最小值、最大值及其第一次出现的索引。

输入：`const float samples[]`、`count`，单位任意但必须明确。

输出：`min_value`、`max_value`、`min_index`、`max_index`。

是否原地处理：不适用；不修改输入。

依赖：公共算法状态码。

典型用途：查看信号范围、为 Vpp 提供极值、定位限幅位置。

不要用于：有孤立毛刺时直接声称极值代表真实波形峰谷。

计算量：LOW，O(N)。

RAM：O(1)。

Benefits：快、直观、无工作区。

Trade-offs：一个异常点就能完全改变结果；采样未落在真实峰值时会低估。

可连接：`ADC_ToVoltage -> MinMax -> Vpp/diagnostics`。

状态：PC_VERIFIED。2026-08-07 通过 PC 严格编译与已知数组测试；未实板验证。
