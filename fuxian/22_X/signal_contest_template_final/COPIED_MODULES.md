# Copied Modules

比赛工程复制模块后，只做这份简单来源记录；不需要维护复杂依赖系统。

| 模块 | 原始路径 | 复制日期 | 本题修改 |
|---|---|---|---|
| 双 ADC 同步采集 | `MSPM0_Signal_Contest/02_acquisition/adc_dual_sync/` | 2026-08-17 | X=ADC0.2/PA25，Y=ADC1.2/PA17，Fs=500 kSPS，N=1024 |
| TFT ILI9341 | `MSPM0_Signal_Contest/01_bsp/tft_ili9341/` | 2026-08-17 | SPI1/PB9/PB8/PB6，DC=PB15，BLK=PB12，横屏李萨如图 |
| 双路同步 ADC 相位测量 | `MSPM0_Signal_Contest/05_precision/dual_adc_phase_measurement/` | 2026-08-17 | 纯算法模块；输入 DMA 完成的 X/Y 原始码，`fY/fX` 使用当前 PLL 倍频数 |
