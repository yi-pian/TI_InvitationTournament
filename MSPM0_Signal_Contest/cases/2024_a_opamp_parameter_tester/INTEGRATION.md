# Integration Pattern

本文件描述最终工程的历史组合契约。新工程只能复用模式，实际函数签名必须重新读取当前 public `.h`。

## 总体数据流

```text
Control: Keypad char
→ App mode / numeric state
→ AD9850 frequency + DAC0 control

Wave measurement: Timer event
→ ADC0 MEM0
→ DMA_CH0
→ uint16_t raw[3072]
→ float voltage_v[3072]
→ Q2 AC RMS or Q3 robust platforms / edge timing
→ scalar result
→ TFT

Power measurement: software trigger
→ ADC1 MEM0
→ uint16_t raw[256]
→ float voltage_v[256]
→ Mean
→ current / power scalar
→ TFT
```

## Q2：UGBW 链

| 阶段 | 输出 → 输入 | C 类型 | 单位/语义 | 长度 | 所有权与生命周期 |
|---|---|---|---|---:|---|
| Timer/Event/ADC/DMA | ADC0 MEM0 → `g_wave_capture.raw` | `uint16_t[]` | 12-bit unsigned ADC code | 3072 | DMA 写；完成前应用不得复用 |
| ADC To Voltage | raw → `g_wave_voltage` | `const uint16_t*` → `float*` | code → V | 3072 | 两个数组均由应用静态持有；非 in-place |
| AC RMS | voltage → result | `const float*` → result struct | mean V、去均值 AC RMS V | 3072→2 scalars | 输入只读；两遍扫描；不需 workspace |
| 三帧平均 | three result structs → scalar | `float` | AC RMS V、mean V | 3 frames | 应用局部累计 |
| `-3 dB` 判定 | adjacent RMS → UGBW | `float` | ratio、Hz | scalar | 应用 Glue；相邻扫频点线性插值 |
| TFT | scalar → text/number | scalar | Hz、V、state | scalar | 阻塞刷新，非 ISR |

历史参数：

- 目标 Timer 事件率：3,555,556 Hz，32 MHz/9 的名义配置。
- 每帧 3,072 点；每个频点 3 帧。
- 扫频：10 kHz 到 2 MHz，步长 25 kHz，设置后等待 1 ms。
- 低频参考交流幅值必须大于 1 mV RMS。
- `target = reference × 0.70710678`。

重要：Q2 的信号有约 1.65 V 偏置，因此 `Mean` 只用于监视 DC，不能当交流增益；AC RMS 自己先去均值。

## Q3：Slew Rate 链

| 阶段 | 输出 → 输入 | C 类型 | 单位/语义 | 长度 | 所有权与 in-place |
|---|---|---|---|---:|---|
| ADC DMA | ADC0 → raw | `uint16_t[]` | ADC code | 3072 | 同 Q2 |
| ADC To Voltage | raw → voltage | `uint16_t[]` → `float[]` | V at PA25 | 3072 | 非 in-place |
| Robust Vpp | voltage + workspace → levels | `const float*`, `float*` → struct | 5%/95% platform V and difference V | 3072 | workspace 会被重排；输入不改 |
| threshold search | voltage + levels → edge times | `const float*` → scalars | 20%→80%/80%→20% samples then us | 3072 | 应用自写；不修改输入 |
| scale | conditioned Vpp → DUT Vpp | `float` | V | scalar | 乘 `MODE3_ADC_TO_DUT_OUTPUT_SCALE=3.0` |
| slew | Vpp/time → SR | `float` | V/us | scalar | `0.6×Vpp/time_us`，上升/下降分开 |

历史参数：固定 5 kHz；Q3 前切换名义 2,000,000 samples/s；32 MHz/16；一个样点名义 0.5 us。5 kHz 一帧约 1.536 ms，可覆盖约 7.68 个周期。

### Buffer 复用

```c
union {
    uint16_t raw[3072];
    float robust_workspace[3072];
} g_wave_capture;
```

这项复用成立的前提是严格的生命周期：

1. DMA 完成后，raw 转换到独立的 `g_wave_voltage`。
2. 后续 Q3 不再读取 raw。
3. 同一片 12,288 B 内存才允许改作 Robust workspace。

