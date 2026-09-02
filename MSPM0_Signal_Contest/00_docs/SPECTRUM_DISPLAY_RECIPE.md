# Spectrum Display Recipe

类型：`RECIPE`。它接收已经校正语义的单边频谱，不负责 FFT 本身。

## 1. Contract

| 项目 | 内容 |
|---|---|
| 输入 | `magnitude[0..N/2]`、实际 `Fs`、FFT `N`、幅值语义、显示频率范围 |
| 输出 | 线性或 dB 频谱、主峰/候选峰 marker、数值标签 |
| 默认显示 | `[MODULE] tft_ili9341` 的 line/bar/text 图元 |
| 算法前置 | Remove DC → Window → FFT → Magnitude → Window Gain Correction |

## 2. 完整链

```text
[MODULE] ADC DMA
→ [RECIPE] ADC To Voltage
→ [RECIPE] Remove DC
→ [MODULE] Window
→ [MODULE] FFT
→ [MODULE] FFT Magnitude
→ [MODULE] Window Gain Correction
→ [RECIPE] Peak Detect / [MODULE] Peak Interpolation
→ [RECIPE] 本显示映射
→ [MODULE] TFT ILI9341
```

不允许把未经 N、single-sided 和 coherent-gain 说明的 raw magnitude 直接标成 V。若只关心相对 dB，仍要写清 reference。

## 3. Hz 到 X 坐标

```text
bin_frequency_hz(k) = k × Fs / N
x = plot_left + (f_k - f_min) / (f_max - f_min) × (plot_width - 1)
```

先把 `f_min/f_max` 转成合法 bin 范围并夹在 `[0, Fs/2]`。若多个 bin 落在同一像素列，默认保留该列最大值；这与时域 envelope 的目的相同：避免窄峰消失。

## 4. Y 轴

### Linear

用于物理幅值，Y scale 单位必须写清 peak、RMS、V 或 code。固定 scale 便于比较，auto scale 便于找峰。

### dB

幅度谱使用：

```text
dB = 20 log10(max(amplitude, floor) / reference)
```

功率/能量用 `10 log10`。`reference` 可为 1 V、full-scale 或 fundamental；不能把 dBV、dBFS、dBc 混写。`floor` 只防 `log(0)` 并限制显示底，不应反写原频谱。

## 5. Marker 与 Top Peaks

每个 marker 至少保存：bin、插值后 frequency_hz、amplitude、单位、是否有效。找 top peaks 前应：

1. 限定搜索频段；
2. 排除 DC（若任务不需要）；
3. 选中一个峰后屏蔽其主瓣邻域再找下一个；
4. marker 文字避让，不在每帧重画整屏大块背景。

Peak Interpolation 改善频率位置，不会自动修正泄漏、谐波能量或物理幅值。

## 6. RAM / CPU / SysConfig

- raw：`2N` bytes；float voltage/window：按是否 in-place 为 `4N`；complex FFT：`8N`；single-sided magnitude：约 `4(N/2+1)`。
- 显示可按列边算边画，不需要第二份全屏频谱数组。
- SysConfig 只来自 ADC DMA + TFT 两边；必须检查 Timer/Event/DMA、SPI/GPIO 和 Pin 冲突。
- FFT 计算和画屏都放 main/task；ADC ISR 只置完成标志。

## 7. 常见失效

- alias：没有模拟 anti-alias，屏幕上看到的是折叠峰。
- 非相干采样无窗：泄漏看起来像噪声/多峰。
- 忘记 coherent gain：幅值错误。
- dB reference 不写：读者无法知道 dBFS/dBc/dBV。
- 每帧 auto Y：噪声底和峰高无法跨帧比较。
- TFT 刷新阻塞连续采集：用 ping-pong + throttled snapshot。

