# 内部资源还是外置器件：比赛快速选型指南

> 这份文件回答“内部够不够”。查具体性能数字看 [MSPM0G3507_RESOURCE_CAPABILITY_GUIDE.md](MSPM0G3507_RESOURCE_CAPABILITY_GUIDE.md)，确定模块后再看模块 README。

状态：`OFFICIAL_DOCUMENT_VERIFIED`，不是 `BOARD_VERIFIED`。所有边界方案必须用题目实际幅值、负载、频率和你的实板复测。

## 0. 第一页快速判断卡

| 题目说 | 第一候选 | 立即检查 |
|---|---|---|
| 输出 1.65 V 偏置 | Internal DAC | 负载是否高阻、绝对误差是否允许校准 |
| 控制 VGA 增益 | Internal DAC | VGA 控制端范围、输入阻抗、所需电压分辨率 |
| 100 kHz 以下简单正弦 | Software DDS + Internal DAC + DMA | 每周期点数、幅度、建立时间、允许 THD、重建滤波 |
| 500 kHz 高质量正弦 | External DDS / High-speed DAC | 输出幅度、SFDR/THD、滤波与时钟质量 |
| 500 kHz 信号、Gain=10 | OPA2365 或外置高速 OPA | `f × gain × 10 ≈ 50 MHz`；内部 OPA 6 MHz 余量明显不足 |
| 低频传感器、Gain=16 | Internal OPA PGA | 输入共模、输出摆幅、失调、斩波和采样时间 |
| 500 kSPS ADC 采样率 | Internal ADC 值得先评估 | 注意是采样率还是 500 kHz 输入信号；前者通常容易，后者要更高 Fs |
| 16-bit 精密幅值 | External precision ADC | 不把硬件平均后的输出位数当成真实 16-bit 精度 |
| 方波/边沿测频 | Internal COMP + Timer Capture | 输入范围、阈值、传播延迟、hysteresis |
| 低电平高精度过零 | Precision/high-speed external COMP 值得评估 | 内部 COMP 最大 ±20 mV 级失调可能不够 |
| 4 路以上严格同步采样 | External simultaneous ADC family | 通道数、输入范围、接口带宽、同步时钟 |

默认原则只有两句：

1. 内部资源满足指标且留有合理余量时，优先内部：接线少、SysConfig 可配置、Timer/Event/DMA 可内部互联、故障点少。
2. 带宽、采样率、分辨率、输入范围、驱动、噪声、失真或通道数有一项明确不满足，就换外置器件，不为“省一颗芯片”强行内部。

## 1. 五步资源检查

拿到题目后，ADC、DAC、OPA、DDS、Comparator 都按同一顺序：

### STEP 1：把题目指标变成数字

至少写出：

- 输入/输出最小、最大电压；是否有负电压或双极性。
- 最高信号频率、目标采样率/更新率。
- 增益、输出峰值、负载电阻/电容。
- 幅值/频率/相位/THD 的允许误差。
- 通道数、是否严格同步、连续还是单帧。

### STEP 2：先做四个快算

```text
ADC 每周期点数 = Fs / f_signal_max
DAC 每周期点数 = Fupdate / f_output
OPA required_GBW ≈ f_signal × closed_loop_gain × 10
OPA/DAC required_SR = 2π × f_signal × Vout_peak
```

`GBW ×10` 是初筛经验，不是 TI 硬规格。

### STEP 3：检查范围和精度

- ADC/COMP 输入是否在 0～VREF/VDD 合法范围。
- DAC/OPA 输出是否留出摆幅，不贴着电源轨。
- 分辨率之外，ENOB、INL、offset、gain error、VREF 是否满足。

### STEP 4：检查系统资源

- Timer、DMA channel、Event publisher/subscriber 有没有冲突。
- 32 KB SRAM 能否容纳 raw/float/FFT/workspace。
- LaunchPad 上的 J9/J13/J16～J18、R72/R74 是否占用或改变模拟路径。

### STEP 5：形成三态结论

| 状态 | 含义 | 行动 |
|---|---|---|
| `INTERNAL_PREFERRED` | 内部规格满足并有余量 | 选内部模块，尽快最小上板验证 |
| `INTERNAL_NEEDS_TEST` | 纸面可行但余量/失真/负载不确定 | 用示波器/信号源做针对性验证，同时准备外置备选 |
| `EXTERNAL_REQUIRED` | 有明确规格失败 | 不再耗时强行内部，进入外置器件路线 |

