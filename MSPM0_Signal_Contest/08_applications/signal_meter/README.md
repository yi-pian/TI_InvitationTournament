# Signal Meter

> **REFERENCE ASSEMBLY EXAMPLE**：只演示多个现有模块怎样连接，不是赛题解决方案。

## 本 Application 的实现层级

| 功能 | 类型 | 当前实现 |
|---|---|---|
| 固定 Fs/N 采样 | B Complex Hardware Module | ADC DMA（Timer + Event + ADC + DMA） |
| Raw 转电压与基础测量 | C Algorithm Module | ADC To Voltage、Mean、MinMax、VPP、RMS、AC RMS |
| 波形测频 | C Algorithm Module | Zero Cross + Interpolation |
| 完整组合 | E Application Reference | 最短单通道“采集 → 测量 → 结果”参考 |

```text
PA25 → ADC0 + TIMG0 + DMA0 → raw[N]
     → RawToVoltage → Mean/MinMax/VPP/RMS/ACRMS
     → ZeroCross/Interpolation frequency
```

- Status：`BUILD_VERIFIED`；Board=`PENDING_BOARD`。
- Config：`signal_config.h`。
- Hardware profile：P01。
- Projectspec：`ticlang/signal_meter_round1_LP_MSPM0G3507_nortos_ticlang.projectspec`。
- 重点学习：raw/N/Fs 契约、集中 measurement mask、一次采样复用多个测量。

参数去哪里改见 `00_docs/PARAMETER_MODIFY_GUIDE.md`；改变 ADC pin/Timer/DMA 时见 `SYSCONFIG_MODIFY_GUIDE.md`。Build 后必须检查 `.map`。
