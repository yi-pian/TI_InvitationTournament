# MODULE CARD

RECOMMENDED_LEVEL: LEVEL_A_DIRECT_RECIPE；旧 API 兼容保留，新工程见 `00_docs/recipes/multi_cycle_average.md`。

MODULE: Multi Cycle Average

CATEGORY: Precision / Frequency

功能：用首末同方向过零之间的总时间除以周期数，再换算频率。

输入：严格递增、同方向的 `crossing_positions_samples[]`、`sample_rate_hz`。

输出：`frequency_hz`、平均周期（sample）、观测时间（s）、周期数。

是否原地处理：不适用；不修改输入。

依赖：公共算法状态码。

典型用途：降低单个过零时间误差对频率的影响。

不要用于：混合上升/下降事件、频率在记录内明显变化、少于两个同方向事件。

计算量：LOW，O(K) 验证 + O(1) 计算。

RAM：O(1)，复用上游位置数组。

Benefits：误差除以跨越的周期数，接口直观。

Trade-offs：观测越长更新越慢；不能修复错误采样率；输出是整段平均频率。

可连接：`LinearInterpolation -> MultiCycleAverage -> Frequency result`。

状态：PC_VERIFIED。2026-08-07 通过已知位置与两条正弦链测试；未实板验证。