## 2. 内部 ADC 还是外置 ADC

### 2.1 类型比较

| 方案 | 分辨率/采样率 | 通道/同步 | 输入范围/双极性 | MCU 接口 | DMA 难度 | PCB 难度 | 比赛开发时间 | 默认判断 |
|---|---|---|---|---|---|---|---|---|
| MSPM0 internal ADC | 12-bit、最高 4 MSPS；实际 ENOB 低于标称位数 | 两个 ADC 可同步；外部通道数封装相关 | 单端 0～VREF/VDD，不支持负压直入 | 内部寄存器/Event | 低，官方 ADC DMA 链 | 低 | 最短 | 题目未明确超规格时先评估 |
| External SPI SAR ADC | 具体型号常见 12～18 bit、数百 kSPS～数 MSPS | 常见 1～2 路；是否 simultaneous 看型号 | 单端/差分/范围看型号，通常仍需驱动与参考 | SPI + CS/DRDY | 中，需满足 SPI 吞吐和帧时序 | 中，参考/驱动/去耦重要 | 中 | 内部 ENOB/INL 不够但通道不多 |
| Parallel high-speed ADC | 具体型号常为高速、多 MSPS 以上 | 型号相关 | 型号相关，常需专用驱动 | 并行数据+时钟 | 高，MSPM0 GPIO/DMA 吞吐可能先成为瓶颈 | 高，高速时钟/回流/终端 | 长，甚至需 FPGA | 明确需要 >4 MSPS 或宽带时 |
| AD7606-family simultaneous ADC | 分辨率/速率随具体后缀变化 | 典型定位是多路同步 | 系列常提供工业双极性范围；必须查完整型号 | CONVST/BUSY + 串行或并行 | 中高，每次转换要搬完整一帧多通道 | 中高，模拟供电/参考/布局更多 | 中长 | 多路同步、±输入范围题 |
| High-resolution Delta-Sigma ADC | 具体型号常见 16～24 bit、较低数据率并有数字滤波延迟 | 1～多路，是否同步看型号 | 差分、PGA、低噪声型号常见 | SPI/I2C + DRDY | 中，重点是滤波建立/DRDY，不只吞吐 | 中，低噪声布局和参考关键 | 中 | 称重、桥式、毫伏、精密 DC；不用于高速瞬态 |

“常见”只描述器件类别，不能替代具体型号 Datasheet。

### 2.2 题目要求 → 推荐

| 要求 | 默认推荐 | 原因 |
|---|---|---|
| 1～2 路、0～3.3 V、≤12-bit 需求、≤4 MSPS | Internal ADC | 内部触发/DMA最简单 |
| 双通道严格同步、速率在内部范围 | ADC0 + ADC1 simultaneous | 不必为“两路同步”立即上外置 ADC |
| 500 kSPS 采样率 | Internal ADC | 只占 4 MSPS 上限的 1/8，重点转为前端建立和实际信号带宽 |
| 500 kHz 输入正弦 | Internal ADC 可评估，但 Fs 不能仍为 500 kSPS | Nyquist 至少 >1 MSPS；频谱/幅相通常希望 2～4 MSPS 或更高点数 |
| 4 路/8 路严格同步 | AD7606-family 或其他 simultaneous ADC | 两个内部 ADC 不够通道 |
| ±5 V / ±10 V 工业输入 | 带相应输入范围的外置 ADC，或先做可靠模拟前端 | 内部 ADC 不能直接接双极性/高压 |
| 16-bit 精密 AC/DC | External SAR/Delta-Sigma | 12-bit 标称和平均输出位数不能替代真实精度/ENOB |
| >4 MSPS | External high-speed ADC | 内部吞吐明确失败 |
| 低频毫伏/桥式传感器 | External Delta-Sigma/PGA ADC，或先验证片内 OPA+ADC | 看精度、噪声和校准要求；高位 ΔΣ 通常更直接 |

### 2.3 不要犯的错误

- 把 `14-bit effective resolution with averaging` 写成原生 14-bit、能做任意 14-bit AC 测量。
- 只满足 Nyquist 就认为波形、FFT、幅相都足够。2 点/周期只能说明没有违反最基本采样定理，不能保证工程质量。
- 输入超过 0～VREF 时只在代码里做缩放。模拟端已经过压/削顶，软件救不回来。
- 外置 ADC 只看位数，不看参考、驱动、采样建立、接口带宽和 PCB。

