# DSP / Math backend audit

审计日期：2026-08-07。目标：MSPM0G3507，Cortex-M0+，80 MHz，32 KB SRAM，
无硬件 FPU，带 MATHACL。SDK：MSPM0 SDK 2.11.00.07。编译器：
TI Arm Clang 5.1.1.LTS。

## 审计结论

1. 当前正式算法在本次审计前没有使用 CMSIS-DSP、IQMath 或 MathACL，均为自写 C。
2. 已建立薄后端层，所有 `arm_*` 和 `_IQ*` 依赖被隔离；上层公开 API 和 recipe 未改。
3. CMSIS-DSP Q15 被选为比赛 FFT 默认后端；Reference C 必须保留。
4. IQMath 只定位为标量定点数学。RTS 和 MathACL 两种目标库均已真实链接，板上周期
   和数值结果仍是 `PENDING_BOARD`。
5. IIR/Biquad 在当前正式算法库中不存在。本次只给出 CMSIS 选型结论，没有生成模块。

这里的“默认后端”是后续系统组装选择，不是把现有 float complex API 静默重解释为
Q15。本次没有修改 `04_dsp/fft` 的公开契约，也没有引入隐藏的大缓冲。

## 官方和本机证据

本机 SDK 已确认以下实体，不依赖网络猜测：

| 项目 | 本机实际路径/证据 |
|---|---|
| CMSIS Core | `C:\TI\mspm0_sdk_2_11_00_07\source\third_party\CMSIS\Core` |
| CMSIS DSP headers/sources | `...\source\third_party\CMSIS\DSP\Include`、`...\DSP\Source` |
| TI Clang M0+ CMSIS archive | `...\DSP\lib\ticlang\m0p\arm_cortexM0l_math.a` |
| LP-G3507 Q15 FFT example | `...\examples\nortos\LP_MSPM0G3507\cmsis_dsp\cmsis_dsp_fft_q15` |
| IQMath header | `...\source\ti\iqmath\include\IQmathLib.h` |
| IQMath RTS archive | `...\iqmath\lib\ticlang\m0p\rts\mspm0g1x0x_g3x0x\iqmath.a` |
| IQMath MathACL archive | `...\iqmath\lib\ticlang\m0p\mathacl\mspm0g1x0x_g3x0x\iqmath.a` |
| RTS official example | `...\examples\nortos\LP_MSPM0G3507\iqmath\iqmath_rts_ops_test` |
| MathACL official example | `...\examples\nortos\LP_MSPM0G3507\iqmath\iqmath_mathacl_ops_test` |
| DriverLib MATHACL definition | `...\source\ti\driverlib\dl_mathacl.h` |

TI 的 MSPM0G3507 数据手册明确该器件为 80 MHz/32 KB SRAM，并列出 MATHACL；
技术参考手册给出 MATHACL 细节：

