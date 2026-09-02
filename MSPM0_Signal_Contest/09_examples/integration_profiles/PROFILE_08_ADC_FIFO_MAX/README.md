# PROFILE_08_ADC_FIFO_MAX

PA25/ADC0.2 的 12-bit 单通道满吞吐率单帧采集基线。ADC 使用 SYSOSC、自动连续转换和软件启动；FIFO 每个 32-bit word 打包两个结果，DMA_CH0 把 FIFO 搬到 RAM。它不占 Timer 和 Event Fabric。

- 默认资源：ADC0、ADC0.2/PA25、DMA_CH0、ADC0 IRQ。
- 默认 ADC sample time：62.5 ns；SysConfig 显示的 conversion period 约 250 ns，对应名义 4 MSPS。
- 固定命名：`SIGNAL_ADC_FIFO`、`SIGNAL_ADC_FIFO_DMA`。
- 可直接适配：`02_acquisition/adc_fifo_dma/`。
- 来源依据：TI SDK 2.11.00.07 官方 `adc12_max_freq_dma` 例程的 12-bit FIFO/DMA 配置，输入通道改为本库常用 PA25/ADC0.2。
- 状态：SysConfig / TI Arm Clang compile / full link PASS；Board `NOT_RUN`。

这套配置追求连续转换吞吐率，不提供任意精确 Fs。若题目要求“严格 1 MSPS、500 kSPS”等定时采样，应选择 `PROFILE_01_ADC_CAPTURE` 与 `adc_dma`。
