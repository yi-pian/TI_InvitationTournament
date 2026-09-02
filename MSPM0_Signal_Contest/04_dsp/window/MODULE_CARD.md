# MODULE CARD

MODULE: Window Dispatcher

CATEGORY: DSP / FFT Preparation

功能：应用 Rectangular/Hann/Hamming/Blackman，并返回实际相干增益与功率增益。

输入：float 数组、count、window type。

输出：加窗数组、coherent_gain、power_gain。

是否原地处理：YES。

依赖：`cosf`、公共状态码。

典型用途：FFT 前控制截断边界造成的频谱泄漏。

不要用于：非 FFT 测量却没有明确目的；以为加窗能提升真实分辨率。

计算量：LOW/MEDIUM，O(N)，非矩形每点 1~2 次 `cosf`。

RAM：O(1)，不保存系数表。

Benefits：RAM 小、同一实现统一增益定义。

Trade-offs：运行计算窗系数较慢；主瓣变宽；幅值必须修正。

可连接：`RemoveDC -> Window -> FFT`。

状态：PC_VERIFIED。2026-08-07 四种窗的端点/相干增益与完整 FFT 链测试通过。
