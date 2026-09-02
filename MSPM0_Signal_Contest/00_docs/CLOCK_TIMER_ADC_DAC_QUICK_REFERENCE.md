# Clock / Timer / ADC / DAC 比赛现场速查

> **ADC Clock ≠ Fs；Timer Clock ≠ Event Rate；DAC Clock ≠ Fupdate。**

## 1. ADC：先选 Fs，再回算 Timer

```text
最高感兴趣频率（THD 要算到最高谐波）
→ 任务：幅值 / FFT / 相位 / 边沿
→ 选 Fs
→ Ts=1/Fs
→ Timer Desired Period=Ts
→ 检查 ADC Sample + Conversion + Sequence 能赶上
```

```text
points/cycle = Fs/fsignal
FFT Δf = Fs/N
frame/Tobs = N/Fs
edge points ≈ Fs×rise_time
```

## 2. DAC：先选点/周期

```text
points/cycle = Fupdate/Fout
Fupdate = Fout×points/cycle
Tupdate = 1/Fupdate
Timer Desired Period=Tupdate
```

- 固定 DAC DC → 不需要 Timer/DMA，写一次 code。
- DAC DMA → Timer/Event 决定 Fupdate。
- Software DDS → 通常固定 Fupdate，用 phase increment 改 Fout。
- 内部 DAC 当前官方上限 1 MSPS；100 kHz 在 1 MSPS 下只有 10 点/周期。

## 3. 当前 P01/P03 的 32 MHz Timer 回算

Periodic Down Counting：

```text
ticks = round(Ftimer×Tperiod)
Load = ticks-1
actual_rate = Ftimer/ticks
```

| Rate | Desired Period | Ticks | Load核对值 |
|---:|---:|---:|---:|
| 1 kHz | 1 ms | 32000 | 31999 |
| 100 kHz | 10 us | 320 | 319 |
| 200 kHz | 5 us | 160 | 159 |
| 500 kHz | 2 us | 64 | 63 |
| 1 MHz | 1 us | 32 | 31 |
| 2 MHz | 0.5 us | 16 | 15 |
| 4 MHz | 0.25 us | 8 | 7 |

SysConfig 1.26.2 真正填写：Timer → `Desired Timer Period`；核对 `Timer Clock Frequency`、`Actual Timer Period`。不要手改 `ti_msp_dl_config.c/.h`。

## 4. 一眼决策

| 任务 | 先看什么 |
|---|---|
| DC | 噪声、平均时间、响应时间 |
| Vpp/RMS | 最高频率 + 点/周期 |
| FFT | Nyquist + `Fs/N` + `N/Fs` + RAM |
| THD | `f0_max×最高谐波次数` |
| Rise/Fall | `Fs×rise_time` |
| Phase | 同步双 ADC + `1/Fs` + 插值/校准 |
| DAC 波形 | `Fout×points/cycle` |
| 固定 DAC 电压 | 不需要更新率 |

## 5. 改 rate 后别漏

- Timer clock source/divider/prescaler 与软件 `timer_clock_hz` 一致。
- ADC：sample time、conversion period、多通道总吞吐。
- DAC：DDS update rate、points/cycle、settling、滤波。
- FFT：同步实际 Fs、重算 Δf/Tobs。
- N/RAM/CPU deadline 重算。
- 正式模块运行时可能覆盖 Profile 的 10 us；读取配置后的实际 rate。
- Clean → Build → 仪器验证；Build 不是 Board Verified。

详细选择：[SAMPLE_RATE_SELECTION_GUIDE.md](SAMPLE_RATE_SELECTION_GUIDE.md)；完整 GUI 教程：[MSPM0_SYSCONFIG_CLOCK_TIMER_ADC_DAC_BEGINNER_GUIDE.md](MSPM0_SYSCONFIG_CLOCK_TIMER_ADC_DAC_BEGINNER_GUIDE.md)。
