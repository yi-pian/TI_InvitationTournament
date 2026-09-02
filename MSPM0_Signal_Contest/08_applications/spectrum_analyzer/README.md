# Spectrum Analyzer

> **REFERENCE ASSEMBLY EXAMPLE**：演示完整频谱模块连接，不是赛题解决方案。

## 本 Application 的实现层级

| 功能 | 类型 | 当前实现 |
|---|---|---|
| 定时 N 点采样 | B Complex Hardware Module | ADC DMA |
| 频谱处理 | C Algorithm Module | ADC To Voltage、Remove DC、Hann、FFT、Magnitude |
| 幅值/峰值精修 | C Algorithm Module | Window Gain Correction、Peak、Parabolic |
| 完整组合 | E Application Reference | 参考主峰与多个峰结果组织 |

```text
ADC DMA → RawToVoltage → RemoveDC → Hann → FFT
        → Magnitude → WindowGainCorrection → Peak/Parabolic → top peaks
```

- Status：`BUILD_VERIFIED`；Q31 PC truth PASS；Board=`PENDING_BOARD`。
- Config：`signal_config.h`。
- Hardware profile：P01。
- Projectspec：`ticlang/spectrum_analyzer_q31_LP_MSPM0G3507_nortos_ticlang.projectspec`。
- 重点学习：complex workspace、N/2+1 magnitude、window gain 与 peak 输出。

N=1024 Q31 实测链接 SRAM 17,045 B。改 N/Backend 后重新 full link 并看 `.map`。
