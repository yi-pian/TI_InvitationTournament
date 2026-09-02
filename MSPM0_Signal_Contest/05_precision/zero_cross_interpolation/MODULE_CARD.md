# MODULE CARD

MODULE: Zero Cross Linear Interpolation

CATEGORY: Precision / Timing

功能：用阈值两侧相邻样本连线，估计带小数的过零样本位置。

输入：电压数组、阈值、ZeroCross 事件数组。

输出：`crossing_positions_samples[]`，单位 sample。

是否原地处理：NO。

依赖：ZeroCross 事件类型、公共算法状态码。

典型用途：提高过零频率/相位的时间分辨率。

不要用于：两个夹点不包含阈值、阈值附近严重弯曲/噪声、把插值当成增加真实 ADC 带宽。

计算量：LOW，O(K)，K 为事件数。

RAM：模块内部 O(1)；输出 `4*K` 字节。

Benefits：从整数样本位置得到亚样本估计，代价小。

Trade-offs：依赖局部线性和样本幅值精度；不会消除系统采样时钟误差。

可连接：`ZeroCross -> LinearInterpolation -> MultiCycleAverage`。

状态：PC_VERIFIED。2026-08-07 通过解析位置与正弦频率真值测试；未实板验证。
