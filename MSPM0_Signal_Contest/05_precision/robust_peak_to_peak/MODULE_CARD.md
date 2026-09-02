# MODULE CARD

MODULE: Robust Peak-to-Peak

CATEGORY: Precision / Robust Measurement

功能：用上下分位数差估计抗毛刺 Vpp。

输入：float V、count、quantiles、N-float workspace；输出上下界和 robust_vpp_v。

是否原地处理：NO；原输入只读，workspace 被重排。

依赖：公共状态码。

典型用途：偶发 ADC 异常值下的周期幅度测量。

不要用于：真实窄脉冲、过冲和尖峰测量。

计算量 MEDIUM，平均 O(N)；RAM 4N bytes。

Benefits：不被单个毛刺欺骗。Trade-offs：舍弃真实尾部，参数会引入偏差。

可连接：`Voltage -> Hampel(optional) -> RobustVPP`。

状态：PC_VERIFIED；分位数真值通过，未实板。
