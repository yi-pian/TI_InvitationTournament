# MODULE CARD: fft_peak

| 项目 | 内容 |
|---|---|
| 路径 | `MSPM0_Signal_Contest/04_dsp/fft_peak` |
| 类型 | 正式 Spectrum helper；复用 PeakDetect 的 clean reimplementation |
| 输入/输出 | magnitude + search bins + Fs/N → discrete peak + Hz |
| 复杂度 | O(K)，O(1) RAM |
| 主 API | `SignalFFTPeak_Process` |
| 当前验证 | `BUILD_VERIFIED`，Board `NOT_RUN`；见 `VERIFICATION.yaml` |
