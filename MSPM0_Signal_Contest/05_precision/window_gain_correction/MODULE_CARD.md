# MODULE CARD

RECOMMENDED_LEVEL: LEVEL_B_SIMPLE_HELPER；无状态单函数，保留以统一单边谱边界标度。

MODULE: Window Gain Correction

CATEGORY: Precision / FFT Amplitude

功能：raw DFT magnitude -> 单边峰值幅度，处理 N、CG、DC/Nyquist x2 例外。

输入：N/2+1 raw magnitude、N、coherent_gain；输出同长度 peak amplitude。

是否原地处理：YES。

依赖：公共状态码。

典型用途：FFT 单音物理幅值恢复。

不要用于：把宽带噪声直接按单音 peak 解释；泄漏严重却只读一个 bin。

计算量 LOW O(N/2)，RAM O(1) 可原地。

Benefits：避免硬写 N/2 和 0.5。Trade-offs：只解决标度，不解决 scalloping/leakage。

可连接：`Window(CG)+Magnitude -> GainCorrection -> amplitude spectrum`。

状态：PC_VERIFIED；解析 raw magnitude 与 Hann 0.5 Vpeak 链通过。
