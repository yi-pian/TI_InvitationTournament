# MODULE CARD

MODULE: FFT Three-point Parabolic Interpolation

CATEGORY: Precision / FFT Frequency

功能：用峰及左右 magnitude 拟合抛物线，估计 fractional bin。

输入：magnitude、peak index、Fs、N；输出 offset/bin/frequency/interpolated magnitude。

是否原地处理：不适用。

依赖：公共状态码。

典型用途：把整数 peak bin 改善为 bin 间频率估计。

不要用于：边界 bin、平顶、非局部最大、多峰重叠严重。

计算量 LOW O(1)，RAM O(1)。

Benefits：只用 3 点、成本极低。Trade-offs：窗型/offset 会产生系统偏差，不能突破 SNR/记录时长限制。

可连接：`Peak + Magnitude -> Parabolic -> frequency_hz`。

状态：PC_VERIFIED；解析抛物线精确，Hann 1037 Hz 实测误差 4.833 Hz（Fs/N=100 Hz）。
