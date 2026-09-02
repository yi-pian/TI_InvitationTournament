# Display / HMI Selection Guide

这份文件只回答“结果怎么显示、用户怎么操作”。它不代替测量算法 Recipe。

## 1. 默认选择表

| 我要实现 | 默认先用 | 特殊情况 | 为什么 |
|---|---|---|---|
| 多个数值 + 菜单 | TFT ILI9341 | 只有少量字符用 SSD1306 | TFT 分辨率、颜色和图形能力更适合综合仪器 |
| 少量数值/状态 | SSD1306 | 要曲线/复杂菜单用 TFT | 128×64、1 KiB framebuffer，接线简单 |
| 波形/脉冲 | TFT + TFT Waveform | SSD1306 只做低分辨率预览 | TFT Waveform 有 min-max envelope，不漏每列内窄峰 |
| 频谱/Bode/XY | TFT 基础图元 + 对应 Recipe | 无专用大框架 | 映射规则简单，数据语义应留在 Application |
| 16 个直接功能键 | Matrix Keypad 4×4 | 键很少用 Button | 一次提供 0～9、A～D、*、# 等输入 |
| 连续改数值/菜单滚动 | Rotary Encoder | 只需上下/确认可用 3 个 Button | 旋钮更适合频率、量程、时基等连续参数 |
| 单次确认/开始/停止 | Button | 保持拨动状态用 Latching Switch | 已有消抖 pressed/released 事件 |
| 保持 ON/OFF 的机械开关 | Latching Button Switch | 瞬时键不要用它 | 输出稳定 on/off 和改变事件 |

## 2. 真实模块与 API

### TFT ILI9341 — `[MODULE] BUILD_VERIFIED`

- 路径：`01_bsp/tft_ili9341/`
- 初始化：`SignalTFTILI9341_MSPM0_Init()`；底层通用入口 `TFT_ILI9341_Init()`。
- 常用：`DrawLine`、`DrawRect`、`FillRect`、`DrawString`、`DrawInt32`、`DrawFloat`。
- 字库：4 套 printable ASCII；内置“电”“子”仅为自定义 16×16 字模示例，不是完整中文字库。
- RAM：无全屏 framebuffer；字库在 Flash；已有带字库完整链接证据，Board 未运行。
- SysConfig：SPI + CS/DC/RESET/BL GPIO；具体实例/Pin 只从目标 `.syscfg` 和生成头文件确认。

### TFT Waveform — `[MODULE] BUILD_VERIFIED`

- 路径：`01_bsp/tft_waveform/`
- 入口：`SignalTFTWaveform_Draw()`。
- 作用：fixed/auto Y scale、grid/baseline、decimate/min-max envelope。
- 不做：ADC、trigger、timebase、refresh scheduler。

### SSD1306 — `[EXTERNAL DEVICE DRIVER] COMPILE_VERIFIED`

- 路径：`12_external_devices/display/ssd1306/`。
- 常用：`SSD1306_Init`、`Update`、`ClearBuffer`、`DrawPixel`、`DrawLine`、`DrawString6x8`。
- RAM：128×64 单色 framebuffer 固定 1024 B。
- 注意：full-frame blocking I2C 更新会占用可观时间；菜单/数值可用，连续示波器主显示不作为默认。

### Matrix Keypad 4×4 — `[MODULE] BUILD_VERIFIED`

- 路径：`01_bsp/matrix_keypad_4x4/`。
- 入口：`SignalMatrixKeypad4x4_Init`、`Scan`、`GetKey`、`GetFirstPressed`。
- 资源：4 output + 4 input GPIO；需要应用周期扫描，矩阵多键可能 ghost。

### Rotary Encoder — `[MODULE] BUILD_VERIFIED`

- 路径：`01_bsp/rotary_encoder/`。
- 入口：`SignalRotaryEncoder_Init`、`Update`、`Get/SetPosition`。
- 资源：A/B 两个 GPIO input，可选 SW；默认轮询，丢步再考虑 IRQ。

### Button / Latching Switch — `[MODULE] BUILD_VERIFIED`

- 路径：`01_bsp/button/`、`01_bsp/latching_button_switch/`。
- 普通按键：`SignalButton_Init/Update/GetPressed`。
- 自锁开关：`SignalLatchingButtonSwitch_Init/Update/GetState`。
- 资源：每实例一个 GPIO input；不要为 1 个按键引入矩阵扫描。

## 3. 显示刷新与采集的所有权

```text
单帧：ADC DMA 完成 → Process → Display snapshot → 下一帧

连续：Ping-Pong DMA ISR 只切换 buffer
                   ↓
       main 获取 ready block → Process → Release
                              └→ 低频率更新 display snapshot
```

禁止在 ADC/DMA/Timer ISR 里画屏。TFT/OLED 的阻塞通信时间必须和采集/处理时序分开测量；README 中的刷新周期只能是示例，不能当题目固定值。

## 4. 一个最小菜单怎么拼

```text
Rotary Encoder step / Matrix Keypad key / Button event
  → Application 中的 selected_item、editing、value
  → 只重画变化的文字/数值区域
  → 参数确认后再通知采集/发生链安全重配
```

菜单状态属于 Application，不需要再造“UI Framework”模块。频率、Fs、N、V/div 等硬件敏感参数不要在 DMA 正运行时直接改。

## 5. 资源冲突清单

| 组合 | 必查 |
|---|---|
| TFT + 外置 SPI ADC/DAC/DDS | SPI 是否共享、CS 独立、mode/bitrate 切换、阻塞时间 |
| Keypad + TFT | GPIO Pin 是否冲突 |
| Encoder IRQ + Timer Capture/ADC | IRQ 优先级与最长关中断时间 |
| SSD1306 + I2C ADC/数字电位器 | 地址、总线速率、blocking transaction |
| TFT + 连续 ADC | DMA buffer 所有权；画屏不得持有采集 buffer 太久 |

用目标 Profile 的 `.syscfg` 和 `resource_check` 得出结论；本指南不猜某个应用的具体实例号。

