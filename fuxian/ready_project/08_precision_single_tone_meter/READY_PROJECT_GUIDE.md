# 08 精密单频测量仪使用与复用说明

## 1. 工程作用

本工程针对接近正弦的周期信号，同时保留 FFT、Zero Cross 和 Sine Fit 三种频率结果，并根据 FAST、NORMAL、PRECISION 模式选择推荐频率。

屏幕显示：

- 推荐 Frequency；
- Amplitude、Vpp、RMS、DC、AC RMS；
- THD、SNR；
- FFT frequency；
- ZeroCross frequency；
- SineFit frequency。

不同算法结果使用不同变量，绝不互相覆盖。

## 2. 默认接口与参数

| 项目 | 默认设置 |
|---|---|
| 输入 | ADC CH1 / PA25 |
| CH2 | 同步 DMA 占位 |
| 采样点数 | 512 |
| 请求采样率 | 100 kSa/s |
| 初始模式 | NORMAL |
| 显示 | ST7789 |

本工程适合较稳定、接近正弦的单频信号。方波、严重削顶、多音或快速扫频信号不适合作为 4 参数正弦拟合对象。

## 3. 模式与按键

| 按键 | 功能 |
|---|---|
| A | 上一个精度模式 |
| B | 下一个精度模式 |

| 模式 | 执行内容 | 推荐 `frequency_hz` |
|---|---|---|
| FAST | Basic + 整数 FFT，同时保留过零结果 | 整数 FFT 频率 |
| NORMAL | Basic + FFT 三点插值 + THD/SNR | 插值 FFT 频率 |
| PRECISION | NORMAL 前级 + Sine Fit 4P | 有效 Sine Fit；失败时回退插值 FFT |

## 4. 运行数据流

```text
采集唯一 ADC 帧
  -> 一次 code→V
  -> 一次 Mean/Basic/去 DC
  -> 一次 Window + FFT
  -> 整数 FFT frequency
  -> ZeroCross frequency
  -> NORMAL/PRECISION：FFT 三点插值
  -> THD/SNR 复用同一 magnitude
  -> PRECISION：以 FFT 插值频率为初值执行 Sine Fit 4P
  -> 明确选择推荐 frequency_hz
  -> 局部刷新各数值字段
```

Sine Fit 不会重新采集，也不会覆盖 FFT 和 Zero Cross 的独立结果。

## 5. 复用的 fuyong 内容

| 来源 | 当前函数/能力 | 类型 | 用途 |
|---|---|---|---|
| `04_dual_adc_dma` | `AcquireADCFrame()` | `FUYONG_ADAPTED` | 获取单通道帧 |
| `30_basic_measurement` | `PrepareSignalAndBasic()` | `FUYONG_ADAPTED` | code→V、Mean、Vpp、RMS、AC RMS |
| `20_fft_analysis` | `RunFFTCommon()` | `FUYONG_ADAPTED` | 唯一一次 FFT |
| `11_zero_cross_frequency` | `MeasureFrequencyZeroCross()` | `FUYONG_ADAPTED` | 独立过零频率 |
| `20_fft_analysis` | `RefineFFTFrequency()` | `FUYONG_COPY` 教学调用 | 三点插值 |
| `20_fft_analysis` | Harmonic/THD/SNR | `FUYONG_ADAPTED` | 信号质量 |
| `60_precision_measurement` | `RunSineFit4Param()` | `FUYONG_ADAPTED` | 精密正弦拟合 |
| 本工程 | `RunMeasurement()` 模式调度和回退 | `READY_PROJECT_LOCAL` | 推荐频率选择 |
| `moni01` | 8 项按键队列和局部数值刷新 | `FUYONG_ADAPTED` | 可靠交互 |

## 6. 如何使用

1. 将接近正弦的待测信号接 CH1，并确保共地与输入范围安全。
2. Build 并烧录，先使用 NORMAL。
3. 对比 FFT、ZeroCross 和推荐频率；结果接近说明信号与采样较稳定。
4. 需要快速响应时选择 FAST。
5. 需要更高精度且信号接近正弦时选择 PRECISION。
6. 如果 SineFit 显示 0 或无效，检查信号幅值、失真、频率初值和采样窗口；推荐值会回退到插值 FFT。
7. THD 高或 SNR 低时，Sine Fit 结果不一定优于普通 FFT。

## 7. 推荐频率选择逻辑

不要在其他工程中简单写成“后执行的算法覆盖 `frequency_hz`”。本工程使用：

```text
fft_frequency_hz
zero_cross_frequency_hz
sine_fit_frequency_hz
        │
        └── 按 mode 和 valid 状态选择 ──> frequency_hz
```

这样现场能够看到算法分歧，并判断异常来自波形、噪声还是算法适用性。

## 8. 如何复用到其他工程

### 8.1 复用 FAST 测量

复制 `PrepareSignalAndBasic()`、`RunFFTCommon()` 和 `RunMeasurement()` 的 FAST 分支。只需一份 FFT 工作区和 magnitude。

### 8.2 复用 FFT 插值

复制 `RefineFFTFrequency()` 及插值模块。输入必须是已经完成的 magnitude、peak bin 和采样率；不要在插值函数中重新 FFT。

### 8.3 复用 Sine Fit 4P

复制 `RunSineFit4Param()`，并原样复制 Sine Fit 模块。必须给出合理初始频率，优先使用插值 FFT 结果。保留 valid/失败回退逻辑。

### 8.4 复用多算法对照页面

分别保留三个频率变量，再复制局部显示字段。目标工程可以减少显示小数位，但不应把三种结果合并为同一存储变量。

### 8.5 与 Lock-In 组合

若目标频率已知，可在现有电压数据之后加入 `61_lock_in`。Lock-In 应复用 `voltage_samples` 或 centered 数据，不要重新采集或再次 code→V。

## 9. 验证状态

- Generate / Compile / Link：PASS；
- `-Wall -Werror`：PASS；
- Flash / Board：NOT_RUN；
- SRAM：13401 B（40.90%）；
- Flash：50368 B（38.43%）。

三种频率算法的实际误差、Sine Fit 收敛范围和 THD/SNR 精度仍需用已校准信号源进行实板测试。
