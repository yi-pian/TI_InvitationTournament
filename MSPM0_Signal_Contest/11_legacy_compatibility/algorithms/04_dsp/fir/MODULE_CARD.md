# MODULE CARD

MODULE: FIR

CATEGORY: DSP / Linear Filter

功能：使用调用者提供的任意 FIR 系数和跨块延迟线做卷积。

输入：系数、tap_count、delay_line、输入块。

输出：滤波块，单位由系数增益决定。

是否原地处理：YES。

依赖：公共算法状态码。

典型用途：可控低通/高通/带通、对称系数线性相位滤波。

不要用于：未验证系数/采样率；CPU 预算不足的高阶实时链；THD 前滤掉待测谐波。

计算量：MEDIUM/HIGH，O(N·T)。

RAM：状态 `4*T` 字节；输出可原地。

Benefits：外部系数不写死截止频率；状态跨帧；对称系数可实现线性相位。

Trade-offs：陡峭响应通常需要较多 taps；固定群延迟；M0+ 软件浮点乘加较贵。

可连接：`ADC_ToVoltage -> FIR -> measurement/decimation`。

状态：PC_VERIFIED。2026-08-07 通过 3-tap 跨两块冲激响应测试；未实板验证。
