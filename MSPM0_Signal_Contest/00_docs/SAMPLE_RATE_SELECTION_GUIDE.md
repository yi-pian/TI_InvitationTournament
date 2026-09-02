# Sample Rate Selection Guide：拿到题目怎样选择 Fs、N 与 Fupdate

这份文档只解决两个问题：ADC 应该选多少 `Fs`，DAC 应该选多少 `Fupdate`。它不是固定答案表；先从指标倒推，再用当前 MSPM0G3507、Profile、RAM 和模拟前端验证。

完整 SysConfig 点选教程见 [MSPM0_SYSCONFIG_CLOCK_TIMER_ADC_DAC_BEGINNER_GUIDE.md](MSPM0_SYSCONFIG_CLOCK_TIMER_ADC_DAC_BEGINNER_GUIDE.md)，现场公式见 [CLOCK_TIMER_ADC_DAC_QUICK_REFERENCE.md](CLOCK_TIMER_ADC_DAC_QUICK_REFERENCE.md)。

## 1. 先写出这七项，不要先写 Fs

1. 信号最高基波频率 `f0_max`。
2. 最高感兴趣频率 `f_highest_interest`：包括谐波、边带和想保留的瞬态带宽。
3. 任务：DC、幅值、波形显示、FFT、THD、相位、边沿还是控制输出。
4. 需要的 FFT 分辨率或观察时间。
5. 最短 rise/fall time 或 pulse width。
6. 响应时间：多久必须给出一次结果。
7. 通道数、同步要求、RAM、CPU 和模拟前端带宽。

缺少的指标写 `UNKNOWN`，不能用 100 kSPS 或 4 MSPS 随手填空。

## 2. 三个永远先算的式子

```text
samples_per_cycle = Fs / fsignal
FFT bin spacing Δf = Fs / N
frame/observation time Tobs = N / Fs
```

它们代表三个不同目标：

- 点/周期：波形形状和每周期时间网格有多细；
- `Δf`：固定 N 的 FFT 频率格子有多粗；
- `Tobs`：一帧覆盖多少时间、多少个低频周期。

提高 Fs 会增加点/秒，但固定 N 时会缩短 Tobs、增大 Δf。不存在“全部选最大 Fs 最准”。

## 3. ADC Fs 决策表

| 任务 | 首要约束 | 初始候选怎样算 | 还必须检查 |
|---|---|---|---|
| DC/慢传感器 | 噪声、平均时间、响应时间 | 先按所需更新率的若干倍采样 | 工频干扰、settling、平均窗口 |
| 正弦 Vpp/RMS | 最高频率、点/周期 | `Fs≈fmax×期望点数` | 抗混叠、前端带宽、异常点 |
| 波形显示 | 看起来平滑 | 常从 20～100 点/周期试 | TFT 像素、降采样、RAM |
| 基础频率 | 周期覆盖和时间精度 | 保证多周期与足够点/周期 | Timer Capture 是否更简单 |
| FFT/频谱 | Nyquist + Δf + Tobs | 联立 `Fs>2f_high`、`N≥Fs/Δf_req` | Window、coherent gain、RAM |
| THD | 最高谐波 | `f_high=f0max×Hmax` 后选 Fs | 抗混叠、前端自身 THD |
| 相位/时延 | 时间网格与同步 | 看 `360°×f/Fs` 或 `Ts` | 双 ADC 同步、通道延时校准 |
| Rise/Fall/Slew | 最短边沿 | `Fs≈目标边沿点数/trise` | 模拟带宽、trigger、插值 |
| 脉宽/占空比 | 最短 pulse/edge | 给最短脉宽足够点，或直接 Capture | 阈值、迟滞、边沿噪声 |

表中的点数都是候选起点，不是硬指标。先用最小能满足指标的 Fs，再根据板测误差调整。

## 4. Nyquist 只是下限

理论上带限信号要满足 `Fs > 2×f_highest_interest`，但比赛系统还需要：

- 给模拟抗混叠滤波器留过渡带；
- 为幅值、相位、边沿和显示保留足够采样点；
- 避开输入频率恰好接近 Nyquist 时的极端相位敏感性；
- 让 ADC sample/conversion sequence 真正跟得上。

因此“2.01 点/周期”不能当正常波形测量方案。

## 5. 每周期点数怎样理解

以下标成 **HEURISTIC**：

| 点/周期 | 适合做什么 | 不适合什么 |
|---:|---|---|
| 2～4 | 只用于极限可检测性讨论 | 波形显示、精确 Vpp、THD |
| 8～10 | 基础频率/FFT 候选 | 高质量波形、边沿细节 |
| 20 | 常见基础幅值/相位起点 | 不能替代前端校准和插值 |
| 50～100 | 更平滑显示和更细时间网格 | 会增加吞吐；不是所有题都需要 |

算法会改变需求。正弦拟合、FFT Peak Interpolation、过零线性插值能获得亚采样点参数估计；任意波形 Vpp 和边沿形状不能只靠“亚点插值”凭空恢复丢失带宽。

