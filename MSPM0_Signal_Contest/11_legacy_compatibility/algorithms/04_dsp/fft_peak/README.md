# FFT Peak：离散频谱主峰

这是源码丢失后的 clean reimplementation。它通过正式 `PeakDetect` 找峰，只额外把 bin 换成 Hz，不复制第二套最大值算法。

```text
FFT → Magnitude → FFT Peak → bin / peak_value / frequency_hz
```

## 加入和调用

链接 `signal_fft_peak.c/.h`，同时链接 `04_dsp/peak_detect/signal_peak_detect.c`。Include Path 加两个模块目录及 `03_measurement/common`；无需 SysConfig。

```c
signal_fft_peak_result_t r;
SignalFFTPeak_Process(magnitude, magnitude_count,
                      1U, magnitude_count - 1U,
                      FS_HZ, FFT_N, &r);
```

把 `first_bin=1` 可排除 DC；也可以按题目频率范围换算搜索起止 bin。并列最大值返回第一次出现的位置。该结果仍是整数 bin，频率分辨率为 Fs/N；需要亚 bin 精度时再接 Parabolic/Jacobsen/Quinn/Macleod。输入必须是非负有限 magnitude，搜索区间必须在数组内。详见 [REIMPLEMENTATION_SPEC](REIMPLEMENTATION_SPEC.md)。
