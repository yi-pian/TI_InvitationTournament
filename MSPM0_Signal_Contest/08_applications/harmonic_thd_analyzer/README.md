# Harmonic / THD Analyzer

> **REFERENCE ASSEMBLY EXAMPLE**：演示 Harmonic/THD 拼装，不是赛题解决方案。

## 本 Application 的实现层级

| 功能 | 类型 | 当前实现 |
|---|---|---|
| 定时 N 点采样 | B Complex Hardware Module | ADC DMA |
| 电压与频谱链 | C Algorithm Module | ADC To Voltage、Remove DC、Hann、FFT、Magnitude |
| 谐波质量 | C Algorithm Module | Peak/Interpolation、Multi-bin Energy、Harmonic、THD |
| 完整组合 | E Application Reference | 参考 H1～H5 与 THD 结果组织 |

```text
ADC DMA → Voltage → RemoveDC → Hann → FFT → Magnitude
        → fundamental interpolation → H1..H5 MultiBinEnergy → THD
```

- Status：`BUILD_VERIFIED`；Q31 PC truth PASS；Board=`PENDING_BOARD`。
- Config：`signal_config.h`。
- Hardware profile：P01。
- Projectspec：`ticlang/harmonic_thd_analyzer_q31_LP_MSPM0G3507_nortos_ticlang.projectspec`。
- 重点学习：峰值频率、谐波 bin radius、Harmonic result 直接连接 THD。

改 Fs/frequency range 前检查 H5 是否仍低于 Nyquist。
