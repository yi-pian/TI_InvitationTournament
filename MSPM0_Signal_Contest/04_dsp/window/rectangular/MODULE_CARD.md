# MODULE CARD

RECOMMENDED_LEVEL: INTERNAL_CHILD_ALIAS；用户应调用父级 Window Dispatcher。

MODULE: Rectangular Window

CATEGORY: DSP / Window

功能：系数全 1，保持原样；输入/输出 float，支持原地；依赖 Window Dispatcher。

输入：float samples、count；输出：不变 float、coherent_gain=1。

主瓣趋势：四种窗中最窄。旁瓣趋势：最高，非相干时泄漏最严重。相干增益：1。

Benefits：不衰减、分辨相邻频率最好。Trade-offs：边界不连续时弱谱线易被强泄漏淹没。

典型用途：严格相干采样。不要用于：未知非整 bin 频率且关心低泄漏。

计算量 LOW；RAM O(1)。可连接：`RemoveDC -> Rectangular -> FFT`。

状态：PC_VERIFIED（端点、增益、FFT 冲激链）；未实板。
