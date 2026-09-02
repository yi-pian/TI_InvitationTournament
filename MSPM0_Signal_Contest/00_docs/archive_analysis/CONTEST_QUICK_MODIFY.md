# Contest quick modify

## 改采样点数 N

- 改应用配置和调用者数组，不改算法 `.c`。
- 检查 `sample_count <= buffer capacity`。
- FFT 的 N 必须是 2 的幂；先查 `FFT_MEMORY_BUDGET.md`。
- CCS Graph/Memory Browser 的长度同步修改。

## 改采样率 Fs

- `SignalADC_SetSampleRate(Fs)` 或初始化配置修改。
- `timer_clock_hz` 必须是 divider/prescaler 后的 Timer 计数时钟。
- `GetConfiguredTriggerRate` 是整数 Timer 推导值，不是物理实测值。
- 所有频率、相位、FFT 调用都传同一语义的 Fs。

## 改 ADC 通道

- 只改 `.syscfg` 的 channel/pin/instance 和必要的 DMA trigger。
- 保持应用使用的生成实例名稳定。
- 检查输入范围、参考电压、源阻抗、采样时间和地连接。
- 不在 `main.c` 使用多层 `../../` include。

## 1024 改 2048/4096 FFT

- 调整 raw、complex 和 magnitude 工作区；看 `.map` 而不是只看源数组。
- 2048 float FFT 可做但要复用缓冲并限制其他 BSS。
- 4096 float complex 单数组已占满 32 KB，当前实现禁止作为板上方案。

## 改窗函数

- 相干单频且频点严格落 bin 可用 Rect。
- 一般频谱和 THD 优先 Hann。
- 幅度必须除以相干增益；不要只换窗、不改幅度标定。

## 改 DDS 频率

- 只调用 `SignalDDS_SetFrequency(output_hz, update_rate_hz)`。
- 输出频率必须小于更新率一半；实际频率由 32 位 tuning word 量化。
- 改更新 Timer 后同步传新的 update rate。

## 改波形/幅度/偏置

- 通过 `sine/square/triangle/sawtooth` 的参数重新生成表。
- `offset_fraction ± amplitude_fraction` 必须落在 0..1。
- 方波占空比必须在 0..1 开区间。

## CCS include path

projectspec 的 `-I` 必须指向真实物理目录，例如 `${PROJECT_ROOT}/../../../../03_measurement/rms`；linked resource 的虚拟 `targetDirectory` 不会自动成为 include search path。改 projectspec 后推荐删除旧导入工程并重新导入，再 Generate/Clean/Rebuild。