## 3. 内部 DAC、外置 DAC 还是外置 DDS

### 3.1 先用每周期点数而不是“频率标签”

内部 DAC 最高 1 MSPS。假设实际使用 `Fupdate = 1 MSPS`：

| 输出频率 | 理想每周期点数 | 初步判断 |
|---:|---:|---|
| 固定 DC | 不适用 | Internal DAC 优先 |
| 1 kHz | 1000 | 非常宽松，内部 DAC 优先 |
| 10 kHz | 100 | 通常宽松，内部 DAC 优先评估 |
| 100 kHz | 10 | 边界/质量依赖强；必须验证建立、镜像、滤波和 THD |
| 500 kHz | 2 | 不适合高质量正弦；外置 DDS/高速 DAC |
| MHz 级 | <1 | 内部 DAC 无法按普通采样波形方式完成 |

如果实际更新率低于 1 MSPS，点数还要进一步减少。

### 3.2 方案比较

| 方案 | 优点 | 主要边界 | 适合 |
|---|---|---|---|
| Internal DAC direct code | 最简单、无外部器件 | 12 bit 精度、±1 mA 量级负载、校准/参考误差 | DC bias、VGA/PGA control |
| Software DDS + Internal DAC DMA | 任意波表、频率/相位由软件控制、内部 Timer/DMA | 1 MSPS、建立时间、CPU/RAM 生成块、重建滤波 | 低/中频波形、扫频、自测 |
| External SPI DAC | 更高分辨率/精度/多通道可选 | SPI 吞吐和更新率；输出驱动仍看型号 | 精密 DC、慢波形、多路控制 |
| External high-speed DAC | 高更新率、宽带任意波 | 高速接口、时钟、PCB、重建滤波复杂 | 高速任意波/调制 |
| External DDS | 独立高速相位累加和 DAC，MCU 只写控制字 | 波形种类、幅度控制、杂散、模块时钟和滤波 | 高质量/高频正弦、快速扫频、稳定频率源 |

### 3.3 场景结论

| 场景 | 第一候选 | 何时升级 |
|---|---|---|
| Fixed DC / 1.65 V | Internal DAC | 要求绝对精度超出校准能力、需要更多通道或更强驱动 |
| 控制电压 | Internal DAC | 控制端范围/分辨率不匹配或需要隔离/双极性 |
| 1 kHz 波形 | Software DDS + Internal DAC DMA | 极低失真/超高动态范围题 |
| 10 kHz 波形 | Software DDS + Internal DAC DMA | THD/SFDR/幅度实测不够 |
| 100 kHz 波形 | `INTERNAL_NEEDS_TEST` | 高质量/低失真时直接准备外置 DDS/DAC |
| 500 kHz 波形 | External DDS/high-speed DAC | 仅粗糙方波可考虑 Timer PWM，不把它冒充高质量正弦 |
| MHz 波形 | External DDS/high-speed DAC | 内部 DAC 不作为主候选 |
| Precision DC | External precision DAC + reference，或内部 DAC 校准后实测 | 由允许误差决定，不只看 12/16 bit 标签 |

## 4. 内部 OPA、GPAMP、板载 OPA2365 还是另加 OPA

### 4.1 必须先算的五件事

1. `required_GBW ≈ fmax × closed_loop_gain × 10`。
2. `required_SR = 2π × fmax × Vout_peak`。
3. 输入共模有没有落在器件合法范围。
4. 输出峰值和负载是否留出摆幅/驱动余量。
5. offset × gain、noise、gain error、THD 是否进入题目误差。

### 4.2 资源比较

| 资源 | GBW / SR 典型 | 强项 | 慎用/不用 |
|---|---|---|---|
| Internal OPA0/1 | 1.5/6 MHz；1.3/4.9 V/µs | 低/中频 buffer、离散 PGA 1～32、bias、内部接 ADC/COMP/DAC、低漂移斩波 | 高频 × 高增益、极低噪声、高精度任意增益、重负载 |
| Internal GPAMP | 0.32 MHz；0.32 V/µs | 低频传感器、慢速 buffer、辅助调理 | 不当 6 MHz OPA；高频或快速 ADC 驱动 |
| Board OPA2365 | 50 MHz；25 V/µs | 高速 ADC 驱动、中高频低失真缓冲/增益、低噪声 | 板上增益需改焊反馈网络；不是 SysConfig 资源；50 MHz 对 500 kHz×10 只是经验初筛边界 |
| External precision OPA | 型号相关 | 超低 offset/drift/noise、精密 DC/低频 | 可能带宽/驱动不高；必须按目标型号选 |
| External high-speed OPA | 型号相关 | 高 GBW/SR、ADC driver、MHz 信号链 | 稳定性、布局、去耦、输出共模和功耗更难 |

