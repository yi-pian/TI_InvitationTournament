# Signal Analyzer

> **REFERENCE ASSEMBLY EXAMPLE**：展示编译期 Profile 如何裁剪模块，不负责题目分析或自动选择 Profile。

## 本 Application 的实现层级

| 功能 | 类型 | 当前实现 |
|---|---|---|
| 双通道同步采集 | B Complex Hardware Module | Dual ADC Sync + Platform Adapter |
| Basic/Frequency/Spectrum/THD/Phase | C Algorithm Module | 由编译期 Profile 选择，不强制全部运行 |
| 简单 DriverLib 动作 | A Direct DriverLib | 无独立功能；硬件初始化来自目标 `.syscfg` |
| 综合母版 | E Application Reference | 参考 Profile 裁剪、buffer 与结果组织 |

```text
P02 ADC A/B → RawToVoltage → selected compile-time Profile
├─ Basic measurements
├─ ZeroCross frequency
├─ Spectrum/SNR/SFDR
├─ Harmonic/THD
└─ Dual-channel phase
```

- Status：5/5 Profiles `BUILD_VERIFIED`；Board=`PENDING_BOARD`。
- Config：`signal_config.h`；feature/profile：`signal_features.h`。
- Hardware profile：P02。
- Projectspec：`ticlang/signal_analyzer_final_LP_MSPM0G3507_nortos_ticlang.projectspec`。
- 重点学习：只编译/执行所选分支，避免所有算法无条件同时运行。

具体拼装方法见 `00_docs/MODULE_ASSEMBLY_GUIDE.md`；不要把本工程当作自动赛题分析框架。
