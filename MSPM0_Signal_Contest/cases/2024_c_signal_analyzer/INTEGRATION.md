# 2024 C Integration Record

## Source and SysConfig

正式工程位于 `fuxian/24_C/signal_contest_template_final/signal_contest_template_final`，CCS 入口位于 `fuxian/24_C/ticlang/`。母版的损坏 SysConfig 被恢复为 ADC0/ADC1、TIMG0 采样事件、DMA_CH0/CH1、TIMG6 capture、SPI1 TFT 和必要 GPIO 的单一配置源；生成文件不手工修改。

## Copied modules

从 `MSPM0_Signal_Contest` 复制或按其 README 适配了 `adc_dual_sync`、`adc_timer_trigger`、`timer_capture`、`tft_ili9341`、`tft_waveform`、`harmonic` 和 `multi_bin_energy`。工程还保留 `signal_remove_dc` 作为兼容实现。`signal_config.h` 集中保存 Fs、FFT 点数、VREF、输入增益和显示参数。

## Application glue

`main.c` 自己编写了状态机、三缓冲事件拼接、ADC0 活动区间检测、波形分类、Hann/Q15 FFT 调用、H1~H5 选择、硬件/FFT 频率分支、三周期截取和 TFT dirty-field 刷新。DMA ISR 不调用 FFT、不绘图、不格式化浮点字符串。

## Runtime sequence

```text
SYSCFG_DL_init
  -> TFT static screen once
  -> timer capture start
  -> synchronized ADC0/ADC1 DMA start
  -> DMA IRQ switches next buffer
  -> main consumes completed pair
  -> remove DC, statistics, FFT/classification
  -> choose timebase and render waveform/numeric dirty fields
  -> if burst locked, stop DMA and keep the full event on screen
```

## Rebuild rule

修改 Pin、外设实例、事件、DMA 或时钟时只改 `.syscfg` 后重新 Generate；修改算法或布局后重新 Clean/Build。不要编辑 `Debug/` 下的自动生成 makefile 或 `ti_msp_dl_config.*`。
