# Op-Amp Test Pipeline Guide

这份指南区分“MSPM0 能从采样计算什么”和“必须有真实供电、负载、切换/仪器才能测什么”。算法结果不能代替硬件测试条件。

## 1. 总架构

```text
[MODULE] DDS/WaveTable → [MODULE] DAC DMA → reconstruction/buffer
                                   ↓ stimulus
                                 DUT op-amp
                                   ↓
[MODULE] Dual ADC Sync（Vin/Vout）
→ 两路 [RECIPE] ADC To Voltage
→ gain / phase / frequency response / slew recipes
→ [MODULE] TFT numeric/Bode/waveform
```

绝对 gain/phase 推荐同时采 Vin 与 Vout；把 DDS 设定幅度当 Vin 会遗漏 DAC、buffer、负载和连线误差。

## 2. 闭环增益

```text
稳态去除 settling
→ Vin amplitude（sine fit / RMS / coherent FFT）
→ Vout amplitude（同一种方法）
→ gain = Aout/Ain
→ gain_db = 20 log10(gain)
```

复用 `[RECIPE] measurement_gain`。失效：任一路 clipping、幅值接近噪声、两路不同步、输入/输出方法不一致。RAM 主要是双路 `4N` raw + 可选 `8N` float。

## 3. -3 dB 带宽与频响

```text
[MODULE] Frequency Sweep
→ 每点 DDS/DAC settle
→ Dual ADC acquire
→ [MODULE] Lock-In 或 Sine Fit/FFT
→ [RECIPE] Frequency Response
→ crossing interpolation at reference_gain_db - 3 dB
```

复用 `sweep_analyzer` 参考 Application，但其 reference amplitude 当前来自 DDS 配置；严谨测运放应改成参考/响应双路并做直通校准。`bandwidth`/cutoff 见算法库 Recipe，屏幕 Bode 见 `XY_BODE_DISPLAY_RECIPE.md`。

## 4. 单位增益带宽 GBW

闭环非反相/反相配置下，先测低频 closed-loop noise gain，再测其 -3 dB bandwidth，近似 `GBW≈noise_gain×bandwidth` 只在单主极点、未受 slew/输出负载/仪器带宽限制时成立。复用 `[RECIPE] measurement_bandwidth`，但 GBW 的模型判定仍属于 Op-Amp 测试 Recipe，不是一个现成 Primitive。

## 5. Slew Rate

```text
高速 ADC frame
→ 稳健上下电平
→ 20/80（或题目指定 10/90）阈值
→ crossing 线性插值
→ 多边沿 median/mean
→ rise/fall time
→ ΔV/Δt 或局部稳健斜率
```

复用算法库 `[RECIPE] measurement_slew_rate`。采样率必须让阈值区间内有足够点；ADC、前端、发生器和探头带宽/自身 slew 都要明显快于 DUT。`ADC FIFO DMA` 可用于高吞吐单帧，但其实际采样模式与题目 Fs 要从 Profile 确认。

## 6. 不能只靠现有软件闭合的项目

| 项目 | 还缺什么 | 当前状态 |
|---|---|---|
| Input offset | 精密短接/源、低漂移放大/量程、极性和温漂控制 | `MISSING HARDWARE TEST FIXTURE` |
| Input bias current | 已知高阻网络、漏电/PCB 清洁、精密电压测量 | `MISSING HARDWARE TEST FIXTURE` |
| Static power | 电源电流测量路径/分流/仪表，不能由输出波形推算 | `MISSING CURRENT MEASUREMENT` |
| Output swing | 可控负载、供电轨测量、失真/clipping 判据 | `MISSING LOAD/SUPPLY FIXTURE` |
| CMRR/PSRR | 可控 common-mode/电源扰动、同步参考、精密幅值测量 | `MISSING STIMULUS FIXTURE` |

这些缺口可以写 Application glue 和测试流程，但在硬件未建立前不能标 BUILD/BOARD verified 的“完整运放测试仪”。

## 7. SysConfig / 资源与风险

- 资源：DAC12 + DAC Timer/Event/DMA，ADC0/ADC1 + common trigger + two DMA，TFT SPI/GPIO（如显示）。
- 必须确认 DAC 与 ADC Timer/Event/DMA 不冲突；参考 `PROFILE_04_ADC_DAC`/`PROFILE_06_FULL_SIGNAL`，但最终以目标 `.syscfg` 为准。
- 模拟风险：输入/输出共模和摆幅、负载稳定性、anti-alias、DAC reconstruction、直通校准、ground/50 Ω 终端。
- 每个 sweep point 保存 `frequency/gain/phase/valid`，约 13～16 B/point；FFT/Sine Fit workspace 另算。

## 8. 验收

先做直通（无 DUT）测系统 gain/phase/带宽基线，再接已知低速运放，然后覆盖幅值、频率、负载和供电边界。Build 只能证明软件可链接；GBW、slew、swing 等必须用台式仪器/已知标准件对照后才能写 BOARD_VERIFIED。

