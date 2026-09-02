# Contest Parameter Calculation

## 1. 选择 Fs

Nyquist 只给理论下限：`Fs > 2 fmax`。实际测量推荐从 `Fs = 5~10 fmax` 起步；相位、过零和波形形状常取 `10~20 fmax`。前端抗混叠滤波器必须在 `Fs/2` 前衰减带外信号。

例：测 1~10 kHz 正弦频率，过零法希望每周期 ≥10 点，可选 `Fs=100 kS/s`。若测 10 kHz 信号的 H5，最高有效分量为 50 kHz，则 `Fs` 必须明显高于 100 kS/s，不能仍用 100 kS/s。

## 2. 选择 N 与时间

- 记录时间：`Trecord = N / Fs`。
- 每周期点数：`samples_per_cycle = Fs / f`。
- 覆盖周期数：`cycles = N f / Fs`。
- FFT bin 间隔：`Δf = Fs / N`。

例：`Fs=100 kS/s, N=1024`，记录 10.24 ms，bin 间隔 97.65625 Hz；1 kHz 约覆盖 10.24 周期。Parabolic interpolation 能细化峰位置，但不能把两条间距远小于窗主瓣的谱线可靠分开。

## 3. 相干采样

相干条件为 `f_in = k Fs / N`，其中 `k` 与 `N` 最好互质以遍历更多码型。不能控制输入频率时使用 Hann 等窗口；能控制 DDS 扫频时优先把每点设为相干频率，减少泄漏。

## 4. 窗函数

| Window | 适用 | 代价 |
|---|---|---|
| Rectangular | 相干采样、最高频率分辨率 | 非相干泄漏大 |
| Hann | 通用频谱、频率/幅值 | 主瓣变宽；必须做 coherent gain correction |
| Hamming | 较低第一旁瓣 | 幅值同样需校正 |
| Blackman | 强旁瓣抑制 | 主瓣更宽、近邻谱线分辨率下降 |

## 5. RAM 预算

逐数组计算，不只看 FFT 本体：

```text
raw ADC             = 2N
voltage float       = 4N
complex float FFT   = 8N
one-sided magnitude ≈ 2N + 4
two-channel raw     = 4N
two-channel voltage = 8N
two-channel FFT     = 16N
```

单通道 Simple FFT 基础约 `16N` B，另加状态、DriverLib、栈、UART/UI。N=1024 的真实 Spectrum 链接为 17,045 B SRAM。N=2048 的 Frequency C 完整应用已因 `.bss=0x8010` 超 32 KiB 失败；4096 标记 `UNSUPPORTED_BY_CURRENT_SIMPLE_FFT_API`。

## 6. 响应与 CPU

每帧 deadline 近似 `N/Fs`。算法最坏执行时间必须小于允许处理间隔；当前没有板上 cycle 数据时只能写 `PENDING_BOARD`。相关法约随 `N × lag_range` 增长，Hampel/分位数含排序，扫频总时长约：

`Tsweep = points × (settling_time + N/Fs + processing_time)`。

例：20 点、每点 settling 10 ms、N=1000、Fs=100 kS/s，不计计算开销也需 `20 × (10+10) ms = 0.4 s`。

## 7. DMA Buffer

ADC raw buffer 为 `2N` B。RingBuffer 实现用空槽区分满/空，要保存 N 个样本必须分配 N+1。双缓冲/保留原帧会再乘 2；所有大 Buffer 必须在 `.map` 中核对。

## 8. DDS

- 相位累加器步进：`phase_step ≈ round(fout / Fupdate × 2^32)`。
- 理论频率分辨率：`ΔfDDS = Fupdate / 2^32`。
- 实际配置频率由量化后的 step 反算，应用应读取 `SignalDDS_GetConfiguredFrequency()`。
- 每周期更新点数：`Fupdate / fout`，建议 ≥20~50。

例：`Fupdate=100 kS/s, fout=1 kHz`，每周期 100 点，理论 step 分辨率约 0.0000233 Hz；整机准确度仍受 Timer clock、DAC 与模拟滤波限制。

## 9. 参数落点

Fs、N、VREF、频率范围、窗口、触发电平、DDS/扫频参数改 `signal_config.h`；Profile/功能改 `signal_features.h`。ADC/DAC 引脚、Timer 实例、DMA channel、Event route、Comparator 阈值硬件路径必须改 `.syscfg` 并重新 generate。
