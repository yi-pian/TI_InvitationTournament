# 步骤02：SysConfig 配置

先双击 `signal_contest_template.syscfg`，按 README 的 GUI 路径添加 ADC12、DAC12、TIMER、GPIO、SPI。ADC A=`ADC0/PA25/DMA_CH0`，ADC B=`ADC1/PA17/DMA_CH2`；ADC 定时器=`TIMG0/2 us`，两个 ZERO event channel 1、2。DAC=`DAC0/PA15`，FIFO=`TWO_QTRS_EMPTY`，trigger=`HWTRIG0`，DMA=`DMA_CH1`，transfer=`FULL_CH_REPEAT_SINGLE`，source/destination=`HALF_WORD`；DAC 定时器=`TIMG6/2 us`，publisher channel 3。显示和键盘沿用母版 SPI1/PB9/PB8/PB6、PB15/PB12 和 PB 行列脚。

与 README 一致的部分：DAC FIFO、DMA、Timer/Event 闭环；ADC 由 Timer event 触发；纯算法模块不新增外设。与 README 的差异：ADC B 改用 DMA_CH2，避免与 DAC DMA_CH1 冲突；ADC 采样率保持 500 kS/s，DAC 在 `main.c` 通过 README 的 `SignalDACDMA_MSPM0_SetUpdateRate` 按当前频点动态设为频率的 50 倍（0.5 kHz～10 kHz 时为 25 kS/s～500 kS/s），因此本次题目参数调整不需要新增或改动 SysConfig 字段。Generate 后只核对生成宏，不手改 `ti_msp_dl_config.*`。
