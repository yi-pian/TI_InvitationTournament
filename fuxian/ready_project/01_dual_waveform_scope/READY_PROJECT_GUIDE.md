# 01 双通道波形显示器使用与复用说明

## 1. 工程作用

本工程是一个轻量双通道示波器模板。烧录后连接两路模拟信号，即可在 ST7789 TFT 上观察 CH1、CH2、双通道叠加波形以及 XY/Lissajous 图形。

适合比赛现场快速完成：

- 两路波形是否正常的直观检查；
- 两路幅值、周期和相位关系的初步判断；
- 放大、滤波、移相等电路的输入/输出对照；
- 从本工程继续删减为赛题专用双通道显示界面。

## 2. 默认接口与参数

| 项目 | 默认设置 |
|---|---|
| CH1 | ADC PA25 |
| CH2 | ADC PA17 |
| 显示 | ST7789，横屏 320×240 |
| 键盘 | 4×4 矩阵键盘 |
| 采样点数 | `SAMPLE_COUNT = 512` |
| 请求采样率 | 100 kSa/s |
| ADC 参考电压 | 3.3 V |

输入信号必须与开发板共地，并限制在 ADC 允许范围内。高于 3.3 V、低于地或带较大负压的信号必须先经过衰减、偏置或保护电路。

## 3. 页面与按键

| 按键 | 功能 |
|---|---|
| A | 上一页面 |
| B | 下一页面 |
| C | 减少显示周期数，最少 1 周期 |
| D | 增加显示周期数，最多 10 周期 |

页面包括：

- `CH1`：只显示 CH1；
- `CH2`：只显示 CH2；
- `DUAL`：两路波形叠加；
- `XY`：CH1 映射到 X 轴、CH2 映射到 Y 轴。

屏幕还显示实际采样率、当前时间跨度、显示周期数以及 CH1/CH2 的 V/div。

## 4. 运行数据流

```text
同步双 ADC DMA 采集
  -> 每路各执行一次 ADC code 转电压
  -> 每路各执行一次去直流与自动量程
  -> 过零估计频率，只用于选择显示窗口
  -> 按屏幕宽度抽点
  -> 局部清除波形框和数值区域
  -> 绘制当前页面
```

同一帧的电压转换与去直流各通道只执行一次。CH1、CH2、DUAL 和 XY 页面复用相同的 `centered_ch1_samples`、`centered_ch2_samples`，不会因为切换页面重新采集另一帧。

## 5. 复用的 fuyong 内容

| 来源 | 当前函数/能力 | 类型 | 用途 |
|---|---|---|---|
| `04_dual_adc_dma` | `AcquireDualADCFrame()` | `FUYONG_COPY` | 同一 Timer 事件触发两路 ADC 和 DMA |
| `30_basic_measurement` | `ConvertADCToVoltage()` | `FUYONG_ADAPTED` | 参数化完成任一路 code→V |
| `20_fft_analysis` | `PrepareSignal()` 的去 DC 步骤 | `FUYONG_ADAPTED` | 生成 centered 数据 |
| `24_auto_range` | 自动半量程计算 | `FUYONG_ADAPTED` | CH1/CH2 独立纵轴量程 |
| `11_zero_cross_frequency` | `MeasureFrequencyZeroCross()` | `FUYONG_ADAPTED` | 估计周期并选择显示窗口 |
| `21_time_domain_waveform` | `DrawTimeDomainWaveform()` | `FUYONG_ADAPTED` | 屏宽抽点和时域折线 |
| `21_waveform_display` | 双波形/XY 显示思想 | `READY_PROJECT_LOCAL` 组合 | 页面组合 |
| `70_keypad_usage`、`moni01` | 5 ms 扫描和 8 项按键队列 | `FUYONG_ADAPTED` | ISR 入队、主循环处理 |
| `80_tft_usage`、`moni01` | 静态 UI + 动态区域局部刷新 | `FUYONG_ADAPTED` | 避免整屏闪烁 |

## 6. 如何使用

1. 在 CCS 中导入本工程并执行 SysConfig Generate、Clean、Build。
2. 将 CH1、CH2 信号连接到对应 ADC 输入，并连接公共地。
3. 烧录后默认进入 DUAL 页面。
4. 用 A/B 切换页面，用 C/D 调整希望观察的周期数。
5. 查看 `Fs` 和 `Span`，确认当前采样率和时间窗口适合输入频率。
6. 如果波形顶到边框，先检查输入是否超出 ADC 范围；自动量程只改变显示比例，不提供硬件保护。

## 7. 如何复用到其他工程

### 7.1 只复用双 ADC 采集和电压数据

从本工程 `main.c` 复制以下 static 函数和对应变量：

- `AcquireDualADCFrame()`；
- `ConvertADCToVoltage()`；
- `PrepareSignal()`；
- `adc_ch1_samples`、`adc_ch2_samples`；
- `voltage_ch1_samples`、`voltage_ch2_samples`；
- `centered_ch1_samples`、`centered_ch2_samples`；
- `SAMPLE_COUNT`、`sample_rate_hz`。

目标工程必须同时具备本工程的双 ADC/DMA 模块和对应 SysConfig 实例。模块文件应从已验证工程原样复制，不能在目标工程内修改。

### 7.2 复用时域波形显示

再复制：

- `VisibleSampleCount()`；
- `MapY()`；
- `DrawTimeDomainWaveform()`；
- `DrawStaticUi()`；
- `UpdateDisplay()` 中波形框局部刷新部分。

目标工程已有测量结果时，应直接把现有 centered 数组传给绘图函数，不要重新进行 code→V 或去 DC。

### 7.3 复用 XY 页面

复制 `DrawXY()` 和 `PAGE_XY` 页面分支。必须保证两路样本来自同步采集的同一帧；两路分别采集或时间不对齐时，XY 图没有可靠的相位意义。

### 7.4 复用按键方法

复制 `key_queue[]`、head/tail、`QueueKey()`、`HandleKeypad()` 的队列框架和 SysTick 中的 5 ms 扫描段。中断里只允许扫描和入队，页面切换、TFT 绘制和 ADC 操作继续留在主循环。

## 8. 验证状态

- Generate / Compile / Link：PASS；
- `-Wall -Werror`：PASS；
- Flash：NOT_RUN；
- Board：NOT_RUN；
- SRAM：10950 B（33.42%）；
- Flash：28232 B（21.54%）。

构建通过不等于已经实板确认模拟输入精度、噪声和屏幕刷新效果。首次上板应先使用幅值和频率已知的两路低压正弦信号检查。
