# Waveform Display Recipe

类型：`RECIPE`。目标是把已经选好的时域数据窗口可靠地画出来；不是 ADC 驱动，也不是完整示波器 Application。

## 1. Contract

| 项目 | 内容 |
|---|---|
| 输入 | `float samples[N]`、实际 `Fs`、显示起点、显示点数、Y scale、plot rectangle |
| 输出 | 屏幕波形、网格、baseline；可选 time/voltage labels |
| 前置 | buffer 不再由 DMA 写；触发位置/显示窗口已确定 |
| 默认实现 | `[MODULE] tft_ili9341` + `[MODULE] tft_waveform` |
| 状态 | 绘图逻辑 PC/Build verified；真实屏幕 Board `NOT_RUN` |

## 2. 完整逻辑链

```text
[MODULE] ADC DMA / ADC Ping-Pong DMA
→ [RECIPE] ADC To Voltage
→ [MODULE] Trigger Capture（需要触发时）
→ [RECIPE] Application 选择 start 与 view_count
→ [MODULE] TFT Waveform
→ [MODULE] TFT ILI9341
```

每一步存在的原因：ADC 提供等间隔 raw；转换给出物理单位；Trigger 决定稳定横向位置；Application 的 timebase 决定看多长；TFT Waveform 负责 N→像素；TFT 驱动负责 SPI/图元。

## 3. X 轴映射

显示窗口时间：

```text
T_view_s = view_count / Fs
time_per_div_s = T_view_s / horizontal_divisions
```

`Fs` 必须是当前实际配置/校准值，不是随手写的 nominal 值。若只显示两个周期：先由可靠频率/period Recipe 得到 `T`，令 `T_view≈2T`，再从触发点附近选 `view_count≈2T×Fs`；边界不足时缩短或等待下一帧，不能越界。

## 4. N 大于屏宽时怎么保留信息

### A. Simple decimation

每个显示列取一个代表样本并连接折线。适合平滑正弦；可能漏掉只占少数样本的脉冲/毛刺。

### B. Min-max envelope — 默认

每个显示列对应一段输入样本，找该段最小和最大，再画一条竖线：

```text
column k ← samples[start_k ... end_k)
              ↓ min/max
          vertical line
```

适合方波、过冲、毛刺、窄脉冲和大 N。正式实现是 `SignalTFTWaveform_GetEnvelopeColumn/Draw`，不需要 Application 再写一份循环。

## 5. Y 轴、V/div 与 baseline

- Fixed scale：`minimum_value`/`maximum_value` 固定；适合真实 V/div 与帧间比较。
- Auto scale：每帧 min/max；适合找信号，不适合声明固定 V/div。
- baseline：0 V 或模拟偏置（例如 ADC 前端中点），必须与输入数组单位一致。
- 超范围：只在屏边裁剪，测量结果仍来自原数组；若 ADC 已 clipping，应单独告警。

```text
volts_per_div = (scale_maximum - scale_minimum) / vertical_divisions
```

## 6. Trigger 对齐

`Trigger Capture` 在 raw 数组中找带 hysteresis 的上升/下降沿。把触发点前 `pretrigger_count` 点和之后点组成 view，再交给显示。触发电平可以画水平线，触发时刻可以画竖线。

失效条件：没有满足方向的 crossing、阈值超出信号范围、hysteresis 太小导致噪声多触发、pre/post 区间越界。此时显示 `NO TRIGGER` 或 free-run，不伪造触发点。

## 7. 单帧与连续刷新

| 模式 | 安全顺序 | RAM |
|---|---|---|
| Snapshot | Acquire → Process → Display → next Acquire | raw `2N` + optional float `4N` |
| Ping-Pong | DMA 写 A 时处理 B；取得 ready、必要时复制 display snapshot、Release | raw `4N` + processing/display buffer |
| Ring history | producer push、trigger 后复制固定 view | ring `2×capacity` + view |

显示函数不得在 ISR 中运行。若 UI 慢，降低 UI 刷新频率或画 dirty region，不降低 ADC Fs 来迁就屏幕。

## 8. 常改参数

| 参数 | 在哪里 | 影响 |
|---|---|---|
| `Fs`、`N` | acquisition config / `.syscfg` | 时间窗、RAM、带宽 |
| `view_count`、`start` | Application | timebase / trigger position |
| fixed min/max | display config | V/div |
| envelope/decimate | display config | 窄事件保真/观感 |
| grid divisions | display config | 刻度 |
| display cadence | Application scheduler | UI 流畅与 CPU/SPI 占用 |

## 9. 验证

PC：对 ramp/sine/square/single-sample pulse 比较映射，确认每个 envelope bucket 无遗漏；边界覆盖 flat、NaN、N<width、N≫width。Board：输入已知方波与窄脉冲，对照台式示波器检查时基、V/div、触发稳定性和刷新期间采样丢帧。

