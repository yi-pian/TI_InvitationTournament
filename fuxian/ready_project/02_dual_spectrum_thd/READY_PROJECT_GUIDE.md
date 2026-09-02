# 02 双通道频谱、谐波与 THD 分析仪使用与复用说明

## 1. 工程作用

本工程同时分析 CH1 和 CH2 的频谱、基波频率、谐波、THD、SNR 与 SFDR。两路 FFT 每帧各执行一次，所有数值分析和频谱绘图复用已经生成的幅度谱。

典型用途包括：

- 检查信号源、放大器或滤波器输出的谐波失真；
- 对比电路输入和输出的频谱变化；
- 快速寻找杂散、噪声和削顶产生的高次谐波；
- 作为赛题中的双通道 FFT/THD 母版。

## 2. 默认接口与参数

| 项目 | 默认设置 |
|---|---|
| CH1 | ADC PA25 |
| CH2 | ADC PA17 |
| 采样点数 | 512 |
| 请求采样率 | 100 kSa/s |
| 显示 | ST7789 |
| 控制 | 4×4 键盘 |

输入必须共地并限制在 ADC 允许电压范围内。FFT 的最高可显示频率受实际采样率的 Nyquist 频率限制。

## 3. 页面与按键

| 按键 | 功能 |
|---|---|
| A | 上一页面 |
| B | 下一页面 |
| C | 循环切换 5/10/25/50 kHz 频率显示范围 |

页面包括：

- CH1 频谱；
- CH2 频谱；
- 双通道数值汇总。

频谱页显示 F、THD、SNR、SFDR、H2、H3 和当前横轴范围。SUMMARY 页同时显示两路的核心结果。

## 4. 运行数据流

```text
同步双 ADC DMA 帧
  -> CH1 code→V、去 DC
  -> CH2 code→V、去 DC
  -> CH1 一次 Window + Q15 FFT + 单边幅度谱
  -> CH2 一次 Window + Q15 FFT + 单边幅度谱
  -> 分别从已有幅度谱计算频率、插值、谐波、THD、SNR、SFDR
  -> 当前页面局部刷新频谱框或数值行
```

切换页面或横轴范围不会重新执行 FFT。横轴范围只改变频谱像素映射。

## 5. 复用的 fuyong 内容

| 来源 | 当前函数/能力 | 类型 | 用途 |
|---|---|---|---|
| `04_dual_adc_dma` | `AcquireDualADCFrame()` | `FUYONG_COPY` | 获取同步双路帧 |
| `20_fft_analysis` | `PrepareSignal()` | `FUYONG_ADAPTED` | code→V 和去 DC 参数化 |
| `20_fft_analysis` | `RunFFTCommon()` | `FUYONG_ADAPTED` | Window、Q15 FFT、幅度和增益校正 |
| `20_fft_analysis` | FFT 频率与三点插值 | `FUYONG_ADAPTED` | 提高基波频率分辨率 |
| `20_fft_analysis` | Harmonic/THD/SNR/SFDR 调用 | `FUYONG_ADAPTED` | 质量分析 |
| `22_spectrum_display` | 频谱显示基础 | `FUYONG_ADAPTED` | 单边频谱显示 |
| 本工程 | dB 自动 Y 范围和谐波标记 | `READY_PROJECT_LOCAL` | 完整频谱仪页面 |
| `70_keypad_usage`、`moni01` | 8 项按键队列 | `FUYONG_ADAPTED` | 防止连续按键丢失 |
| `80_tft_usage`、`moni01` | 静态页和局部动态刷新 | `FUYONG_ADAPTED` | 只刷新频谱和数字 |

## 6. 如何使用

1. 在 CCS 中 Generate、Clean、Build 后烧录。
2. 将两路待测信号连接至 CH1/CH2，并连接公共地。
3. 用 A/B 切换 CH1、CH2 和 SUMMARY。
4. 用 C 选择适合信号基波及谐波的显示范围。
5. 若基波靠近 Nyquist 频率，能看到的高次谐波数量会受到限制。
6. THD、SNR、SFDR 对采样相干性、输入幅值、ADC 噪声和前端失真敏感；比赛现场应先用已知低失真正弦校验。

## 7. 如何复用到其他工程

### 7.1 复用单通道 FFT

复制 `PrepareSignal()`、`RunFFTCommon()` 以及 FFT 工作区：

- `fft_input[]`；
- `fft_q15[]`；
- `fft_magnitude_q15[]`；
- 一份目标通道 `fft_magnitude[]`。

调用顺序必须是采集一次、转换一次、去 DC 一次、FFT 一次。后续算法全部接收同一个 `fft_magnitude[]`。

### 7.2 复用 THD/SNR/SFDR

复制 `AnalyzeChannelSpectrum()`，并保留 `channel_analysis_t`。目标工程应原样复制 Window、增益校正、插值、Harmonic、THD、SNR、SFDR 模块，不能只复制函数调用而遗漏模块依赖。

### 7.3 复用双通道分析

为两路分别准备 magnitude 输出数组，但 FFT 工作区可以像本工程一样顺序共享。禁止为 CH1/CH2 写两套相同算法；对同一个参数化函数调用两次即可。

### 7.4 复用频谱图

复制 `DrawSpectrumTrace()` 和图框宏。频谱绘图直接读取 magnitude，不能为了显示重新 FFT。目标界面应保留“先局部清频谱框内部，再画新曲线”的方式。

### 7.5 复用按键和局部刷新

复制 `QueueKey()`、队列式 `HandleKeypad()`、SysTick 扫描段以及 `DrawStaticUi()`。如果目标工程已有 SysTick，需要把键盘 5 ms 分频并入原 ISR，不能重复定义第二个 `SysTick_Handler()`。

## 8. 验证状态

- Generate / Compile / Link：PASS；
- `-Wall -Werror`：PASS；
- Flash / Board：NOT_RUN；
- SRAM：18550 B（56.61%）；
- Flash：47992 B（36.61%）。

本工程是八个工程中 SRAM 占用最高者。移植时优先共享 FFT 工作区，不要再增加一套大数组。
