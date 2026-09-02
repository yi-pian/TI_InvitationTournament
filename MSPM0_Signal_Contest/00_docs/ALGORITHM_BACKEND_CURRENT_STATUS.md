# 算法 Backend 真实现状审计

> **SUPERSEDED 2026-08-13：** 本文只保留旧 Backend 历史；下文逐模块“默认 Reference”等字段全部失效。当前比赛默认基础是 SDK CMSIS-DSP 1.16.2，Q15 是未完成板上周期比较前的默认候选，Reference C 只用于 PC truth。当前结论见 `CMSIS_DSP_STANDARDIZATION_REPORT.md`。

审计日期：2026-08-08。目标：MSPM0G3507，Cortex-M0+，80 MHz，32 KB SRAM，无 FPU，带 MATHACL。

## 1 先说结论

PRE-BACKEND 源码中没有 `arm_math.h`、`arm_const_structs.h`、`arm_cfft_*`、IQMath 或 MATHACL 调用。44 个原模块全部以自写 Reference C 为主；需要开方、三角、对数等操作时调用标准 `math.h`。算法库也没有 CCS `.projectspec`，只有 PC Makefile，所以不能因为 SDK 安装了 CMSIS/IQMath 就说算法库已经用了它们。

本轮在不改变现有公开函数签名的前提下增加了：

- FFT：Reference C、CMSIS Q15、CMSIS Q31、CMSIS F32 编译期后端；
- RMS、AC RMS、Statistics、FFT-bin Phase：Reference float、IQMath RTS、IQMath MATHACL 标量数学后端；
- RAW/float/Q15/Q30 轻量 Adapter；
- PC 数值 benchmark 和 TI Arm Clang 离线目标 Build/Link smoke。

“默认仍是 Reference”是 2026-08-08 的历史结论，已经失效。

## 2 审计依据

- 实际扫描 `03_measurement/**/*.c,h`、`04_dsp/**/*.c,h`、`05_precision/**/*.c,h` 和测试 Makefile；
- 本机 SDK：`C:\ti\mspm0_sdk_2_11_00_07`；
- TI Arm Clang：`C:\ti\ti_cgt_arm_llvm_4.0.2.LTS`；
- CMSIS-DSP V1.10.0 头文件和 `arm_cortexM0l_math.a`；
- IQMath RTS/MATHACL 两个 G1x0x/G3x0x `iqmath.a`；
- SDK 的 LP-MSPM0G3507 CMSIS FFT、IQMath RTS、IQMath MATHACL 官方示例；
- `source/ti/driverlib/dl_mathacl.h` 中的真实枚举和函数。

## 3 逐模块表

“原始实现”指 PRE-BACKEND 源码；“当前可选后端”指本轮完成后的状态。

