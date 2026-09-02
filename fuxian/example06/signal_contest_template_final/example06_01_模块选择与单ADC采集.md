# example06-01：模块选择与单 ADC/DMA 采集

## 比赛动作

1. 先打开 `MSPM0_Signal_Contest/02_acquisition/adc_dma/README.md`，确认本题一路混合输入使用
   `signal_adc_dma.c/.h`，而不是双 ADC 模块。
2. 原样复制这两个文件和公共依赖 `01_bsp/common/signal_status.h` 到 `modules/`。
3. 按 README 在 SysConfig 增加 `SIGNAL_ADC`、`SIGNAL_ADC_DMA` 和 `SIGNAL_SAMPLE_TIMER`。
   本题 Fs=500 kS/s，N=1024，TIMER 周期 2 us。

SysConfig 使用 README 的字段和示例资源：ADC0/PA25/通道2、DMA_CH0、TIMG0、Event Channel 1。
组合 README 只补充了“单 ADC 与显示/键盘共存”的顺序；没有改模块 README 的接口。

## main 复制与自写内容

- 复制 README 的配置结构、`SignalADC_Init()`、`SetSampleRate()`、`Start()`、
  `IsFinished()` 调用顺序（`main.c:238-286`）。
- `main.c` 的静态 `g_raw[]` 缓冲区是应用少量逻辑，DMA 完成前不读取；它保存一路混合输入。
- `App_Fail()` 是错误收敛逻辑；不在 DMA ISR 中计算或刷新屏幕。

## 逐段解释

- 238-245：给模块传入采样率、Timer 实际时钟和 16 位 Timer 最大计数。
- 248-249：执行 SysConfig 生成的硬件初始化。
- 257-258：初始化模块并打开 DMA 完成处理。
- 275-280：每帧设置采样率并启动单路 DMA。
- 282-286：只等待模块完成标志；完成前不碰采样数组。
- 288-289：完成后才把 `g_raw[]` 交给应用测量。

## 验收

SysConfig CLI 生成成功；生成头文件包含单路 ADC、DMA 和 TIMER 宏。模块 `.c/.h` SHA-256
与来源一致，未修改。
