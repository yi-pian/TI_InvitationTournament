# MODULE CARD

MODULE: Sine Fit 3-Parameter

CATEGORY: Precision / Sine Estimation

功能：已知频率，最小二乘估计正弦幅值、相位、DC 与残差。

输入：float V、count、Hz/Fs；输出 V、deg/rad、residual V。

是否原地处理：NO，输入只读。

依赖：math、公共状态码。

典型用途：单音正弦精确幅相测量。

不要用于：严重失真、多音、频漂或错误频率。

计算量 MEDIUM O(N)，RAM O(1)。

Benefits：利用全帧且无需相干采样。Trade-offs：软件浮点三角函数与模型依赖。

可连接：`Frequency -> SineFit3 -> Amplitude/Phase`。

状态：PC_VERIFIED；1234.5 Hz 合成真值通过，未实板。
