# MODULE CARD

MODULE: Radix-2 FFT

CATEGORY: DSP / Spectrum

功能：未归一化前向复 FFT；提供实数输入简单接口和复数原地省 RAM 接口。

输入：N 点 float 或 complex，N 为 2 次幂。

输出：N 点 complex DFT，`exp(-j...)` 约定。

是否原地处理：复数接口 YES；实数简单接口把数据复制到复数输出。

依赖：`sinf/cosf`、complex 公共类型。

典型用途：频率、幅值、谐波、THD、SNR/SFDR 基础。

不要用于：N 非 2 次幂、RAM 未预算、把最大 bin 当精确频率/物理幅值。

计算量：O(N log2N)；每级只算一组旋转步进，避免大 twiddle 表。

RAM：算法内部 O(1)；复数 buffer `8N` 字节。

Benefits：标准 C、无动态内存、无大查表、模块拆分清楚。

Trade-offs：M0+ 软件浮点；旋转递推有小漂移；实数输入未利用实 FFT 对称性，RAM/CPU 可继续优化。

可连接：`Window -> FFT -> Magnitude`。

状态：PC_VERIFIED。2026-08-07 冲激全 bin、精确/非整 bin 正弦完整链通过；未实板计时。
