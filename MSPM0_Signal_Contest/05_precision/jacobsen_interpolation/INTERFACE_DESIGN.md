# Interface Design（不可调用）

计划输入：`const signal_complex_f32_t *spectrum`、`bin_count`、内部 `peak_index`、`sample_rate_hz`、`fft_size`、窗函数标识。

计划输出：`bin_offset`、`fractional_bin`、`frequency_hz`、质量/有效标志。

接口必须拒绝 DC/Nyquist 边界、非局部峰、非有限值，并把相位/FFT 正负号约定写进头文件。
