# MODULE CARD

MODULE: Harmonic Analyzer

CATEGORY: DSP / Harmonics

功能：从已知基波频率定位 1~10 阶谐波并按半径积分能量。

输入：magnitude、Fs、N、f0、阶数、radius；输出每阶频率/bin/range/energy。

是否原地处理：不适用。

依赖：Multi-bin Energy。

典型用途：Fundamental/H2~H5/THD。

不要用于：把 `2*k` 代替真实频率定位、超 Nyquist、积分窗口重叠。

计算量 LOW，O(H·R)，RAM 固定约 11 个 item。

Benefits：使用 `h*f0*N/Fs`，支持非整 bin；BASIC/COMPETITION 由 radius 明确切换。

Trade-offs：依赖可靠 f0；频率响应/窗/噪声仍会影响各阶能量。

可连接：`Magnitude + interpolated f0 -> Harmonic -> THD`。

状态：PC_VERIFIED。相干 BASIC 和 Hann 非整 bin COMPETITION 均通过。

24_C 用法：H1~H5 只按 FFT 插值基波定位，Hann 后从 radius_bins=2 起做邻近 bin 能量积分；Timer Capture 频率不能覆盖 fundamental_frequency_hz，H5 必须低于 Nyquist。
