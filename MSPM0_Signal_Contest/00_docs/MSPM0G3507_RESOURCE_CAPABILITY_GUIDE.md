# MSPM0G3507 资源能力指南

> 用途：先弄清 MCU 和 LaunchPad 手里究竟有什么、性能边界在哪里，再决定是否引入外置器件。选型结论请继续看 [INTERNAL_VS_EXTERNAL_SELECTION_GUIDE.md](INTERNAL_VS_EXTERNAL_SELECTION_GUIDE.md)。

状态：`OFFICIAL_DOCUMENT_VERIFIED`。本文没有声称 `BOARD_VERIFIED`；板上电压、噪声、失真和焊接状态仍须对你手中的实板确认。

## 0. 先记住这 8 句话

1. MSPM0G3507 有两路可同步工作的 12-bit、最高 4 MSPS ADC；“4 MSPS”不等于任何 2 MHz 以下信号都能高质量分析。
2. 它有一路 12-bit、最高 1 MSPS 的缓冲 DAC，适合偏置、控制电压和中低频波形；高频高纯度正弦要看每周期点数、建立时间和滤波。
3. 它有两个片上 OPA，最高典型 GBW 6 MHz，并带最高 32 倍 PGA；适合低/中频缓冲、增益和传感器调理。
4. GPAMP 只有约 0.32 MHz GBW，不能当成第三个 6 MHz OPA。
5. 三个片上 Comparator 的高速模式传播延迟典型 32 ns，适合阈值、边沿、过零和硬件触发；它不是高精度模拟测量器。
6. 芯片有 7 通道 DMA、7 个 Timer 和事件互联，能把采样/输出的时间动作从 CPU 循环中拿走。
7. MATHACL 是数学加速器，不是 DDS；它可以加速除法、平方根、乘加和三角计算，但不会自动输出波形。
8. LP-MSPM0G3507 板上另有一颗外置双运放 OPA2365；它不在 MCU 内部，默认用来缓冲 ADC0.4/PB25 的高速模拟输入。

## 1. 本文依据、版本和读数规则

本文只使用 TI 官方资料，核对日期为 2026-08-11：