| 模块 | 原始实现 / 数据类型 | CMSIS | IQMath | MATHACL | Reference | 值得优化 | 当前建议 Backend |
|---|---|---:|---:|---:|---:|---|---|
| ADC_ToVoltage | 循环换算，`uint16_t→float V` | 否 | 否 | 否 | 是 | 低 | Reference；定点链用 Adapter 绕过整块 float |
| Mean | float 补偿求和 | 否 | 否 | 否 | 是 | 低 | Reference |
| MinMax | float 扫描 | 否 | 否 | 否 | 是 | 低 | Reference |
| Vpp | MinMax 后相减 | 否 | 否 | 否 | 是 | 低 | Reference |
| RMS | float 补偿平方和 + `sqrtf` | 否 | 可选 IQ24 sqrt | 可由 IQMath 库调用 | 是 | 中 | 默认 Reference；MATHACL 仅显式工程档 |
| AC RMS | 去均值后平方和 + `sqrtf` | 否 | 可选 IQ24 sqrt | 可由 IQMath 库调用 | 是 | 中 | 同 RMS |
| Statistics | Welford float + `sqrtf` | 否 | 可选 IQ24 sqrt | 可由 IQMath 库调用 | 是 | 低 | 默认 Reference，保留数值稳定性 |
| ZeroCross | float 阈值扫描 | 否 | 否 | 否 | 是 | 低 | Reference |
| Phase | float/fmod/atan2 | 否 | 可选 IQ24 atan2 | 可由 IQMath 库调用 | 是 | 高 | 默认 Reference；MATHACL 为可选目标档 |
| RemoveDC | float 补偿均值/减法 | 否 | 否 | 否 | 是 | 低 | Reference |
| ClippingDetect | float 比较/计数 | 否 | 否 | 否 | 是 | 低 | Reference |
| MovingAverage | float 滑动和 | 否 | 否 | 否 | 是 | 中 | 当前 Reference；定点全链后再评估 Q15 |
| Median | float 排序窗口 | 否 | 否 | 否 | 是 | 低 | Reference |
| MAD | float Median + 绝对偏差 | 否 | 否 | 否 | 是 | 低 | Reference |
| Hampel | float Median/MAD | 否 | 否 | 否 | 是 | 低 | Reference |
| FIR | 外部 float 系数、环形状态 | 否 | 否 | 否 | 是 | 高 | 当前 Reference；CMSIS Q15 需新定点实例，不替换旧结构体 |
| IIR Biquad | float DF2T/SOS | 否 | 否 | 否 | 是 | 高 | 当前 Reference；CMSIS Q15 系数/状态约定不同，暂不强接 |
| Rectangular | wrapper | 否 | 否 | 否 | 是 | 低 | Reference |
| Hann | wrapper，内部 `cosf` | 否 | 否 | 否 | 是 | 中 | Reference；可预生成 Q15 窗系数省运行三角函数 |
| Hamming | wrapper，内部 `cosf` | 否 | 否 | 否 | 是 | 中 | 同 Hann |
| Blackman | wrapper，内部 `cosf` | 否 | 否 | 否 | 是 | 中 | 同 Hann |
| Window core | float 生成/原地乘法 | 否 | 否 | 否 | 是 | 中 | 当前 Reference；比赛优先离线系数表 |
| FFT | 自写 float radix-2 | 当前可选 Q15/Q31/F32 | 否 | 否 | 保留 | 高 | 稳定 Competition：CMSIS Q31；Q15 为显式性能档 |
| FFT Magnitude | float `hypotf` | 原始否 | 否 | 否 | 是 | 中 | 当前 Reference；只有频率时可比较模平方避免 sqrt |
| PeakDetect | float 扫描 | 否 | 否 | 否 | 是 | 低 | Reference |
| Harmonic | float 频谱索引/多 bin | 否 | 否 | 否 | 是 | 低 | Reference；不能让 backend 泄漏到谐波 API |
| THD | float 能量 + `sqrtf` | 否 | 未接入 | 未接入 | 是 | 中 | Reference；输入频谱可来自 CMSIS Q31 |
| SNR | float 能量 + log10 路径 | 否 | 否 | 否 | 是 | 中 | Reference，先保证 dB 语义 |
| SFDR | float 峰值/dB | 否 | 否 | 否 | 是 | 中 | Reference |
| Correlation | 自写 float O(N·lag) + `sqrtf` | 否 | 否 | 否 | 是 | 高 | 先 Reference；后续可用 CMSIS dot product，但需目标周期证据 |
| Autocorrelation | 自写 float O(N·lag) + `sqrtf` | 否 | 否 | 否 | 是 | 高 | 同 Correlation |
| ZeroCrossInterpolation | float 线性插值 | 否 | 除法可评估 | DIV 可用 | 是 | 低 | Reference，单次除法不值得来回转 IQ24 |
| MultiCycleAverage | float 周期差/除法 | 否 | 除法可评估 | DIV 可用 | 是 | 低 | Reference |
| FFT Parabolic Interpolation | float 三点公式 | 否 | 除法可评估 | DIV 可用 | 是 | 低 | Reference |
| Log Parabolic | `logf/expf` | 否 | IQMath 有 log/exp | MATHACL 库支持范围须实测 | 是 | 中 | 暂保留 Reference |
| WindowGainCorrection | float 缩放 | 否 | 否 | 否 | 是 | 低 | Reference |
| MultiBinEnergy | float 能量 + `sqrtf` | 否 | 未接入 | 未接入 | 是 | 中 | Reference；Q15 链可用 int64 累加 |
| ADC Gain/Offset Calibration | float 线性标定 | 否 | DIV 可评估 | DIV 可用 | 是 | 低 | Reference，标定不是高频内环 |
| ChannelDelayCalibration | float/fmod | 否 | 否 | 否 | 是 | 低 | Reference |
| RobustPeakToPeak | float 排序/`floorf/ceilf` | 否 | 否 | 否 | 是 | 低 | Reference |
| RobustRMS | float 截尾 + `sqrtf` | 否 | 未接入 | 未接入 | 是 | 低 | Reference |
| SineFit3 | float sin/cos/hypot/atan2/sqrt | 否 | 候选 | 候选 | 是 | 高 | 暂保留 Reference；精度优先，需独立一致性测试 |
| SineFit4 | 调用 3 参数并搜频率 | 否 | 候选 | 候选 | 是 | 高 | 暂保留 Reference |
| LockIn | float NCO + hypot/atan2 | 否 | 候选 | 候选 | 是 | 高 | 暂保留 Reference；适合以后整条 IQ24 链 |
| BackendAdapter（新增） | `uint16_t/float/int16_t/uint64_t` | 无依赖 | 无依赖 | 无依赖 | 是 | 必需 | PC_VERIFIED |

## 4 MATHACL 真实能力

本机 `dl_mathacl.h` 和官方示例确认存在这些硬件操作：SINCOS、ARCTAN2、ARCTAN、DIV、SQRT、MPY_32、SQUARE_32、MPY_64、SQUARE_64、MAC、SAC。DriverLib 真实启动函数包括：

- `DL_MathACL_startSinCosOperation`
- `DL_MathACL_startArcTan2Operation`
- `DL_MathACL_startDivOperation`
- `DL_MathACL_startSqrtOperation`
- `DL_MathACL_startMpyOperation`

算法层没有创造新的寄存器 API。比赛代码优先通过 IQMath 的统一函数名链接 MATHACL 版本 `iqmath.a`，而不是每个算法直接抢占 MATHACL 外设。MATHACL 有初始化、忙状态和共享所有权问题，尚未上板测周期。

## 5 验证状态

- Reference：`REFERENCE_VERIFIED`，完整 `234 PASS / 0 FAIL`；
- CMSIS Q15/Q31/F32 benchmark：各 `26 PASS / 0 FAIL`；
- Q31/F32 既有 FFT+THD+Phase 三批：`80 PASS / 0 FAIL`；
- Q15 既有三批：基础 FFT `37/0`，THD `22/1`，Phase `19/1`，所以不标为完整回归无差异；
- TI Arm Clang 离线目标链接：512/1024/2048 成功，4096 因真实 SRAM 超限失败；
- IQMath RTS/MATHACL：算法 RMS+Phase 目标编译链接成功；
- `BOARD_RUNTIME_VERIFIED`：没有，严禁伪造。
