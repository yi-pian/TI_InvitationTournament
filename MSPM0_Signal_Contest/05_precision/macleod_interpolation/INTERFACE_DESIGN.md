# Interface Design（不可调用）

计划输入：复数 FFT 数组、内部峰索引、Fs/N、窗/缩放元数据。

计划输出：bin_offset、fractional_bin、frequency_hz、quality。边界、非有限值、分母接近零必须显式报错。
