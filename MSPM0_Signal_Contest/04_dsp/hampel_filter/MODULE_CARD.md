# MODULE CARD

MODULE: Hampel Filter

CATEGORY: DSP / Signal Conditioning

功能：用局部中位数和 MAD 识别离群点，并用局部中位数替换。

输入：输入/输出数组、奇数 window、sigma 阈值、minimum_scale、workspace。

输出：滤波数组和 `replaced_count`。

是否原地处理：NO。

依赖：MAD。

典型用途：ADC 偶发毛刺、鲁棒 Vpp 前处理。

不要用于：真实脉冲、突发、尖锐边沿和瞬态测量。

计算量：MEDIUM/HIGH，约 O(N·W) 平均，局部选择最坏更高。

RAM：输出 `4N` + workspace `4W` 字节。

Benefits：阈值随局部噪声尺度变化，比固定幅值阈值更自适应。

Trade-offs：非线性；参数不当会删除真实事件；平坦区 MAD=0 需用 minimum_scale 管理。

可连接：`ADC_ToVoltage -> Hampel -> RobustVPP/ZeroCross`。

状态：PC_VERIFIED。2026-08-07 通过孤立 100 倍毛刺及替换计数测试；未实板验证。