## 6. FFT：从分辨率反推 N

```text
N ≥ Fs / Δf_required
```

例：覆盖到 100 kHz，希望 FFT bin 不粗于 100 Hz。若选 Fs=250 kSPS：

```text
N ≥ 250000/100 = 2500
```

若正式 FFT 只支持 2 的幂，候选 N=4096。此时仅 float raw-to-voltage、complex FFT 和 magnitude 就可能超过 MSPM0G3507 32 KB SRAM，所以不能只算频谱公式；应考虑：

- 降低不必要的 Fs；
- 复用 buffer、使用 Q15/Q31 Backend；
- Peak Interpolation/Zoom FFT/CZT；
- 分块或将部分处理移到 PC。

### N=1024 常用对照

| Fs | Δf | Tobs | 说明 |
|---:|---:|---:|---|
| 100 kSPS | 97.65625 Hz | 10.24 ms | 细一些，但只覆盖到 50 kHz |
| 500 kSPS | 488.28125 Hz | 2.048 ms | 覆盖到 250 kHz |
| 1 MSPS | 976.5625 Hz | 1.024 ms | 覆盖到 500 kHz |
| 4 MSPS | 3906.25 Hz | 0.256 ms | 覆盖宽，但 bin 很粗 |

## 7. THD：先算最高谐波

```text
f_highest_interest = f0_max × highest_harmonic_order
```

例如 100 kHz 基波测 H5，最高兴趣频率是 500 kHz。Fs 还要给抗混叠滤波器过渡带留余量。选择后检查：

- N 是否覆盖足够周期；
- 基波/谐波是否相干采样；
- 非相干时用 Hann 等窗口并做 coherent gain 修正；
- Harmonic 使用单 bin 还是 multi-bin energy；
- ADC、OPA/VGA 和外部前端自身 THD 是否比目标更低。

## 8. 边沿：从 rise time 反推

```text
Fs ≈ desired_samples_on_edge / rise_time
```

若希望 10 us 边沿上有 20 个采样间隔：Fs≈2 MSPS。然后用：

```text
稳健低/高电平
→ 20/80% 或 10/90% 阈值
→ crossing 线性插值
→ 多边沿平均
→ rise/fall time
→ slew rate
```

如果模拟前端带宽不够，数字 Fs 再高也不会恢复真实边沿。

## 9. 相位和时延：Fs 是粗网格，不是最终精度

```text
raw_phase_step_deg = 360° × fsignal/Fs
raw_delay_step = 1/Fs
```

选择顺序：

1. 优先同步双 ADC，避免软件轮询造成通道时差。
2. 让每周期至少有合理点数；20 点/周期可作为常见起点。
3. 采用 FFT Phase、Cross Correlation 或 crossing interpolation 获取亚采样点估计。
4. 用同相信号校准两通道固定 delay/phase，特别是不同前端滤波路径。
5. 低 SNR 时增加周期数、相关长度或平均，不要只升 Fs。

## 10. 低频信号不要被高 Fs 反噬

例：最低 10 Hz，而 Fs=100 kSPS、N=1024：

```text
Tobs=10.24 ms
10 Hz 一个周期=100 ms
```

一帧连一个周期都装不下，ZeroCross/FFT 频率都不稳。可选：

- 降 Fs；
- 增 N（先看 RAM）；
- 用 Timer Capture/长时间计数；
- decimation 后再做低频算法。

频率上限与最低频率窗口要同时检查。

## 11. 多通道怎样算

一次 trigger 顺序采 `M` 个通道时：

```text
per_channel_Fs = trigger_rate
total_conversion_rate = trigger_rate × M
```

例：100 k triggers/s、4 通道 → 每通道 100 kSPS，总计 400 k conversions/s。还要把每个 Memory 的 sample+conversion 时间相加，确保序列在下一 trigger 前完成。

双 ADC 同步各采一个通道则是两套 ADC 同时工作，资源和同步方式不同，按 P02 与实际 ADC instance 检查。

## 12. MSPM0G3507 内部 ADC 的边界

当前官方资料给出的关键能力是：两个可同时工作的 12-bit ADC；每个 ADC 在 12/10-bit 下最高 4 MSPS；ADC clock 允许范围 4～48 MHz。使用这些数字时仍须满足当前分辨率、sample time、clock source、sequence 和输入驱动条件。

在 SysConfig ADC 页检查：

- `Calculated Sample Clock Frequency`
- `Actual Sample Time 0`
- 各 Memory 的 `ADC Conversion Period`
- `Total Conversion Frequency`
- Problems 面板的时钟/吞吐警告

Timer 能产生 4 MHz event 只证明定时器做得到，不证明你的多通道 ADC、前端和 DMA 系统都能无误工作。

## 13. DAC Fupdate 怎样选

先决定希望每周期多少个更新点：

```text
Fupdate = Fout × points_per_cycle
```