- [MSPM0G350x datasheet](https://www.ti.com/lit/ds/symlink/mspm0g3507.pdf)
- [MSPM0 G-Series 80-MHz TRM](https://www.ti.com/lit/pdf/SLAU846)
- [SDK 2.11 examples guide](https://software-dl.ti.com/msp430/esd/MSPM0-SDK/latest/docs/english/sdk_users_guide/doc_guide/doc_guide-srcs/examples_guide.html)

`dl_mathacl.h` 中真实存在的 operation enum 与数据手册一致：`SINCOS`、`ARCTAN2`、
`DIV`、`SQRT`、`MPY_32`、`SQUARE_32`、`MPY_64`、`SQUARE_64`、`MAC`、`SAC`。
没有在后端中声明硬件不存在的操作。DIV/SQUARE32/MPY32/MAC/SAC 共用饱和控制；
MAC/SAC 使用前需按 DriverLib 约定清理结果寄存器。

## 当前算法库审计

| 类别 | 当前实现 | CMSIS/IQMath 决策 |
|---|---|---|
| FFT/IFFT | `04_dsp/fft` 自写 float radix-2；IFFT 为 `inverse=true` 并除以 N | CMSIS Q15 默认，Q31/F32 可选；Reference 保留 |
| Magnitude | `sqrtf(re²+im²)` | 数据量大时 CMSIS；Q15 输出按 2.14 解读 |
| FIR | 自写 float taps，double accumulator | 长/重复 block 用 CMSIS；短滤波 C 可保留 |
| IIR/Biquad | 不存在 | 未来需要时选 CMSIS biquad，不在本次生成 |
| Statistics | mean/min/max/RMS/AC RMS 均为自写，RMS 多用 double 累加 | block 运算可迁 CMSIS；简单测量保留 C |
| Complex | 自定义复数类型与幅值 | 复数向量内核可用 CMSIS |
| Correlation | 自写归一化相关、限制 lag、double energy | 保留搜索/归一化编排；CMSIS raw correlate 不能直接替代 |
| Phase | float + `fmodf` | IQMath/MathACL 候选，目标周期/误差待测 |
| DDS | 整数 phase accumulator + LUT | 保留；不把 IQMath 放入每 sample 热路径 |
| SineFit | sin/cos/hypot/atan2 + double 求解 | 保留 reference，定点化风险高 |
| Calibration | 简单 float gain/offset | 保留 C，调用频率低 |
| Peak interpolation | 简单 float | 保留 C，收益不足以抵消格式复杂度 |

## PC 数值测试

PC 测试把 SDK 2.11.00.07 的 CMSIS-DSP 源码直接编入测试程序，使用
GCC 13.2、`-Wall -Wextra -Werror`，已实际运行且所有行 PASS。完整 CSV：
`10_tests/backend_benchmark/backend_benchmark_host_results.csv`。

| N | Q15 max abs error | Q31 max abs error | F32 max abs error |
|---:|---:|---:|---:|
| 512 | 2.44147814e-4 | 3.79513949e-8 | 2.07654084e-5 |
| 1024 | 1.22071739e-4 | 3.99304554e-8 | 4.21969453e-5 |
| 2048 | 2.44144903e-4 | 5.53554855e-8 | 1.18678203e-4 |
| 4096 | 1.22074940e-4 | 5.56174200e-8 | 2.34464649e-4 |

另已实际通过：四种点数的 IFFT/round-trip 缩放检查、RMS Q15/F32、Magnitude
Q15/F32、sqrt Q15/F32、atan2 Q15/F32、sin/cos Q15/F32、
ADC→Q15→IQ24→float 转换。这里的 cycle 均为 `PENDING_BOARD`，没有用 PC 时间
冒充 MSPM0 周期。

## TI Clang 目标构建和内存

`build_target.ps1` 使用 SDK SysConfig 生成 80 MHz PLL 配置并检查
`CPUCLK_FREQ == 80000000`，随后用 TI Arm Clang 5.1.1、`-O2 -Wall -Werror`
真实编译链接。完整结果：

- `10_tests/backend_benchmark/build_target/target_backend_build_results.json`
- `10_tests/backend_benchmark/build_target/fft_target_build_matrix.csv`

完整 benchmark 工程（默认 Q15 N=1024，同时含 CMSIS auxiliary 与 IQMath scalar）：

| IQMath variant | Flash | SRAM | Build | Board run |
|---|---:|---:|---|---|
| RTS | 50,544 B | 8,839 B | PASS | PENDING |
| MathACL | 47,728 B | 8,839 B | PASS | PENDING |

单 FFT 目标探针结果：

| Backend | 512 | 1024 | 2048 | 4096 |
|---|---:|---:|---:|---:|
| Reference F32 SRAM | 4,614 | 8,710 | 16,902 | RAM_INFEASIBLE |
| CMSIS Q15 SRAM | 2,562 | 4,610 | 8,706 | 16,898 |
| CMSIS Q31 SRAM | 4,610 | 8,706 | 16,898 | RAM_INFEASIBLE |
| CMSIS F32 SRAM | 4,610 | 8,706 | 16,898 | RAM_INFEASIBLE |

Flash 随固定 FFT 点数链接所需 twiddle/table；详细数字保留在 CSV。4096 点的不可行项
由链接器真实报错 `.bss size 0x8002`，不是估算。

## 工程集成

两个 `.projectspec` 分别选择 RTS 和 MathACL；均使用正式模块的 linked file，
没有复制后端源码。编译搜索路径包含：

- `${COM_TI_MSPM0_SDK_INSTALL_DIR}/source/third_party/CMSIS/Core/Include`
- `${COM_TI_MSPM0_SDK_INSTALL_DIR}/source/third_party/CMSIS/DSP/Include`
- `${COM_TI_MSPM0_SDK_INSTALL_DIR}/source`
- 仓库内 `01_bsp/common`、`04_dsp/fft` 和 `algorithm_backends/*` 的物理路径

SysConfig 的 `device.cmd.genlibs` 已分别生成正确的 CMSIS archive 与 RTS/MathACL
`iqmath.a`。不能把 Project Explorer 的虚拟目录当作 include path。

## 状态

| Backend | 状态 | 边界 |
|---|---|---|
| Reference | `REFERENCE_VERIFIED` | PC 实际运行；目标 512~2048 构建通过 |
| CMSIS-DSP | `CMSIS_HOST_RUNTIME_VERIFIED` + `CMSIS_TARGET_BUILD_VERIFIED` | LP-G3507 runtime/cycles PENDING |
| IQMath RTS | `IQMATH_RTS_TARGET_BUILD_VERIFIED` | runtime/error/cycles PENDING |
| IQMath MathACL | `IQMATH_MATHACL_TARGET_BUILD_VERIFIED` | runtime/error/cycles PENDING |

不能把后两项简写成 `IQMATH_*_VERIFIED`，直到实板运行完成。

## 下一道准入门

在 LP-MSPM0G3507 上分别运行 RTS 和 MathACL benchmark，记录 pass、误差和 cycles；
再决定 Phase/sqrt 是否切到 MathACL。系统集成可以继续，但不能伪称目标周期已验证，
也不能因后端审计改动上层 recipe。
