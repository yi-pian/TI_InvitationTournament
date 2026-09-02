# MODULE CARD

MODULE: Robust RMS

CATEGORY: Precision / Robust Measurement

功能：分位数 Winsorize 后计算总/交流 RMS。

输入：float V、count、quantiles、remove_dc、N-float workspace；输出 RMS V、边界、均值和计数。

是否原地处理：NO；输入只读，workspace 可变。

依赖：RobustPeakToPeak。

典型用途：偶发 ADC 异常值下的稳定 RMS。

不要用于：真实冲击、burst、窄脉冲能量。

计算量 MEDIUM，平均 O(N)；RAM 4N bytes。

Benefits：控制平方对毛刺的放大。Trade-offs：改变原数据能量定义。

可连接：`Voltage -> RobustRMS -> Result`。

状态：PC_VERIFIED；winsorized 七点真值通过，未实板。
