# 阶段七：Sine Fit 与 Lock-In

## 1. Sine Fit

复制 `sine_fit_3param` 和 `sine_fit_4param`。main 先用 FFT 插值频率作粗初值，再调用 3 参数和 4 参数拟合：

```c
signal_sine_fit_3param_config_t fit3_config = {
    g_fft_interpolated.frequency_hz,
    (float)g_effective_sample_rate_hz
};
SignalSineFit3Param_Process(g_calibrated, SIGNAL_SAMPLE_COUNT,
    &fit3_config, &g_fit3);
SignalSineFit4Param_Process(g_calibrated, SIGNAL_SAMPLE_COUNT,
    &fit4_config, &g_fit4);
```

逐行解释：配置结构体第一项是 FFT 得到的近似频率，第二项是实际采样率；3 参数拟合在已知频率下求余弦系数、正弦系数和 DC；4 参数拟合以该频率为中心搜索窄频带，同时精修频率、幅值、相位和 DC。CH2 使用同一粗频率再拟合一次，`g_channel_phase_deg = g_fit3_ch2.phase_deg - g_fit3.phase_deg` 得到未校准的 B-A 相位；随后 `SignalChannelDelayCalibration_Apply` 把已有固定延迟换回相位并写入 `g_corrected_phase_deg`，CAL 页面显示该补偿值。

## 2. Lock-In

复制 `lock_in`。配置使用当前 DDS 输出频率和实际采样率：

```c
signal_lock_in_config_t lock_config = {
    (float)g_output_frequency_hz,
    (float)g_effective_sample_rate_hz, 0.0f, 1U
};
SignalLockIn_Process(g_calibrated, SIGNAL_SAMPLE_COUNT,
    &lock_config, &g_lock_in);
```

第一、二行给出参考频率和采样率；第三行规定参考相位和是否去 DC；处理函数对输入与正交参考相乘并平均，输出 I、Q、幅值和相位。只有 OUT 的 DDS 频率确实等于输入激励频率时，Lock-In 结果才有物理意义。

## 3. README 差异和验证

调用顺序和公式全部由模块完成；自写代码只负责把 FFT 结果传给 Fit、把 DDS 频率传给 Lock-In、把 CH1/CH2 两次拟合结果相减。当前未用示波器验证拟合误差和微弱信号噪声底。