### 4.3 DEFAULT / USE WHEN / DON'T USE WHEN

#### Internal OPA

- **DEFAULT**：低/中频 buffer、Gain 2/4/8/16/32 的简单 PGA、片内 DAC bias、内部 ADC 路由。
- **USE WHEN**：GBW/SR 初算有余量，共模/摆幅合法，精度可校准。
- **DON'T USE WHEN**：`f × gain × 10` 明显超过 6 MHz，或噪声/失真/驱动明确不够。

#### GPAMP

- **DEFAULT**：低速传感器或 DC 辅助通道。
- **USE WHEN**：带宽远低于 0.32 MHz 且能接受约微秒级建立。
- **DON'T USE WHEN**：100 kHz 高增益、4 MSPS 前端驱动或精密高速幅相。

#### Board OPA2365

- **DEFAULT**：LaunchPad 上评估高速 ADC0.4/PB25 输入。
- **USE WHEN**：内部 OPA 带宽/噪声不够，且板上 PB25-/R72/R74 路径可以接受或可以焊接修改。
- **DON'T USE WHEN**：未确认板上焊接状态、需要另一 ADC pin、需要超过其供电/输入/输出/稳定性范围。

#### External OPA

- **DEFAULT**：只有内部与板载方案出现明确瓶颈才引入。
- **USE WHEN**：需要专用低噪声、低漂移、高压、差分、轨到轨、高速或高驱动。
- **DON'T USE WHEN**：只因为“外置看起来更专业”，却没有算指标、没有布局时间和验证方案。

### 4.4 内部 PGA 还是外置 PGA/VGA

片内 OPA PGA 的优势是内部路由和离散增益档位，通常可选 buffer、2、4、8、16、32 倍；它不是任意连续可调增益。

| 需求 | 第一候选 |
|---|---|
| 低/中频、离散 1～32 倍、可接受校准 | Internal OPA PGA |
| 需要频繁切换且内部档位正好覆盖 | Internal OPA PGA |
| 需要更细/更宽增益范围、差分输入或远高于 6 MHz 的增益带宽 | Exact external PGA/VGA |
| 需要模拟控制增益 | External VGA；Internal DAC 可产生控制电压 |
| 需要严格增益准确度、噪声或失真 | 按具体 external PGA/OPA Datasheet 选型并校准 |

板载 OPA2365 本身不是数字 PGA；改变其闭环增益通常要修改板上反馈电阻。

## 5. 软件 DDS 还是外置 DDS

### 5.1 方案表

| 方案 | 频率/分辨率 | 相位/幅度 | 谱纯度和 MCU 负担 | 接口难度 | 第一用途 |
|---|---|---|---|---|---|
| Software DDS + Internal DAC | 受 1 MSPS DAC 和每周期点数限制；相位步进由软件累加器决定 | 软件相位灵活；幅度/offset 可按样本缩放 | 高频时镜像/台阶明显；CPU 生成块、DMA 输出 | 低 | 中低频任意波、快速搭建、自测 |
| AD9833 class | 官方 AD9833 为 25 MHz MCLK、28-bit tuning，0～12.5 MHz 标称输出范围 | 可编程频率/相位；正弦/三角/方波；无通用数字幅度寄存器 | 独立 DDS，MCU 只写控制字；仍需看模块时钟、滤波和输出幅度 | 低，3-wire SPI | kHz～低 MHz 稳定信号、扫频 |
| AD9850 class | 官方 AD9850 为 125 MHz clock、32-bit tuning | 5-bit phase control；模拟正弦和 comparator 方波 | 更高频，官方给出 40 MHz 输出时 DAC SFDR >50 dB 的条件指标；模块滤波很关键 | 中，串行或并行装载 | 更高频正弦/时钟/扫频 |
| Higher-speed DDS | 具体型号可达更高时钟/更好 SFDR | 常有幅相/调制功能 | 性能高，时钟、供电、PCB 和滤波难度也高 | 高 | MHz～RF、严格杂散/相噪要求 |