它不是普通的 in-place 算法调用；如果新流程需要同时保留 raw 和 workspace，不能照搬 union。

## Q4：静态功耗链

| 阶段 | 输出 → 输入 | C 类型 | 单位/语义 | 长度 | 所有权 |
|---|---|---|---|---:|---|
| ADC1 single conversion | POWER_ADC MEM0 → raw | `uint16_t[]` | ADC code | 256 | 应用逐点写 |
| ADC To Voltage | raw → voltage | `uint16_t[]` → `float[]` | shunt sense V | 256 | 应用静态数组；非 in-place |
| Mean | voltage → mean | `const float*` → struct | V | 256→1 | 输入只读 |
| Electrical formula | V → I → P | `float` | A、mA、mW | scalar | 应用 Glue |

历史公式：

```text
I = Vshunt / 750 ohm
P = I × 24 V × 1.0 rail_factor
```

750 Ω、24 V、rail factor、输入 scale 和 offset 都属于案例电路，不是模块必需参数。

## 初始化顺序

最终 `main` 的实际顺序：

1. `SYSCFG_DL_init()`：先让 `.syscfg` 生成的 GPIO、ADC、DAC、SPI、Timer、DMA、Event 生效。
2. `SignalADC_Init(&adc_config)`：建立 ADC DMA 模块运行参数。
3. `AD9850_Init(...)`：复位并建立外部 DDS 状态。
4. 设置 VCA820 DAC 码和 Q1 初始 DDS 频率。
5. 初始化矩阵键盘。
6. 初始化 ILI9341、设置旋转方向、清屏、画静态布局。
7. 进入 5 ms 扫描周期的 Superloop。

不要把算法或 TFT 放进 ADC/DMA ISR。最终应用在主循环中等待采集完成、执行浮点计算并刷新显示。

## SysConfig：模块必需 vs 案例特定

### 模块必需

- `adc_dma`：一个 ADC conversion memory、Timer 周期事件、Event 路由、一个 DMA channel、DMA 完成状态/IRQ，以及与当前模块源码一致的生成宏。
- `POWER_ADC` Direct DriverLib：一个软件触发 ADC conversion memory，12-bit unsigned，参考源和结果完成标志。
- AD9850：四个数字输出 GPIO。
- VCA820 控制：一个 DAC12 输出，当前电路需要输出放大器 ON。
- ILI9341：SPI TX/SCLK/CS 加 DC/BLK GPIO；是否需要 MISO 由当前平台模块和屏幕接线重新确认。
- Keypad：四个行输出、四个列输入及上拉。

### 这个案例的具体分配

| 资源 | 历史实例/Pin |
|---|---|
| Wave ADC | ADC0 channel 2 / PA25 |
| Power ADC | ADC1 channel 2 / PA17 |
| DMA | DMA_CH0 |
| Timer/Event | TIMG0 / publisher channel 1 |
| DAC | DAC0 / PA15 / amplifier ON |
| AD9850 | PA12、PA13、PA28、PA31 |
| TFT | SPI1 PB9/PB8/PB6，PB15 DC，PB12 BLK |
| Keypad rows | PB16、PB0、PB7、PB17 |
| Keypad columns | PB18、PB13、PB20、PB4，内部 Pull-up |

这些 Pin 和实例不是可迁移真值。新工程应让 Resource Check 根据当前 `.syscfg` 判断冲突。

如果 MCU、ADC、显示器或板卡改变，不能复用本案例的 SysConfig；只能把模块必需资源重新映射到当前硬件。

## Application Pattern

通用部分：

```text
Init → Read control → Configure excitation → Settle
→ Acquire bounded data → Convert units → Measure
→ Decide threshold/result → Display → Loop
```

本题专用部分：

- A/B/C/D 模式和 `#` 重测规则。
- VCA820 控制曲线与 DDS 幅度常量。
- Q2 扫频范围、步长、0.707 判定和 LIMIT 页面。
- Q3 5 kHz、5%/95%、20%/80%、比例 3.0。
- Q4 750 Ω、24 V 和 rail factor。
- TFT 页面坐标、颜色、字符串。

新题若改变显示器，可以保留 scalar result 和状态机，替换 Display；改变 ADC/DDS 时，可以保留测量数学，但必须重新做 Acquire、单位和时间契约。
