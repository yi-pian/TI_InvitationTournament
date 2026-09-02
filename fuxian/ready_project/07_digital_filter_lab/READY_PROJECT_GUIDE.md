# 07 数字滤波处理器使用与复用说明

## 1. 工程作用

本工程对同一 ADC 帧同时保留 RAW 和 FILTERED 数据，并在 TFT 上比较两者的时域波形和频谱。支持 NONE、MOVING_AVERAGE、MEDIAN、HAMPEL。

适合：

- 观察移动平均对高频噪声的抑制；
- 观察中值滤波对脉冲噪声的处理；
- 观察 Hampel 对离群点的识别与替换；
- 快速选择适合赛题数据的鲁棒预处理方法。

## 2. 默认接口与参数

| 项目 | 默认设置 |
|---|---|
| 输入 | ADC CH1 / PA25 |
| CH2 | 同步 DMA 占位 |
| 采样点数 | 512 |
| 请求采样率 | 100 kSa/s |
| 初始模式 | NONE |
| 初始窗口 | 5，保持奇数 |
| Hampel 阈值 | 3.0 |

## 3. 页面与按键

| 按键 | 功能 |
|---|---|
| A/B | WAVEFORM / SPECTRUM 页面切换 |
| C | NONE / MOVING_AVERAGE / MEDIAN / HAMPEL |
| `*` / `#` | 减小/增大窗口，保持奇数 |
| D | 循环调整 Hampel threshold |

颜色：RAW 为黄色，FILTERED 为青色。

## 4. 运行数据流

```text
采集唯一 ADC frame
  -> code→voltage_samples，只执行一次
  -> ApplySelectedFilter 生成 filtered_samples
  -> RAW 对同一帧执行一次 FFT
  -> FILTERED 对滤波结果执行一次 FFT
  -> WAVEFORM 或 SPECTRUM 页面复用上述四份数组
  -> 局部刷新参数行和图框内部
```

RAW 与 FILTERED 绝不是两次不同采集，因此能够直接比较滤波效果。

## 5. 各模式含义

| 模式 | 适合问题 | 主要代价 |
|---|---|---|
| NONE | 查看原始信号 | 不处理噪声 |
| MOVING_AVERAGE | 随机高频噪声 | 会平滑边沿并产生窗口延迟 |
| MEDIAN | 孤立脉冲、毛刺 | 窗口过大会损失细节 |
| HAMPEL | 离群点检测与替换 | 依赖窗口和 threshold |

本工程没有为了凑功能自行加入未经验证的大型 FIR/IIR 设计器。

## 6. 复用的 fuyong 内容

| 来源 | 当前函数/能力 | 类型 | 用途 |
|---|---|---|---|
| `04_dual_adc_dma` | `AcquireADCFrame()` | `FUYONG_ADAPTED` | 获取唯一 RAW 帧 |
| `15_filter_processing` | `ConvertADCToVoltage()` | `FUYONG_ADAPTED` | code→V |
| `15_filter_processing` | `ApplyMovingAverage()` | `FUYONG_ADAPTED` | 移动平均 |
| `15_filter_processing` | Median/Hampel 模块调用 | `FUYONG_ADAPTED` | 鲁棒滤波 |
| `50_robust_measurement` | Hampel 思路 | `FUYONG_ADAPTED` | 离群点处理 |
| `20_fft_analysis` | `RunFFTCommon()` | `FUYONG_ADAPTED` | RAW/FILTERED 各一次 FFT |
| `21_time_domain_waveform`、`22_spectrum_display` | 波形和频谱映射 | `FUYONG_ADAPTED` | 双 trace 页面 |
| `moni01` | 按键队列和局部图框刷新 | `FUYONG_ADAPTED` | 稳定交互与显示 |

## 7. 如何使用

1. 将待处理信号接 CH1 并共地。
2. Build、烧录后先选择 NONE 查看原始信号。
3. 用 C 选择滤波器，用 `*`/`#` 调整窗口。
4. 在 WAVEFORM 页观察形状、毛刺和延迟。
5. 在 SPECTRUM 页观察噪声与频率分量变化。
6. 使用 HAMPEL 时查看右下角 outlier count，并用 D 调整阈值。

窗口越大不一定越好。比赛现场应以“保留有效波形特征并降低目标噪声”为准。

## 8. 如何复用到其他工程

### 8.1 复用滤波函数

复制 `ApplyMovingAverage()` 或 `ApplySelectedFilter()`，并原样复制 Median/Hampel 模块。函数输入输出应保持分离，避免原地覆盖仍需显示或测量的 RAW 数据。

### 8.2 与测量算法组合

采集后先做一次 code→V，再生成 FILTERED。Basic、FFT、过零或拟合算法根据需要选择 RAW 或 FILTERED 数组，但不要各自再次转换 ADC。

### 8.3 复用同帧对比结构

至少保留：

- `voltage_samples[]`；
- `filtered_samples[]`；
- 一份必要 workspace；
- 明确的 `filter_mode`、window 和 threshold。

大数组必须是 static。若目标工程 SRAM 紧张，可让 Median/Hampel 和 FFT 在不同阶段共享经过确认的 workspace，但不能覆盖仍要绘图的数据。

### 8.4 复用双频谱比较

RAW 和 FILTERED 各自 FFT 一次，然后把两个 magnitude 传给绘图。不能为了频谱页重新采样或重新滤波。

## 9. 验证状态

- Generate / Compile / Link：PASS；
- `-Wall -Werror`：PASS；
- Flash / Board：NOT_RUN；
- SRAM：16430 B（50.14%）；
- Flash：47512 B（36.25%）。

该工程 SRAM 占用较高；继续加入 FIR/IIR 前必须重新检查 map 文件。
