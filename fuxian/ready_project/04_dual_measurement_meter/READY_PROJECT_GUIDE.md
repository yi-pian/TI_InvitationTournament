# 04 双通道综合测量仪使用与复用说明

## 1. 工程作用

本工程对两路同步信号进行综合数值测量。每帧输出两路频率、Vpp、RMS、DC，并计算 CH2 相对 CH1 的增益、增益 dB、相位和延时。

适合：

- 测量放大器或滤波器输入/输出；
- 快速检查两路频率和幅值是否一致；
- 测量增益、衰减、相移和时间延迟；
- 作为信号题中最常用的双通道数值母版。

## 2. 默认接口

| 项目 | 默认设置 |
|---|---|
| CH1 | ADC PA25，通常作为参考或输入 |
| CH2 | ADC PA17，通常作为被测输出 |
| 采样点数 | 512 |
| 请求采样率 | 100 kSa/s |
| 显示 | ST7789 |
| 页面按键 | A/B |

两路必须共地，且输入处于 ADC 允许范围内。

## 3. 页面说明

### BASIC

分别显示 CH1 和 CH2 的：

- Frequency；
- Vpp；
- RMS；
- DC/Mean。

### DUAL / RELATION

显示：

- `Gain = CH2 Vpp / CH1 Vpp`；
- `Gain(dB) = 20 log10(Gain)`；
- CH2 相对 CH1 的 Phase；
- `Delay = Phase / (360 × Frequency)`。

A/B 均用于切换 BASIC 与 DUAL 页面。

## 4. 运行数据流

```text
同步双 ADC DMA 帧
  -> CH1/CH2 各一次 code→V
  -> 两路共用参数化 Basic 测量函数
  -> 两路各一次过零测频
  -> 双通道相位模块处理同一原始帧
  -> 计算 Gain、Gain dB、Delay
  -> 局部刷新当前页面数值
```

Gain、Phase 和 Delay 使用同一帧数据，不会将不同时刻的两帧结果组合。

## 5. 复用的 fuyong 内容

| 来源 | 当前函数/能力 | 类型 | 用途 |
|---|---|---|---|
| `04_dual_adc_dma` | `AcquireDualADCFrame()` | `FUYONG_COPY` | 同步双路帧 |
| `30_basic_measurement` | `ConvertADCToVoltage()` | `FUYONG_ADAPTED` | 参数化 code→V |
| `30_basic_measurement` | `MeasureBasicParameters()` | `FUYONG_ADAPTED` | Mean/Min/Max/Vpp/RMS/AC RMS |
| `11_zero_cross_frequency` | `MeasureFrequencyZeroCross()` | `FUYONG_ADAPTED` | 两路频率 |
| `40_dual_channel_measurement` | `SignalDualADCPhase_Process()` 调用 | `FUYONG_ADAPTED` | 相位关系 |
| 本工程 | Gain、dB、Delay 组合逻辑 | `READY_PROJECT_LOCAL` | 形成综合仪表 |
| `70_keypad_usage`、`moni01` | 队列按键切页 | `FUYONG_ADAPTED` | 页面控制 |
| `80_tft_usage`、`moni01` | 两页静态标签和局部数值刷新 | `FUYONG_ADAPTED` | 无整屏周期闪烁 |

## 6. 如何使用

1. Generate、Clean、Build 并烧录。
2. 将参考信号或网络输入接 CH1，被测输出接 CH2。
3. 进入 BASIC 页检查两路频率、Vpp、RMS 和 DC 是否合理。
4. 进入 DUAL 页读取增益、相位和延时。
5. 若显示 `PHASE: NO DATA`，检查两路幅值、频率、噪声、削顶以及采样窗口内是否有足够周期。
6. 若 CH1 幅值接近零，Gain 会失去意义；先确认参考通道有效。

## 7. 如何复用到其他工程

### 7.1 复用 Basic 测量

复制 `ConvertADCToVoltage()`、`MeasureBasicParameters()` 和 `basic_result_t`。同一函数可处理任意通道：只需传入不同输入数组和结果指针，不要复制为 `MeasureCH1()`、`MeasureCH2()` 两份。

### 7.2 复用增益测量

复用 `RunMeasurement()` 中的 Gain 计算段，并明确 CH1 是分母、CH2 是分子。如果赛题定义相反，只交换通道输入或修改一次公式，不要改变变量的物理含义。

### 7.3 复用相位和延时

原样复制 `signal_dual_adc_phase` 模块和对应调用配置。相位必须使用同步双 ADC 原始帧。Delay 需要有效频率；应保留 `phase_valid` 与 frequency valid 判断。

### 7.4 复用仪表页面

复制 `DrawStaticUi()` 和 `UpdateDisplay()` 中需要的页面分支。固定标签仅在翻页时重画，测量循环只清除数值矩形。若增加第三页，应扩展 `app_page_t`，不要把测量模式塞入 `current_page`。

### 7.5 与 FFT 组合

如果目标工程需要 FFT 测频，应在两路数据准备后每路执行一次 FFT，并让测频、THD 和绘图共享 magnitude；不要在本 Basic 流程中重复转换电压。

## 8. 验证状态

- Generate / Compile / Link：PASS；
- `-Wall -Werror`：PASS；
- Flash / Board：NOT_RUN；
- SRAM：11700 B（35.71%）；
- Flash：30616 B（23.36%）。

相位符号、通道方向、模拟前端增益和 ADC 校准必须在实板上用已知相位差信号确认。
