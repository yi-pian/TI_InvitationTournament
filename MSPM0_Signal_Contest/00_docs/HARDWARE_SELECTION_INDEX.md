# Hardware Selection Index

本索引用于题目目标→内部/外部硬件路线→最接近 SysConfig Profile 的快速定位。具体性能、Pin、寄存器和电气参数必须继续查模块 README、目标 `.syscfg`、生成 header 和本地官方资料。

```text
rg -n -i "ADC DMA|高吞吐|双通道|DAC|DDS|TFT|键盘|弱信号|比较器|VGA|外部 ADC" MSPM0_Signal_Contest/00_docs/HARDWARE_SELECTION_INDEX.md
```

| 题目/搜索词 | 默认候选 | Profile | 选择门与不能猜的内容 |
|---|---|---|---|
| 单通道 N 点 ADC DMA | `02_acquisition/adc_dma` | P01 | ADC pin/reference/sample time、Timer/Event/DMA 名以 `.syscfg` 为准 |
| ADC FIFO 高吞吐 | `02_acquisition/adc_fifo_dma` | P08 | FIFO 打包、DMA width/count 不能沿用普通 ADC DMA |
| 双通道同步/相位 | `02_acquisition/adc_dual_sync` | P02 | ADC0/1、DMA0/1、Event 与 channel delay owner |
| ADC + DAC 环回/闭环 | ADC DMA + DAC DMA | P04 | PA15→PA25 仅在电压安全和共地后接；当前 Board `NOT_RUN` |
| 完整双 ADC + DAC + Capture | 按需裁剪 P06 | P06 | 不因方便就占用资源超集；先画 owner 表 |
| 方波/整形后高精度频率 | Comparator + Timer Capture | P05 | 阈值、迟滞、传播延迟、Capture clock/overflow |
| 波表/Software DDS 输出 | `06_generator/{dds,dac_dma}` | P03 | DAC 更新率、点/周期、settling、滤波、幅值边界 |
| 片外 DDS | exact AD9833/AD9850 driver | 自建最小配置 | exact part、供电/logic、接口/时序、reset；不把 software DDS 混同 |
| TFT 彩屏 | ILI9341 正式 driver | Example 自带 `.syscfg` | SPI mode/bitrate、CS/DC/BL/RESET、电源与背光类型 |
| OLED/SSD1306 | exact SSD1306 正式 Driver | 独立复制 Profile | 当前源码/复制链 `BUILD_VERIFIED`，实屏 `NOT_RUN`；生成调用前读 `ssd1306.h`，确认 exact controller、I2C 地址/分辨率/电平 |
| 4×4 键盘 | matrix keypad + MSPM0 platform | 无固定 Profile | 实测 R/C 线序；8 GPIO、上拉、active-low 与鬼键 |
| 弱信号 | 低噪前端/VGA + ADC + LockIn Recipe | 依硬件 | 输入范围/噪声/GBW/SR/量程与 VGA 标定 |
| 自动量程/AGC | PGA/VGA/开关 + 自动控制 Recipe | 依 exact device | 控制方向、settling、LUT、滞回与资源 owner |
| 超出片内 ADC/DAC 性能 | `12_external_devices/adc|dac` exact driver | 自建最小配置 | exact datasheet、供电、logic、SPI/I2C mode、DRDY/CONV 时序 |

## Profile 资源摘要

| Profile | 资源事实 | 证据 |
|---|---|---|
| P01 | ADC0.2 PA25, DMA0, TIMG0, Event1 | `09_examples/integration_profiles/PROFILE_01_ADC_CAPTURE` |
| P02 | ADC0.2 PA25 + ADC1.2 PA17, DMA0/1, TIMG0, Event1/2 | `PROFILE_02_DUAL_ADC` |
| P03 | DAC0 PA15, DMA1, TIMG6, Event3 | `PROFILE_03_DAC_GENERATOR` |
| P04 | P01 + P03 独立链 | `PROFILE_04_ADC_DAC` |
| P05 | COMP0 PA27, Event4, TIMG6 Capture | `PROFILE_05_FREQUENCY` |
| P06 | 双 ADC + DAC + Capture + UART 资源超集 | `PROFILE_06_FULL_SIGNAL` |
| P07 | ADC0.2 PA25 + DAC0 PA15 + UART0 + PA12 | `PROFILE_07_BASIC_IO` |
| P08 | ADC0.2 PA25 + FIFO + 32-bit DMA0 | `PROFILE_08_ADC_FIFO_MAX` |

组合前使用 [RESOURCE_CONFLICT_GUIDE.md](RESOURCE_CONFLICT_GUIDE.md) 建立 GPIO/ADC/DAC/SPI/UART/DMA/Timer/Event/IRQ owner 表；Board Pin 特殊用途以本地 LP 官方资料和现有 `.syscfg` 为准。
