# Dual Channel Phase Meter

> **REFERENCE ASSEMBLY EXAMPLE**：演示 DualADC 与两种 Phase 链，不是赛题解决方案。

## 本 Application 的实现层级

| 功能 | 类型 | 当前实现 |
|---|---|---|
| 双通道同步帧 | B Complex Hardware Module | Dual ADC Sync + Dual ADC Platform Adapter |
| 电压换算/去 DC/相位 | C Algorithm Module | ADC To Voltage、Remove DC、FFT Phase、Correlation Phase |
| 简单 DriverLib 动作 | A Direct DriverLib | 无独立功能；`SYSCFG_DL_init()` 只负责生成配置初始化 |
| 完整组合 | E Application Reference | 用于参考双路 buffer 和两种相位后端 |

```text
ADC0/DMA0 + ADC1/DMA1 → two raw buffers → two RawToVoltage → RemoveDC
                      ├→ Hann + FFT bins → FFT Phase
                      └→ Correlation lag → Correlation Phase
```

- Status：`BUILD_VERIFIED`；Q31 PC truth PASS；Board=`PENDING_BOARD`。
- Config：`signal_config.h`。
- Hardware profile：P02。
- Dual ADC Platform：[正式说明](../common/dual_adc_platform_adapter/README.md)，唯一源码 `../common/signal_dual_adc_platform.c/.h`。
- Projectspec：`ticlang/dual_channel_phase_meter_q31_LP_MSPM0G3507_nortos_ticlang.projectspec`。
- 重点学习：两块独立 raw buffer、相同 N/Fs/window、B−A 相位约定。

默认 N=512。N=1024 虽能链接但只余 3,040 B SRAM；DualADC skew/前端延迟仍待板测。

## 低频窗口与 CCS 图形配置

默认 `Fs=100 kHz、N=512` 的同步窗口只有 5.12 ms，不能测周期为 100 ms 的 10 Hz 相位。10 Hz 可从 `Fs=1 kHz、N=1024` 起步，得到 1.024 s 窗口；但 N=1024 的 RAM 裕量很小，必须查看最终 `.map`，必要时减少其他 buffer、改定点后端或分块处理。

双 ADC、公共 Timer、Event 和 DMA 全部在 CCS 中双击 `.syscfg` 后通过 SysConfig 图形界面配置。参考 P02 只用于对照资源；不要直接编辑 `.syscfg` 文本或生成的 `ti_msp_dl_config.*`。ADC Timer 的事件率应设为采样率 Fs（例如 1 kHz），不是输入频率 10 Hz；保存后核对 Calculated Clock、实际触发率和生成宏，再 Clean/Build。

若降低 Fs，必须同时确认最高输入频率满足 Nyquist、模拟抗混叠滤波合适、Correlation 最大 lag 足够，以及 FFT bin/已知频率配置同步更新。
