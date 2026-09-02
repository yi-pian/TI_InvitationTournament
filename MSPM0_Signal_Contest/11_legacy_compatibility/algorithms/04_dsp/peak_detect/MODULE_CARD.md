# MODULE CARD

RECOMMENDED_LEVEL: LEVEL_A_DIRECT_RECIPE；旧 API 兼容保留，新工程见 `00_docs/recipes/peak_detect.md`。

MODULE: Peak Detect

CATEGORY: DSP / Spectrum Utility

功能：在用户指定闭区间找首次最大值。

输入：float spectrum、count、start/end；输出 peak_index/value。

是否原地处理：不适用；只读。

依赖：公共状态码。

典型用途：排除 DC 后找主峰。

不要用于：把全局最大谱线无条件当基波；局部峰形需要多峰检测时。

计算量 LOW O(range)，RAM O(1)。

Benefits：搜索范围明确。Trade-offs：只返回一个峰；最大 bin 不是精确频率。

可连接：`Magnitude -> Peak -> Interpolation`。

状态：PC_VERIFIED；已知数组和完整 FFT 峰索引通过。
