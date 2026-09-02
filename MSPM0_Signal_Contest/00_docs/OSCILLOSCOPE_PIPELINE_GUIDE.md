# Oscilloscope Pipeline Guide

当前 `08_applications/oscilloscope/` 是 **LEGACY analysis glue**，没有采集、触发、显示和可导入工程，不能当成完成的数字示波器。下面给出用真实模块拼成示波器的分层路线。

## 1. 最小可交付架构

```text
Analog Frontend
  ↓
[MODULE] ADC DMA / ADC Ping-Pong DMA
  ↓ raw[N]
[RECIPE] ADC To Voltage
  ├→ [RECIPE] DC/Min/Max/Vpp/RMS/Frequency panel
  └→ [MODULE] Trigger Capture → timebase-selected view
                                  ↓
                          [MODULE] TFT Waveform
                                  ↓
                          [MODULE] TFT ILI9341

[MODULE] Button / Keypad / Rotary Encoder
  → Application menu: run/stop, trigger, time/div, V/div
```

`CORE_CHAIN` 到 voltage/trigger/measurements；`SYSTEM_CHAIN` 还必须包括 TFT、输入控件、buffer ownership、刷新节拍和错误状态。

## 2. 从最简单到连续版

### Stage A：Snapshot 波形

`ADC DMA → ToVoltage → TFT Waveform → TFT`。先固定 Fs/N、fixed V range，不加 trigger/menu。RAM 约 raw `2N` + float `4N`；TFT 无 framebuffer。

### Stage B：稳定触发

在一帧里 `SignalTrigger_Find`，再按 pretrigger 提取/选择 view。找不到触发时明确 free-run/NO TRIGGER。

### Stage C：测量栏

对同一个完成帧运行 Mean/MinMax/Vpp/RMS/AC RMS 和一个被选中的 frequency Recipe。不要为了显示再复制第二份测量数组。

### Stage D：Time/div 与 V/div

- Time/div 改显示 view_count；只有当前采样窗不够时才安全停止采集、改 Fs/N、重启。
- V/div 改 TFT fixed scale；输入超 ADC 合法范围必须先改模拟衰减/增益，不能只缩屏幕。

### Stage E：连续采集

使用 Ping-Pong：DMA 写一块、main 处理另一块。只有获得 ready buffer 后读取，并在处理/必要 snapshot 完成后 Release。显示只按较低 UI 节拍更新。

## 3. Trigger 设计

| 项目 | 默认 | 何时改 |
|---|---|---|
| edge | rising | 题目明确下降沿 |
| level | 用户/自动上下电平中点 | 有偏置、非对称脉冲时 |
| hysteresis | 大于 ADC 噪声抖动 | 多触发或漏触发时实测调整 |
| pretrigger | 约显示窗一部分 | 需要更多触发前历史 |
| mode | normal/free-run fallback | single capture 时停止下一帧 |

硬件 Comparator+Timer 更适合精确边沿 timestamp，但波形显示仍需要 ADC 数据；不要把 Timer Capture 当波形 buffer。

## 4. Timebase 选择

目标是让用户看到有意义的周期数：

```text
visible_cycles ≈ view_count × frequency_hz / Fs
```

自动“两周期”依赖有效 frequency；未知/不稳定时保留手动 time/div。若目标最高频率接近 ADC/前端带宽，先解决 anti-alias、采样孔径和前端建立，不靠屏幕插值假装波形准确。

## 5. Vertical scale 与 Auto range

屏幕 auto scale 只改变坐标；硬件 auto range 需要可控 PGA/VGA/衰减器、settling、重采、标定。两者必须分开命名：

```text
DISPLAY_AUTO_SCALE：只看清曲线
HARDWARE_AUTO_RANGE：改变前端 gain/range 后重新采集
```

## 6. 测量面板

最小面板：DC、Vpp、RMS/AC RMS、Frequency、trigger status、Fs、time/div、V/div。结果应带 valid/status；clipping、no trigger、frequency invalid 不能显示为 0。

## 7. SysConfig / 资源

- ADC DMA：ADC12 + sampling Timer + Event Fabric + DMA。
- 连续双缓冲仍使用实际 ADC/DMA 平台；`adc_pingpong_dma` 本身只管理 buffer 状态，不凭空配置硬件。
- TFT：SPI + CS/DC/RESET/BL GPIO。
- HMI：Keypad 8 GPIO，或 Encoder 2/3 GPIO，或 Button 1 GPIO/instance。
- 具体 ADC、Timer、DMA、SPI instance 和 Pin 必须从选中的 Profile/`.syscfg` 读取；先跑 `resource_check`。

## 8. RAM 预算

| 块 | 估算 |
|---|---:|
| 单帧 raw | `2N` B |
| 两块 ping-pong raw | `4N` B |
| voltage float | `4N` B |
| 独立 display snapshot（如需要） | `4×view_count` B |
| TFT waveform helper | O(1) |
| SSD1306 framebuffer（若选） | 1024 B |

不要为 trigger、measurement、display 各复制整帧。最终以完整 Application `.map` 为准。

## 9. Build / Board 验收顺序

1. 原 Profile ADC DMA 已知直流/正弦输入，确认 raw、Fs、N。
2. 加 ToVoltage，检查 VREF/校准。
3. 加 TFT 数值，Build/Link。
4. 加 TFT Waveform snapshot，检查固定 scale。
5. 加 trigger 和 timebase。
6. 加 measurement panel/HMI。
7. 最后升级 ping-pong，测 overrun 和 display blocking。

当前状态：各积木有 Build/PC 证据；完整 oscilloscope SYSTEM_CHAIN 尚无新的 CCS Application full link/board evidence，记为 `DRAFT / MISSING APPLICATION CLOSURE`。

