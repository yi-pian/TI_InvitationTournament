# 阶段一：模块选择与 SysConfig

## 1. 选择模块

先按题目数据流选择硬件模块：双 ADC DMA、DAC DMA、DAC 波表、DDS、Comparator、Timer Capture、Trigger Capture、ST7789、4×4 键盘。再选择算法模块：ADC To Voltage、Mean、MinMax、VPP、RMS、AC RMS、Statistics、Remove DC、Zero Cross、Interpolation、Multi Cycle Average、FFT、Window、Peak、Parabolic、Harmonic、THD、SNR、SFDR、Clipping、MAD、Median、Hampel、Robust VPP/RMS、Sine Fit、Lock-In、Arbitrary Wave 和两个 Calibration。

## 2. 复制内容

复制正式模块 README 指定的 `.c/.h` 到 `signal_contest_template_final/modules/`，完整清单见同目录 `COPIED_MODULES.md`。原正式模块没有改动。同步复制 RMS/AC RMS/Statistics 共同依赖的 `signal_math_backend.h` 与 `signal_math_backend_config.h`；ST7789 的显示文字模块改为 `signal_tft_st7789_font.c/.h`，其 ASCII 点阵数据原样复制自 22_X ILI9341 的 `signal_tft_ili9341_font_data.inc`，不新增 SysConfig 资源。

## 3. SysConfig

先复制 `PROFILE_06_FULL_SIGNAL/profile.syscfg` 的硬件底座，再按 README 在图形界面追加 ST7789 SPI1、DC/背光 GPIO 和键盘 GPIO。ADC0/PA25 为 CH1，ADC1/PA17 为 CH2；TIMG0 产生 10 us ADC 事件；DAC12 + DMA_CH1 + TIMG6 产生 OUT；COMP0/PA27 接 Timer Capture 的事件输入；TIMG7 做捕获；键盘行列按 README 固定引脚配置。

这一步与模块 README 一致，没有改变正式模块要求；不同点只有本题把多个 README 的资源合并进一个 profile，并为 ST7789 增加文字辅助模块。生成后要核对实例宏、DMA 通道、IRQ 和 Timer LOAD，不能手工改生成的 `ti_msp_dl_config.c/.h`。

## 4. main 初始化区复制与自写代码

复制的初始化调用是：`SYSCFG_DL_init()`、`SignalDualADC_Init()`、`SignalDACDMA_MSPM0_Init()`、`SignalTimerCapture_MSPM0_Init()`、`SignalTimerCapture_MSPM0_Start()`、`SignalTFTST7789_MSPM0_Init()`。

自写的组合代码如下：

```c
SYSCFG_DL_init();
g_module_status = SignalDualADC_Init(&adc_config);
if (g_module_status != SIGNAL_RESULT_OK) while (1) { }
```

逐行解释：

1. `SYSCFG_DL_init()` 执行 SysConfig 生成的 GPIO、ADC、DMA、Timer、SPI 和 DAC 初始化。
2. `SignalDualADC_Init(&adc_config)` 把题目需要的采样率、Timer 时钟和最大计数传给双 ADC 模块。
3. `g_module_status` 保存返回状态，便于后面判断失败原因。
4. 初始化失败时停在死循环，避免未初始化外设继续运行。

## 5. 验证边界

本阶段只完成源码和 `.syscfg` 组合，未执行 CCS Generate/Build，未烧录实板，状态为 `BOARD NOT RUN`。
