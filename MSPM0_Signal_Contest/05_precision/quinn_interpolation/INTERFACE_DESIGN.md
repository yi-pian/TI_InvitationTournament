# Interface Design（不可调用）

计划输入：复数 FFT、峰索引、Fs/N、估计器版本枚举、允许的 SNR/质量门限。

计划输出：fractional bin、frequency_hz、quality 和失败状态。不得只接 magnitude；必须记录 FFT 采用 `exp(-j2πkn/N)` 约定。
