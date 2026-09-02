# 24_A-01 模块选择与 SysConfig

先把题目拆成 Q1 信号源、Q2 扫频带宽、Q3 压摆率、Q4 静态功耗、ST7789 显示和矩阵键盘六部分，再按 `COPIED_MODULES.md` 从集成库复制模块。

SysConfig 按原 24_A 和模块 README 配置：ADC0 CH2/PA25 + DMA_CH0 为 `SIGNAL_ADC`；ADC1 CH2/PA17 为 `POWER_ADC`；DAC0/PA15；TIMG0 为 `SIGNAL_SAMPLE_TIMER`；AD9850 用 PA12/PA13/PA28/PA31；键盘用 PB16/PB0/PB7/PB17 和 PB18/PB13/PB20/PB4；ST7789 用 SPI1 PB9/PB8、CS PB6、DC PB15、BL PB12。与 README 不同之处只有题目所需采样率由 main 动态设置，实例名和事件链不变。

自己写的代码只有题目状态变量。`g_app_mode` 保存 Q1-Q4；`g_measurement_requested` 表示按键请求一次测量；`g_tft_dirty` 表示数值区需要刷新。三行变量分别解决“当前做哪问”“是否启动耗时测量”“是否更新屏幕”。