AD9833 官方资料：[产品页](https://www.analog.com/en/products/ad9833.html)、[Datasheet Rev.G](https://www.analog.com/media/en/technical-documentation/data-sheets/AD9833.pdf)。AD9850 官方资料：[产品页](https://www.analog.com/en/products/ad9850.html)、[Datasheet Rev.H](https://www.analog.com/media/en/technical-documentation/data-sheets/ad9850.pdf)。

### 5.2 DEFAULT / USE WHEN / DON'T USE WHEN

#### Software DDS + Internal DAC

- **DEFAULT**：1～10 kHz 级或每周期样本充分的波形；需要任意波表、幅度/offset 软件控制。
- **USE WHEN**：1 MSPS 内有足够每周期点数，DAC 建立/负载/THD 实测满足，CPU/RAM资源可接受。
- **DON'T USE WHEN**：500 kHz 高质量正弦、MHz 输出、严格 SFDR/相噪。

#### AD9833 class

- **DEFAULT**：需要比内部 DAC 更高输出频率，又希望 SPI 接口简单。
- **USE WHEN**：正弦/三角/方波已够，频率和相位控制是重点，幅度可以由外部 VGA/PGA/衰减控制。
- **DON'T USE WHEN**：需要任意波形、独立高精度数字幅度控制，或目标频率接近时钟边界且谱纯度要求高。

#### AD9850 class / high-speed DDS

- **DEFAULT**：数 MHz 到更高频的正弦/扫频候选。
- **USE WHEN**：需要更高参考时钟、细频率步进和更高模拟输出频率。
- **DON'T USE WHEN**：没有合格时钟、供电去耦、低通滤波和高速布局；廉价模块的实装时钟/滤波也必须核对。

## 6. 内部 Comparator 还是外置 Comparator

| 要求 | 内部 COMP 判断 | 外置优先条件 |
|---|---|---|
| 方波频率、一般边沿 | `INTERNAL_PREFERRED` | 输入电平超范围或需特殊输出接口 |
| 正弦过零、一般测频 | `INTERNAL_NEEDS_TEST` | 小信号、噪声大、±20 mV 级 offset 造成不可接受误差 |
| 硬件触发/保护 | 内部 Event/Timer fault 很有优势 | 需要独立硬件安全链、高压隔离或 MCU 掉电仍工作 |
| 高精度相位/时间 | 校准 propagation delay、hysteresis/filter 后再决定 | 需要更低延迟、更低 dispersion 或专用差分输入 |
| 多路窗口比较 | 三个内部 COMP/双 COMP 窗口可先评估 | 通道数不够或阈值精度/范围不够 |

不要把 LM339 当成默认升级。它的具体型号可能比内部高速 COMP 更慢，而且常是开漏输出；必须逐型号比较。

## 7. 五个真实性能计算案例

### Case 1：100 kHz，Gain = 2 的 OPA

**已知**：正弦最高 100 kHz，闭环增益 2；先假设输出峰值 1 V。

1. GBW 初筛：`100 kHz × 2 × 10 = 2 MHz`。
2. 内部 OPA 1.5 MHz 模式小于 2 MHz，不选低速模式。
3. 内部 OPA 6 MHz 模式大于 2 MHz，有约 3 倍于本经验门槛的数值余量。
4. SR：`2π × 100 kHz × 1 V ≈ 0.63 V/µs`，低于高 GBW 模式 4.9 V/µs typ。
5. 再核对输入共模、输出摆幅、PGA 是否有 Gain=2、允许幅相误差。

**结论**：`INTERNAL_PREFERRED` 或至少 `INTERNAL_NEEDS_TEST`。先用内部 OPA 高 GBW 模式，不必立即焊外置 OPA。

### Case 2：500 kHz，Gain = 10

**已知**：正弦最高 500 kHz，闭环增益 10，输出峰值仍假设 1 V。

1. GBW 初筛：`500 kHz × 10 × 10 = 50 MHz`。
2. 内部 OPA 6 MHz 远低于 50 MHz，明确淘汰。
3. SR：`2π × 500 kHz × 1 V ≈ 3.14 V/µs`；内部高 GBW OPA 的 4.9 V/µs typ 看似够，但不能抵消 GBW 失败。
4. 板载 OPA2365 是 50 MHz GBW、25 V/µs typ，达到经验初筛线，明显比内部 OPA 合适。
5. 50 MHz 等于而不是远大于经验所需值；若幅相误差严格，应选更高 GBW 外置 OPA 或做仿真/实测。

**结论**：内部 OPA `EXTERNAL_REQUIRED`；板载 OPA2365 为第一外置候选，但不是无条件保证。

### Case 3：500 kHz ADC sampling

先澄清英语/题面：

- 如果是 `sample rate = 500 kSPS`，内部 ADC 4 MSPS 上限有 8 倍吞吐余量，优先内部。还要检查前端源阻、采样时间和 DMA。
- 如果是 `signal frequency = 500 kHz`，500 kSPS 采样违反普通 Nyquist 条件；至少要 >1 MSPS。做波形/FFT/幅相时，2～4 MSPS 只有 4～8 点/周期，能否满足精度需实测和算法预算。

**结论**：500 kSPS 采样率是 `INTERNAL_PREFERRED`；500 kHz 输入信号则是 `INTERNAL_NEEDS_TEST`，不能混为一件事。

### Case 4：100 kHz DAC sine

假设内部 DAC 用最大更新率 1 MSPS、输出峰值 1 V：

1. 每周期点数：`1 MSPS / 100 kHz = 10`。
2. SR：`2π × 100 kHz × 1 V ≈ 0.63 V/µs`，低于 DAC 约 5.5 V/µs typ，SR 不是首要瓶颈。
3. 1 MSPS 的点间隔是 1 µs，而 DAC 满量程建立时间也约 0.8～1 µs；大幅相邻跳变会吃掉大部分建立时间。
4. 10 点/周期会有明显采样镜像，输出端需要合适重建低通。
5. 如果题目只要“可见的简单正弦”，可以先实测；如果要低 THD、高幅度准确度或高谱纯度，外置 DDS/DAC 更稳妥。

**结论**：`INTERNAL_NEEDS_TEST`，不是绝对可用，也不是看到 100 kHz 就绝对外置。

### Case 5：1.65 V VGA control voltage

假设 VDDA/参考为实测 3.300 V：

1. 理想 LSB：`3.3 V / 4096 ≈ 0.806 mV`。
2. 理想 code：`1.65 / 3.3 × 4095 ≈ 2047.5`，先写 2048。
3. 这是静态高阻控制，1 MSPS 和波形 THD 不重要。
4. 检查 VGA 控制输入范围、输入电流、控制曲线和允许误差。
5. 实测输出；若需要消除 DAC offset/gain/VREF 误差，用两点或多点校准，不把理想 2048 当最终真值。

**结论**：`INTERNAL_PREFERRED`。只有 VGA 需要双极性、更高电压、更高绝对精度或 DAC 驱动不了时才换外置。

## 8. 从结论到仓库模块

| 资源结论 | 下一步入口 |
|---|---|
| Internal ADC 足够 | `02_acquisition/adc_dma/`；连续采样再选 ping-pong/ring buffer |
| External ADC 必须 | `12_external_devices/adc/<exact device>/`；先看 exact README 和 Datasheet |
| Internal DAC 足够，固定电压 | SysConfig + DAC DriverLib；复杂模块不必选 |
| Internal DAC 足够，连续波形 | `06_generator/dds/` → `06_generator/dac_dma/` |
| External DDS/DAC 必须 | `12_external_devices/dds/` 或 `12_external_devices/dac/` 的具体型号 |
| Internal OPA 足够 | SysConfig/官方 OPA example；当前 BSP wrapper 的硬件边界以其 README 为准 |
| Board OPA2365 | 按 LP User Guide 原理图确认 PB25-/R72/R74/J13；它不是 SysConfig module |
| Internal COMP 足够 | SysConfig + COMP DriverLib；测频接 Timer Capture |
| External COMP 必须 | 先确定 exact part，再做输入保护、阈值、输出接口和延迟验证 |

模块选择文档只解决软件入口；硬件性能判定必须先完成本页资源检查。

## 9. 最终停手条件

满足下面任一条件就不要继续“优化内部方案”，直接切外置候选：

- ADC：输入范围、分辨率/ENOB、采样率或同步通道数明确失败。
- DAC：每周期点数太少、建立/SR/THD、输出精度或负载明确失败。
- OPA：GBW、SR、共模、摆幅、噪声、失调或驱动明确失败。
- COMP：输入范围、offset、延迟/dispersion 或输出接口明确失败。
- 系统：内部 Timer/DMA/Event/PinMux 资源冲突无法合理重分配。

反过来，如果内部有充足余量，也不要因为“外置器件指标更漂亮”增加接线、驱动、供电、时钟、PCB 和调试风险。
