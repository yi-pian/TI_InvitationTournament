# 22_spectrum_display

只把已经计算好的 `fft_magnitude[]`（线性幅度）显示成带自动纵轴的 dB 频谱并标记峰值；它**不执行 FFT**。

复制 `SPECTRUM_DISPLAY`：输入 `fft_magnitude[]` 和 `sample_rate_hz`，输出 TFT 图。横轴左端为 0 Hz、右端为 Nyquist 频率；红线标记最大非 DC 谱峰，顶部显示峰值频率和 dB。FFT、谐波识别和 THD 请继续复用 `20_fft_analysis`，本工程只解决显示。
