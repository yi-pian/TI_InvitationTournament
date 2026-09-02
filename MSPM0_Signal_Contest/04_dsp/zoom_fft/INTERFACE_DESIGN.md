# Interface Design（不可调用）

计划 config：input_sample_rate_hz、center_frequency_hz、decimation_factor、外部 FIR 系数/状态、FFT size。

计划流水线：`float input -> complex mixer -> FIR lowpass -> decimator -> complex FFT`。所有 output/workspace/state 由调用者提供，必须返回有效输出点数、等效 Fs 和频率起点。
