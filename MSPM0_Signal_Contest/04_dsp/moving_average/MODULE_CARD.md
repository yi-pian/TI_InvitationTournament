# MODULE CARD

RECOMMENDED_LEVEL: LEVEL_B_SIMPLE_HELPER；无 Init/context/result，单次 Process。

MODULE: Moving Average

CATEGORY: DSP / Smoothing

功能：用当前点及前 `window_size-1` 点的均值做因果平滑。

输入：`float input_samples[]`、`count`、`window_size`。

输出：`float output_samples[]`，单位不变。

是否原地处理：NO。

依赖：公共算法状态码。

典型用途：降低缓慢变化 DC/低频量上的随机噪声。

不要用于：保留快速边沿、高频幅值、精确相位或单次瞬态。

计算量：LOW，O(N)，滚动和。

RAM：模块内部 O(1)；输出 `4*N` 字节。

Benefits：参数直观、计算量低、随机噪声更小。

Trade-offs：降低带宽、使变化滞后、产生频率相关幅值衰减；每帧开头有暖机边界。

可连接：`ADC_ToVoltage -> MovingAverage -> Mean/ZeroCross`。

状态：PC_VERIFIED。2026-08-07 通过手算序列和参数错误测试；未实板验证。
