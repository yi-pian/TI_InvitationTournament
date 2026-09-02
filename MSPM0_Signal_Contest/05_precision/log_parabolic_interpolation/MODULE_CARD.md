# MODULE CARD

MODULE: Log Parabolic Interpolation

CATEGORY: Precision / FFT Peak Interpolation

功能：对数 magnitude 三点抛物线估计分数 bin、Hz 和峰值。

输入：三个正幅值所在数组、peak bin、Fs/N；输出 offset、fractional bin、Hz。

是否原地处理：不修改输入。

依赖：math、公共状态码。

典型用途：单音 FFT 峰值频率精修。

不要用于：低 SNR、重叠峰、平顶、边界峰或非正 magnitude。

计算量 LOW O(1)，RAM O(1)，但含 logf/expf。

Benefits：整数 bin 间估计。Trade-offs：窗和噪声相关的系统偏差。

可连接：`PeakDetect -> LogParabolic -> Frequency`。

状态：PC_VERIFIED（解析 log-parabola 真值）；实际窗函数需场景验证。