然后检查内部 DAC 当前官方 1 MSPS output sampling 上限、settling、重构滤波和 DMA/Timer。

| Fout | 期望点/周期 | 所需 Fupdate | 内部 DAC 结论 |
|---:|---:|---:|---|
| 1 kHz | 100 | 100 kSPS | 温和起点 |
| 10 kHz | 100 | 1 MSPS | 能力边界，需验证 |
| 100 kHz | 20 | 2 MSPS | 超内部 1 MSPS；需降点数或外置 |
| 100 kHz | 10 | 1 MSPS | 可实验，波形质量有限 |
| 500 kHz | 2 | 1 MSPS | 不能视作高质量正弦 |

固定 DC 直接写一次 code。Software DDS 一般固定 Fupdate，再改 phase increment；当高频导致点数不足时才提高 Fupdate。

## 14. 从 Fs/Fupdate 回算 Timer

对当前 P01/P03 的 32 MHz Periodic Down Counting：

```text
T = 1/rate
ticks = round(32,000,000 × T)
Load = ticks - 1
actual_rate = 32,000,000/ticks
```

| Rate | Period | Ticks | Load |
|---:|---:|---:|---:|
| 1 kHz | 1 ms | 32000 | 31999 |
| 100 kHz | 10 us | 320 | 319 |
| 200 kHz | 5 us | 160 | 159 |
| 500 kHz | 2 us | 64 | 63 |
| 1 MHz | 1 us | 32 | 31 |
| 2 MHz | 0.5 us | 16 | 15 |
| 4 MHz | 0.25 us | 8 | 7 |

SysConfig 1.26.2 填 `Desired Timer Period`，再看 `Actual Timer Period`。正式 ADC/DAC DMA 模块运行时会按 rate 和 `timer_clock_hz` 设置 Load，因此必须区分 Profile 基线和运行时实际 rate。

## 15. 十个快速候选

| 题目 | 初始候选 | 关键理由/警告 |
|---|---|---|
| 1 kHz Vpp | Fs=100 kSPS, N=1024 | 100 点/周期，约 10.24 周期 |
| 100 kHz Vpp | Fs=2 MSPS, N=1024 | 20 点/周期；检查前端 |
| 500 kHz 频率 | Timer Capture；ADC 备选 4 MSPS | Capture 更合适；ADC 仅 8 点/周期 |
| 500 kHz FFT | Fs=4 MSPS, N=1024 | Δf=3.90625 kHz，分辨率较粗 |
| 100 kHz 基波到 H5 | Fs=4 MSPS | 最高兴趣频率 500 kHz；抗混叠/THD 校准 |
| 10 us rise time | Fs=2～4 MSPS | 20～40 点边沿候选 |
| 双通道 100 kHz 相位 | Fs=2 MSPS/通道 | 同步双 ADC + 插值/校准 |
| DAC 1 kHz sine | Fupdate=100 kSPS | 100 点/周期 |
| DAC 100 kHz sine | Fupdate=1 MSPS | 仅 10 点/周期，需验证/可能外置 |
| DAC 固定 1.65 V | 无 Fupdate | 写一次 code，不要 Timer/DMA |

这些数字是从明确假设得到的候选，不是所有题目的默认值。精度、SNR、响应时间或前端一变，就要重算。

## 16. 最终提交前检查

- [ ] 已写 `f_highest_interest`，THD 已计入最高谐波。
- [ ] 已检查 Nyquist，并给模拟抗混叠滤波留余量。
- [ ] 已检查点/周期或边沿点数。
- [ ] FFT 已同时算 `Δf` 和 `Tobs`。
- [ ] 最低频率在一帧内有足够周期。
- [ ] 多通道已区分 per-channel Fs 和 total conversions/s。
- [ ] ADC Clock、Sample Time、Conversion Period 能赶上 trigger。
- [ ] DAC 已检查 points/cycle、1 MSPS 上限、settling 和重构滤波。
- [ ] N/工作区/stack 没超过 SRAM，最终看 `.map`。
- [ ] CPU 能在 frame deadline 内处理完成。
- [ ] Timer Clock Source、Divider、Prescaler 与软件 `timer_clock_hz` 一致。
- [ ] 下游算法使用配置后的实际 Fs，而不是过期常量。
- [ ] 已用已知信号、示波器/频率计进行实板验证；Build 不冒充板测。

## 17. 官方事实入口

- [MSPM0G3507 Datasheet](https://www.ti.com/document-viewer/MSPM0G3507/datasheet)
- [MSPM0 G-Series Technical Reference Manual](https://www.ti.com/lit/ug/slau846d/slau846d.pdf)
- [MSPM0 SDK SysConfig Guide](https://software-dl.ti.com/msp430/esd/MSPM0-SDK/latest/docs/english/tools/sysconfig_guide/doc_guide/doc_guide-srcs/sysconfig_guide.html)

器件/SDK/前端改变后，重新核对上限和 GUI 字段；不要把本指南的候选值当成跨器件常量。