- [MSPM0G3507 产品页](https://www.ti.com/product/MSPM0G3507)：当前列出 Datasheet Rev.C（2025-10-16）和 G-Series TRM Rev.E（2026-07-28）。
- [MSPM0G3507 Datasheet Rev.C](https://www.ti.com/lit/ds/symlink/mspm0g3507.pdf)：器件数量、引脚、推荐工作条件和电气参数的第一依据。
- [MSPM0 G-Series Technical Reference Manual](https://www.ti.com/lit/pdf/SLAU846)：寄存器、事件路由、FIFO、DMA 和工作流程的第一依据。
- [LP-MSPM0G3507 User Guide Rev.D](https://www.ti.com/lit/pdf/SLAU873)：板上 OPA2365、跳线、传感器、晶振和原理图的第一依据。
- [MSPM0 Academy](https://dev.ti.com/tirex/explore/node?node=A__AIblFFb5Wbkxsnu6qbPBFA__MSPM0-ACADEMY__2f1Egw1__LATEST)：官方外设学习入口。
- [MSPM0 SDK Examples Guide](https://software-dl.ti.com/msp430/esd/MSPM0-SDK/latest/docs/english/sdk_users_guide/doc_guide/doc_guide-srcs/examples_guide.html)：官方可运行外设例程索引。
- [Make System Design Easy With MSPM0 Precision Analog](https://www.ti.com/lit/an/slaae93/slaae93.pdf)：内部模拟资源和内部互联概览。
- [MSPM0 ADC Noise Analysis and Application Rev.A](https://www.ti.com/lit/an/slaaeo8a/slaaeo8a.pdf)：ADC 噪声、参考源、过采样和 MSPM0 实测方法。
- [OPA2365 产品页](https://www.ti.com/product/OPA2365)与[数据手册 Rev.G](https://www.ti.com/lit/ds/symlink/opa2365.pdf)。
- [TMP6131 产品页](https://www.ti.com/product/TMP6131)与[数据手册](https://www.ti.com/lit/ds/symlink/tmp6131.pdf)。

读表时注意：

- `typ` 是典型值，不是保证每颗芯片都达到；设计极限优先看 `min/max` 和条件。
- SNR、THD、ENOB、摆幅、建立时间都依赖供电、参考源、频率、负载和滤波；不能脱离测试条件比较一个裸数字。
- 下文写“适合”表示值得优先评估，不等于未上板就保证满足题目误差。
- 具体可用 ADC 通道、OPA 输入和复用引脚随封装而变；本仓库的 LP-MSPM0G3507 是 64 引脚 LQFP，最终仍以 SysConfig 的合法选项和板原理图为准。

## 2. 先分清 MCU INTERNAL 与 LAUNCHPAD ONBOARD

### 2.1 MCU INTERNAL：芯片内部资源

| 资源 | 数量/概要 | 核心价值 |
|---|---:|---|
| ADC0/ADC1 | 2 × 12-bit，最高各 4 MSPS | 单/双通道采集、同步采样、DMA |
| DAC12 | 1 × 12-bit，最高 1 MSPS | 偏置、控制电压、波形样本输出 |
| OPA0/OPA1 | 2，GBW 1.5/6 MHz，PGA 1～32 | 缓冲、增益、偏置、内部模拟链 |
| GPAMP | 1，GBW 约 0.32 MHz | 低速通用缓冲/调理 |
| COMP | 3，带各自 8-bit 参考 DAC | 阈值、过零、边沿、Timer fault/event |
| VREF | 1.4 V / 2.5 V | ADC/DAC/COMP 的片上参考 |
| Timer | 7 个、多类型 | 精确定时、采样触发、捕获、PWM |
| DMA | 7 通道 | ADC→RAM、RAM→DAC 等无 CPU 搬运 |
| MATHACL | DIV/SQRT/MAC/TRIG 等 | 数学运算加速；不是信号发生器 |
| Event Fabric | 发布者/订阅者互联 | Timer→ADC、COMP→Timer 等硬件链 |
| CPU/Memory | 80 MHz M0+、128 KB Flash、32 KB SRAM | 算法、控制和缓冲区的总资源上限 |

### 2.2 LAUNCHPAD ONBOARD：开发板上额外资源

| 资源 | 它在哪里 | 对比赛的价值 |
|---|---|---|
| OPA2365 双运放 | MCU 外部、LaunchPad PCB 上 | 为最高 4 MSPS ADC 输入提供高速缓冲/可改增益前端 |
| TMP6131 10 kΩ 线性热敏电阻 | 板上温度传感器电路 | 学习传感器测量；通常不是信号赛主前端 |
| 光电二极管电路 | 板上，默认跳线接片内 OPA0 | 可练习跨阻放大、弱光检测和 ADC 采样 |
| 40 MHz HFXT 晶振 | 板上 | 比内部时钟更适合需要稳定采样/计时的实验 |
| 32.768 kHz LFXT 晶振 | 板上 | RTC、低功耗和低频时基 |
| 按键、LED/RGB LED | 板上 | UI、状态指示，不是模拟测量资源 |
| XDS110 调试器 | 板上隔离区 | 下载、调试、虚拟串口和能量测量 |

板载器件不是 MCU 内部外设。换成自制板后，只有你在 PCB 上也放了它们，它们才存在。

## 3. DDS 概念纠正

### 3.1 MSPM0G3507 没有专用硬件 DDS

Datasheet 的外设清单包含 ADC、DAC、OPA、GPAMP、COMP、Timer、DMA、MATHACL 等，没有 DDS peripheral。因此：

```text
06_generator/dds
= Software DDS
= Phase Accumulator → Wave Table → sample[]
```

完整模拟输出链是：

```text
Software DDS
    ↓ uint16_t sample[]
DAC DMA + Timer/Event
    ↓
Internal DAC
    ↓
重建/低通滤波（按波形质量需要）
    ↓
模拟输出
```

MATHACL 可以帮助生成波表或计算三角函数，但它不会按固定采样率自主推进相位，也不会驱动 DAC。

### 3.2 软件 DDS + 内部 DAC 的真实上限怎么想

不要只看 `DAC max = 1 MSPS`，先算：

```text
samples_per_cycle = DAC_update_rate / output_frequency
```

例：DAC 更新率 1 MSPS 时，1 kHz 有 1000 点/周期，10 kHz 有 100 点/周期，100 kHz 只有 10 点/周期，500 kHz 只有 2 点/周期。点数越少，台阶和镜像越明显，对重建滤波、相位噪声和失真越苛刻。

## 4. ADC0 / ADC1 性能卡

### 4.1 规格总览

| 参数 | MSPM0G3507 数据 | 小白解释与比赛影响 |
|---|---|---|
| 数量 | 2 个 ADC | ADC0 与 ADC1 可被同一事件触发，实现真正双 ADC 同步采样。一个 ADC 内轮询两个通道不等同于同步。 |
| 架构 | SAR | 适合中高速数据采集；每次转换前，前端必须把内部采样电容充到足够准确。 |
| 标称分辨率 | 12/10/8 bit | 12 bit 满量程有 4096 个理想码；这不是自动等于 12 bit 有效精度。 |
| 最大采样率 | 12/10 bit 最高 4 MSPS；8 bit 最高 5.3 MSPS | 这是转换吞吐上限，不是建议的信号最高频率。波形分析还需要足够每周期采样点。 |
| 通道数 | 最多 17 个外部通道，封装相关 | 64-pin LaunchPad 可用通道需看 PinMux；不能假定所有通道都能同时用。 |
| 输入范围 | 0～VDD，且必须落在所选参考范围内 | 负电压、双极性 ±5 V/±10 V 不能直连；必须先衰减、偏置、保护或换外置 ADC。 |
| 参考源 | VDDA、外部 VREF、内部 1.4/2.5 V | VREF 的噪声和误差直接变成测量误差。内部参考方便，外部参考可为精密测量提供更好指标。 |
| INL/DNL | 典型/规格条件下约 ±2 LSB INL、±1 LSB DNL，无失码 | INL 是码值曲线弯曲；DNL 是相邻码步长不均。高精度幅值测量要把它们算进误差预算。 |
| 输入网络 | 采样电容约 3.3 pF、串联输入电阻约 0.5 kΩ | 高源阻抗或高速采样时，采样电容可能充不满；要延长采样时间或加低阻缓冲。 |
| 无 OPA 的短采样示例条件 | 低源阻条件下 62.5 ns 级 | 不代表任何传感器都能用最短采样；源阻、外部电容、通道切换都会影响建立。 |
| ENOB | 典型测试中约 10.9 bit；16 次硬件平均后约 12.3 bit | ENOB 是噪声/失真后的有效位数。产品页“250 kSPS 下 14-bit effective resolution”指硬件平均后的输出分辨率能力，不能写成 14-bit 原生 ADC。 |
| SNR | 典型约 68 dB；特定 16 次平均条件可到约 78 dB | 必须连同输入频率、参考源和滤波条件引用；比赛实测通常还受板上噪声和前端限制。 |
| 硬件平均 | 支持累加/右移，最高可提高输出分辨率 | 用采样速度换噪声；不会消除参考误差、前端失真和系统偏置。 |
| DMA/Event | 支持 | Timer 可以硬件触发 ADC，DMA 把结果搬进 `raw[N]`，抖动和 CPU 负担都比软件循环小。 |

### 4.2 采样时间为什么重要

ADC 输入不是“无限大阻抗的电压表”。采样瞬间，前端要给约 3.3 pF 的采样电容充电。源阻越大、采样时间越短、采样率越高，建立误差越大。

Datasheet 还给出经过片内模拟放大器时的采样时间量级：片内 OPA PGA 增益从 1 到 32 增大时，所需采样时间大致从 0.22 µs 增加到 2.6 µs；GPAMP 路径给出的量级约 3 µs。这说明“加了内部放大器”不等于仍可无条件满速采样。

### 4.3 什么时候先用内部 ADC

- 输入已在 0～VREF 内，单端采样即可。
- 12 bit 原生分辨率和实际 ENOB 满足误差要求。
- 最高 4 MSPS 与所需每周期点数有余量。
- 最多两路严格同步足够。
- 可以接受前端衰减/偏置或缓冲。

### 4.4 什么时候换外置 ADC

- 题目明确要求真实 16/18/24 bit 精密测量。
- 输入是双极性、±5 V/±10 V，且不希望自己做复杂前端。
- 需要 4 路以上严格同步采样。
- 需要远高于 4 MSPS 的采样率或特定抗混叠前端。
- 内部 ADC 的 ENOB、SNR、INL、参考精度或通道串扰不能满足指标。

## 5. DAC12 性能卡

| 参数 | MSPM0G3507 数据 | 小白解释与比赛影响 |
|---|---|---|
| 数量/分辨率 | 1 个，12 bit，也支持 8 bit | 12 bit 理想有 4096 级。只有一路独立模拟输出。 |
| 最高更新率 | 1 MSPS | 连续波形先算每周期点数，不把 1 MSPS 当成 1 MHz 正弦能力。 |
| 参考源 | VDDA、外部参考、内部 1.4/2.5 V | 参考决定满量程和长期准确度。 |
| 输出范围 | 近似 0～参考/供电范围，但不能把“轨到轨”理解为精确到 0 V 和 VDD | 无负载时典型更接近电源轨；带载时余量变大。最终以目标负载条件测量。 |
| 满量程建立时间 | 典型约 0.8 µs，最大约 1 µs 到规定误差带（对应测试条件） | 1 MSPS 时点间隔正好 1 µs，满幅跳变几乎用完全部建立时间。高频大幅波形会更困难。 |
| Slew rate | 典型约 5.5 V/µs | 正弦需要 `SR ≥ 2πfVpk`；高频或大幅度先检查。 |
| 输出电流 | 约 ±1 mA 推荐量级 | 适合高阻控制输入，不适合直接驱动低阻负载、扬声器或大电容。 |
| 输出电阻 | 典型约 1.2 Ω，规格最大约 10 Ω | 负载电流会造成压降；外接缓冲可隔离负载。 |
| 负载电容 | 规格测试到约 100 pF | 长线/大电容可能振铃或变慢，应隔离或外加稳定驱动器。 |
| INL/DNL | 约 ±4 LSB INL、±1 LSB DNL | 精密 DC 输出不能只按理想码值计算，需校准和实测。 |
| Offset/Gain error | 校准后偏置可到数 mV 量级；增益误差约百分比量级规格 | 1.650 V 控制通常够用；若题目要 mV 级绝对精度，要实测标定或外置精密 DAC/参考。 |
| 动态性能示例 | 4 kHz、1 MSPS、外部参考和规定滤波下 SNR 约 80.9 dB、SINAD 约 71.1 dB | 这是特定测试条件，不代表 100 kHz 输出仍相同。 |
| FIFO/DMA/Event | 4 × 12-bit FIFO，支持 DMA 和内部定时 | 能由 Timer/DMA 连续送样，避免 `while + delay` 的抖动。 |

### 5.1 它适合什么

- 固定 DC bias、比较器阈值以外的模拟设定值。
- VGA/PGA 控制电压、偏置或慢速闭环控制。
- 中低频任意周期波，特别是每周期样本充足、幅度不大且允许滤波时。
- 内部连接到 OPA、ADC、COMP 的自检/闭环实验。

### 5.2 它不擅长什么

- 直接驱动大电流、低阻或大电容负载。
- 高纯度、高频、宽带任意波形。
- 无校准的高精度 DC 基准源。
- 需要两路独立同步模拟输出的题目。

## 6. OPA0 / OPA1 性能卡

### 6.1 规格总览

| 参数 | MSPM0G3507 内部 OPA | 意义 |
|---|---|---|
| 数量 | 2 | 可做两级信号链，也会与引脚/内部路由资源竞争。 |
| 供电 | 随 MCU 模拟供电工作 | 输入输出必须留在单电源允许范围，不能直接处理负电压。 |
| 输入共模 | RRI 模式相关，典型范围可接近地；上端通常需离 VDD 约 0.3～1.1 V | “Rail-to-rail”仍有工作模式和条件，靠近上电源轨时先核表。 |
| 输出摆幅 | 10 kΩ 到中点负载时典型距电源轨约 20 mV，最大保证余量更大 | 大负载、温度和工差都会减少可用摆幅。 |
| GBW 模式 | 典型 1.5 MHz / 6 MHz | 闭环增益越高，可用带宽越低。 |
| Slew rate | 典型约 1.3 / 4.9 V/µs | 限制大幅高频波形变化速度。 |
| 输入失调 | 斩波关闭时约 ±0.4 mV 典型、±2 mV 最大；斩波开启典型更低 | 低电平放大时，失调也被增益放大。 |
| 失调漂移 | 斩波关闭约数 µV/°C；斩波开启典型约 0.5 µV/°C | 低频、温漂敏感测量适合评估斩波。 |
| 输入偏置 | 25°C、斩波关闭典型/规格为 pA 级；斩波开启会增大到亚 nA 量级 | 超高源阻传感器要核对斩波导致的偏置和开关效应。 |
| 电压噪声 | 低 GBW、斩波关闭的典型值约 240 nV/√Hz@1 kHz、88 nV/√Hz@10 kHz | 明显高于专用低噪声外置运放；弱信号题要做噪声预算。 |
| THD+N 示例 | 低/高 GBW 的规定频率条件下约 0.0034% / 0.004% | 是典型条件值，频率、增益、输出摆幅改变后要实测。 |
| 输出驱动 | 低/高 GBW 模式约 ±9 mA / ±30 mA 量级，条件相关 | 比 DAC 强，但仍不是功率驱动器。 |
| 负载电容 | 规格条件约 40 pF | 大电容、长线要隔离和验证稳定性。 |
| PGA | Buffer、2、4、8、16、32 倍等离散档位 | 不是任意浮点增益。增益越高，误差和带宽越需要检查。 |
| PGA gain error | 低倍较小，随增益上升；32 倍可到百分比量级 | 精确增益不能只信名义档位，应通过 ADC/基准校准。 |
| Chopper | 支持标准/ADC 辅助斩波；斩波频率随增益变化 | 可降失调/漂移和低频噪声，但会带来斩波纹波/偏置变化，不是高速信号万能开关。 |
| 内部路由 | 可接 DAC、另一 OPA、GPAMP、ADC、COMP 等 | 少走板外线是内部 OPA 的最大优势之一，但路由和引脚必须由 SysConfig/手册确认。 |

### 6.2 6 MHz GBW 到底意味着什么

GBW 不是“能把 6 MHz 信号放大 32 倍”。单极点近似下：

```text
closed_loop_bandwidth ≈ GBW / closed_loop_gain
```

比赛现场可先用保守经验检查：

```text
required_GBW ≈ signal_frequency × closed_loop_gain × margin
margin 初查取 10
```

`10× margin` 是工程经验，不是 TI 的保证规格。它只是帮助你快速淘汰明显不够的方案；最终还要看实际允许幅频误差、相位误差、稳定性和上板结果。

同时检查压摆率：

```text
required_SR = 2π × signal_frequency × output_peak_voltage
```

#### 例 1：100 kHz，Gain = 2

```text
required_GBW ≈ 100 kHz × 2 × 10 = 2 MHz
```

1.5 MHz 模式余量不足；6 MHz 模式值得优先评估。若输出峰值 1 V，`required_SR ≈ 0.63 V/µs`，低于 4.9 V/µs 典型值。结论：内部 OPA 高 GBW 模式有希望，仍要核对输入共模、输出摆幅、增益误差和相位指标。

#### 例 2：500 kHz，Gain = 10

```text
required_GBW ≈ 500 kHz × 10 × 10 = 50 MHz
```

内部 OPA 只有 6 MHz 典型 GBW，明显不应强行使用。即使输出峰值 1 V 的 `required_SR ≈ 3.14 V/µs` 没超过 4.9 V/µs，GBW 已先失败。应优先评估板载 50 MHz OPA2365 或更高带宽外置 OPA。

## 7. GPAMP 性能卡

GPAMP 必须单独看，不能把它理解成“OPA2”。

| 参数 | GPAMP 典型/规格量级 | 与内部 OPA 的差异 |
|---|---:|---|
| 数量 | 1 | 额外一路低速模拟调理资源 |
| GBW | 约 0.32 MHz | 远低于内部 OPA 的 1.5/6 MHz |
| Slew rate | 约 0.32 V/µs | 大幅高频信号更早失真 |
| 建立时间 | 约 9 µs 到 0.1%（条件相关） | 不适合快速多路切换后立即高精度采样 |
| 输入失调 | 斩波关闭最大可到数 mV；斩波开启典型/最大显著降低 | 低频 DC/传感器可利用斩波，仍要校准 |
| 漂移 | 斩波开启典型约 0.34 µV/°C | 低频稳定性是其强项之一 |
| 电压噪声 | 约 43 nV/√Hz@1 kHz、19 nV/√Hz@10 kHz | 噪声密度不差，但带宽和驱动较低 |
| THD+N 示例 | 约 0.012% | 高保真要求不如高速外置 OPA |
| 输出驱动 | 约 4 mA | 不用于重负载 |
| 负载电容 | 规格条件约 200 pF | 仍需按负载验证稳定性 |
| ADC 路径采样时间 | 约 3 µs 量级 | 会限制高采样率/通道吞吐 |

优先使用：慢速传感器缓冲、DC 偏置、低频调理、内部模拟资源紧张时的辅助通道。

慎用：几十 kHz 以上且有增益/幅相精度要求的信号。

不要用：把它当 6 MHz OPA 处理 100 kHz～MHz 级高增益信号。

## 8. LaunchPad 板载 OPA2365

### 8.1 为什么板上要放它

LP-MSPM0G3507 User Guide 明确说明，外置 OPA2365 用来演示/评估 ADC 最高 4 MSPS 性能。高速 SAR ADC 的采样电容需要低阻、快速建立的驱动源；OPA2365 的 50 MHz GBW、25 V/µs 压摆率和快速建立能力比片内 OPA/GPAMP 更适合作为高速 ADC 缓冲。

### 8.2 默认连接和怎么接信号

- OPA2365 是双运放，板上默认把它配置为 ADC 输入通道的 buffer 路线，并预留电阻/电容焊盘供改增益或滤波。
- OPA2365 输出经 `R72 = 100 Ω` 接到 `PB25 / ADC0.4`；板上还并有 `C47 = 82 pF`，形成 ADC 驱动/隔离网络。
- 模拟输入网名为 `PB25-`，可从底部 pin extension 访问；P1 同轴连接器位置默认未装，可按原理图自行装配。
- `J13` 默认安装，给热敏电阻与 OPA2365 模拟电路供 3.3 V。J13 不是“buffer/bypass 选择跳线”。
- 如果完全不用 OPA2365，User Guide 原理图注释要求拆下 R72，并在 R74 位置装 0 Ω，才把 `PB25-` 直接旁路到 PB25/ADC0.4。
- 想改变增益，必须按 OPA2365 拓扑和板上预留焊盘重配反馈电阻；这不是改 SysConfig 就能完成。

第一次使用前，用万用表断电确认当前板版本、R72/R74 实装状态和输入节点，不根据网图猜焊接状态。

### 8.3 OPA2365 官方性能摘要

| 参数 | OPA2365 | 意义 |
|---|---:|---|
| 通道 | 2 | 板上可做两级缓冲/调理；默认连接以板原理图为准 |
| 供电 | 2.2～5.5 V；LaunchPad 模拟电路为 3.3 V | 是板外器件供电，不由 OPA SysConfig 配置 |
| GBW | 50 MHz typ | 适合驱动 4 MSPS SAR ADC；高增益时仍要按 GBW/Gain 下降 |
| Slew rate | 25 V/µs typ | 比内部 OPA 4.9 V/µs 快约一个数量级 |
| 建立时间 | 0.3 µs 到 0.01% typ | 对高速 ADC 驱动有利 |
| 输入电压噪声 | 4.5 nV/√Hz typ | 远低于内部 OPA 的典型噪声密度 |
| 失调 | 100 µV 级典型；产品表最大 0.2 mV@25°C | 低电平增益误差更小，但仍要算闭环增益后的影响 |
| 输入偏置 | 0.2 pA typ、产品表最大 10 pA | 适合较高源阻，但 PCB 污染/漏电也会变重要 |
| 输入/输出 | Rail-to-rail input/output；具体摆幅取决于负载 | 不等于在任意负载下都精确达到 0/3.3 V |
| 输出电流 | 约 65 mA typ | 驱动能力强于片内 OPA，但热、线性和稳定性仍要按手册设计 |
| THD+N | 0.0004% typ（规定条件） | 适合低失真前端；题目频率/幅值条件改变后需实测 |

### 8.4 三者快速比较

| Resource | GBW typ | Slew rate typ | 主要用途 |
|---|---:|---:|---|
| Internal OPA0/1 | 1.5 / 6 MHz | 1.3 / 4.9 V/µs | 低/中频缓冲、PGA、偏置、内部直连 ADC |
| Internal GPAMP | 0.32 MHz | 0.32 V/µs | 低频传感器、慢速辅助调理 |
| Board OPA2365 | 50 MHz | 25 V/µs | 高速 ADC 驱动、中高频低失真缓冲/放大 |

## 9. Comparator 性能卡

| 参数 | MSPM0G3507 | 意义 |
|---|---|---|
| 数量 | 3 | 可做多个阈值、窗口比较或边沿链，受 PinMux/内部路由约束 |
| 模式 | High-speed / Low-power | 高速模式换更短延迟；低功耗模式延迟更大 |
| 传播延迟 | 高速典型 32 ns、最大约 50 ns（100 mV overdrive、滤波关闭条件）；低功耗约 1.2 µs typ、4 µs max | 过零/边沿时间会随 overdrive、噪声和模式改变；高精度相位需实测延迟 |
| 输入范围 | 0～VDD 量级 | 负压/超供电输入不能直接接 |
| 输入失调 | 最大约 ±20 mV | 对小信号“零点”会形成明显阈值误差，不能把它当零误差检测器 |
| Hysteresis | 关闭或约 10/20/30 mV 档 | 抑制阈值附近抖动，但会改变上升/下降翻转点 |
| Reference DAC | 每个 COMP 内置 8-bit DAC，约 `(code+1)/256 × reference` | 适合可调阈值，不等同于主 12-bit DAC 的精度 |
| 滤波 | 可编程数字/模拟去毛刺功能 | 抗噪更强但增加延迟，测相位/脉宽要计入 |
| 内部路由 | 可接 OPA/GPAMP/DAC/VREF，并发布 Event、Timer fault | 可不经 GPIO 把边沿送到 Timer Capture，减少接线和软件延迟 |

直接使用内部 Comparator 的典型场景：

- 逻辑幅度足够的方波测频。
- 正弦经偏置后做一般过零/边沿计时。
- ADC/DMA 的硬件触发、保护阈值、窗口判断。
- 允许通过校准或测量消除固定传播延迟的相位/时间题。

考虑外置比较器的情况：

- 输入是负压、高压或差分范围，内部输入范围不满足。
- 小信号阈值误差不能容忍 ±20 mV 级失调。
- 需要远小于几十 ns 的传播延迟或更低的 propagation-delay dispersion。
- 需要开漏/高压接口、多通道数量或专用零交越保护拓扑。

LM339 类器件不是自动“更高速”；必须按具体型号的传播延迟、输入共模和输出类型比较。

## 10. VREF 性能卡

| 参数 | MSPM0G3507 | 影响 |
|---|---|---|
| 档位 | 1.4 V 或 2.5 V | 选择更低满量程可提高小信号每 LSB 对应的电压分辨率，但输入不能超范围 |
| 初始准确度 | 1.4 V 约 1.38～1.42 V；2.5 V 约 2.46～2.54 V | 不校准时已有约百分比级范围，精密幅值测量不能只写死 1.400/2.500 |
| 温漂 | 约 80 ppm/°C 量级 | 温度变化会造成比例误差 |
| 输出能力 | VREF+ 输出负载约 100 µA 量级 | 只作参考，不当外部模块电源或通用偏置源 |
| 外部电容 | 典型要求 1 µF，允许范围按 Datasheet | 电容类型、容值和布线影响稳定与噪声 |
| 启动 | 约 200 µs 量级 | 启动后要等参考稳定再进行精密转换 |

需要高绝对精度、低温漂、低噪声时，优先评估外部精密参考，并把板布局、去耦和校准一起设计。

## 11. Timer、DMA、Event 和 MATHACL

### 11.1 Timer

芯片有 7 个 Timer：包含 16-bit advanced timer、通用 timer、低功耗 timer 和一个 32-bit timer。它们可以做：

- 固定 `Fs` 周期事件。
- Comparator/GPIO 边沿捕获和测频。
- PWM、互补输出、死区和 QEI。
- 触发 ADC/DAC，而不是 CPU 中 `delay()`。

边界：Timer tick 分辨率、计数位宽、时钟误差和捕获溢出共同决定测频范围。使用前同时算最高频最小 tick 数和最低频最大周期计数。

### 11.2 DMA

7 通道 DMA 的核心价值不是“让算法更快”，而是按硬件请求可靠搬数据：

```text
ADC result → DMA → raw[N]
wave[N] → DMA → DAC FIFO
```

边界：只有 7 个通道；ADC 双通道、DAC、UART/SPI 等会竞争。传输宽度、源/目的地址递增和触发源必须匹配。

### 11.3 Event Fabric

事件互联让发布者和订阅者不经过 CPU：

```text
Timer Event → ADC start
Comparator Event → Timer Capture
Timer Event → DAC update
```

它减少软件抖动，但 publisher/subscriber channel 和实例资源会冲突；修改 SysConfig 后要查 [RESOURCE_CONFLICT_GUIDE.md](RESOURCE_CONFLICT_GUIDE.md)。

### 11.4 MATHACL

官方列出的能力包括 DIV、SQRT、乘法/平方、MAC、SINCOS、ATAN2 等。它适合加速算法中的数学内核。它不拥有采样时钟、相位累加器、波表地址推进或 DAC 输出，所以不是 DDS peripheral。

## 12. LaunchPad 其他模拟/时钟资源

### 12.1 TMP6131 热敏电阻

板上是 10 kΩ 线性 PTC TMP6131，与精密上拉组成分压；室温附近输出约 1.6 V。J9 默认把它接到 PB24/ADC0.5，也可切换到 GPAMP 相关路径。它适合学习传感器测量，不应当成板温“高精度已校准值”。

### 12.2 光传感器

板上是光电二极管电路，默认通过 J16/J17/J18 接片内 OPA0，OPA0 以跨阻形式把光电流变为电压，再内部送 ADC。占用 PA22/PA26/PA27 相关 OPA0 路径；不用时应断开跳线，避免与比赛前端抢资源。

### 12.3 晶振

- Y1：32.768 kHz、12.5 pF。
- Y2：40 MHz、12.5 pF。

当题目看重频率准确度、相位或长时间采样稳定性时，外部 40 MHz HFXT 通常比默认内部振荡器更值得评估；但最终系统误差仍包括信号源和测量时基误差。

## 13. 资源使用前的 60 秒检查

| 资源 | 先回答 |
|---|---|
| ADC | 输入是否 0～VREF？分辨率/ENOB够吗？`Fs / fmax` 有多少点/周期？源阻能否在采样时间内建立？ |
| DAC | 每周期多少点？`2πfVpk` 是否小于 SR？负载电流/电容多大？绝对精度要不要校准？ |
| OPA | `f × gain × 10` 与 GBW 比如何？输出峰值需要多少 SR？共模/摆幅/噪声够吗？ |
| GPAMP | 0.32 MHz GBW 和约 3 µs ADC 路径采样时间是否明显限制目标？ |
| COMP | 输入范围、±20 mV 级失调、hysteresis 和传播延迟是否可接受？ |
| VREF | 初始误差、温漂和噪声是否进入题目误差预算？ |
| Timer | tick 分辨率、位宽、时钟准确度和资源是否够？ |
| DMA | 通道数、方向、宽度和触发源是否冲突？ |
| LaunchPad OPA2365 | R72/R74/J13 当前焊接状态是否确认？输入是否真的接到 PB25-/ADC0.4？ |

如果内部资源满足指标并有余量，优先内部；如果任何一项明确失败，就去 [INTERNAL_VS_EXTERNAL_SELECTION_GUIDE.md](INTERNAL_VS_EXTERNAL_SELECTION_GUIDE.md) 选择外置方案。
