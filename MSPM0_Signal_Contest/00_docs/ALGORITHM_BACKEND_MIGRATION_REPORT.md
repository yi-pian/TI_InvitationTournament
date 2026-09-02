# Algorithm Backend Migration Report

> **HISTORICAL BASELINE:** 本文记录合并前的 Backend 迁移基线；其中“独立算法库”仅是历史描述，不是当前模块检索入口。当前路径以 Canonical Registry 为准。

> **SUPERSEDED 2026-08-13：** 下文表格只记录历史决策证据，禁止作为当前选型。Reference/Q31 默认建议已经被 CMSIS-DSP 标准化策略取代。当前工程默认 Q15 candidate；最终默认必须等待板上 cycle/accuracy benchmark。见 `CMSIS_DSP_STANDARDIZATION_REPORT.md`。

日期：2026-08-08  
算法库：`MSPM0_Signal_Contest`  
系统集成基线：8/8 应用已完成 Backend 修改前 Build/Link Closure；算法 PC 基线 `234 PASS / 0 FAIL`。

## 1 历史结论（已失效，不得用于新工程）

| 项目 | 结论 |
|---|---|
| 源码安全默认 FFT | `REFERENCE_C`，不新增依赖，保护现有 8 个应用 |
| 推荐 Competition FFT | `CMSIS_Q31`，因为完整 234 项回归实跑为 `234/0` |
| Q15 FFT | PC backend `26/0`，但旧 THD/Phase 严格回归各失败 1 项；不是稳定默认 |
| Phase backend | 默认 `REFERENCE_FLOAT`；可选 IQMath RTS/MATHACL atan2 |
| RMS backend | 默认 `REFERENCE_FLOAT`；可选 IQMath RTS/MATHACL sqrt |
| FIR/IIR backend | 仍为 Reference float；未强行改现有实例结构 |
| 公开 API | **没有 BREAKING CHANGE**；旧公开头文件和函数签名未改 |
| 数据类型 | 旧 API 仍为 float；只新增可选 Q15 Adapter API |
| 4096 点 | 当前 `SignalFFT_ForwardReal` 在 32 KB SRAM 上真实链接失败，禁止使用 |
| 板上 cycle | `PENDING_BOARD`，没有伪造 runtime 数据 |

## 2 PRE-BACKEND BASELINE

- 8 个系统集成应用的 `.out/.map`、Flash/SRAM、source manifest、PC truth 数据由系统集成层保存，本任务没有修改它们；
- 独立算法库 PRE-BACKEND 共有 90 个 `.c/.h`，内容清单 SHA-256：`aa75f66db61e047a1fe03e708b89322c2001d2d5e721216e057c45295e43ed89`；
- PRE-BACKEND 实跑：`234 PASS / 0 FAIL`；
- PRE-BACKEND 没有 CMSIS/IQMath/MATHACL 实际调用。

本轮后算法源码/头文件为 95 个；按“每个文件 SHA-256 + 相对路径排序后再 SHA-256”的后端审计清单值为 `c1a309eb24b54daae5c58ddadc163c68dc0038453eaaae42d353e4f1bce64570`。这不是替代系统集成层自己的 manifest 算法，只用于本轮算法库追踪。

## 3 公开 API 兼容性

### 3.1 没有改动

以下旧函数签名保持不变：

- `SignalFFT_ForwardComplexInPlace()`
- `SignalFFT_ForwardReal()`
- `SignalRMS_Process()`
- `SignalACRMS_Process()`
- `SignalStatistics_Process()`
- `SignalPhase_FromFFTBin()` 及其他 Phase API
- FIR/IIR/THD/Harmonic/Magnitude 的全部旧 API

`signal_fft.h` 的 SHA-256 仍为：

`8c9f8431d2783771cc7158dc5d1734cab72d0bcfc93916f7e58e64d3081f34b6`

### 3.2 新增但不破坏

- `signal_fft_backend_config.h`：编译期选择 FFT backend；
- `signal_math_backend_config.h`、`signal_math_backend.h`：内部标量数学选择；
- `signal_backend_adapter.h/.c`：RAW/float/Q15/Q30 Adapter。

没有删除或重命名任何旧符号。CMSIS backend 对 16～4096 点使用 CMSIS；对 CMSIS 没有常量表但旧 Reference 支持的合法点数自动回退 Reference，避免 8 点测试发生行为断裂。

## 4 Backend 选择方式

### FFT

编译器预定义：

```text
SIGNAL_FFT_BACKEND=0  Reference C（默认）
SIGNAL_FFT_BACKEND=1  CMSIS Q15
SIGNAL_FFT_BACKEND=2  CMSIS Q31（稳定 Competition 推荐）
SIGNAL_FFT_BACKEND=3  CMSIS F32
```

Recipe 和应用代码仍调用 `SignalFFT_ForwardReal()`；不要直接写 `arm_cfft_q31()`。

### 标量数学

```text
SIGNAL_MATH_BACKEND=0  Reference math.h（默认）
SIGNAL_MATH_BACKEND=1  IQMath RTS
SIGNAL_MATH_BACKEND=2  IQMath MATHACL library
```

RMS 使用内部 sqrt Adapter，FFT-bin Phase 使用内部 atan2 Adapter。RTS/MATHACL 的 C 函数名相同，最终实现由链接的 `iqmath.a` 决定。

## 5 新增 include/library 依赖

### CMSIS Q31/Q15/F32 档

编译：

```text
-DARM_MATH_CM0
-DSIGNAL_FFT_BACKEND=2          # Q31 示例
-mcpu=cortex-m0plus
-march=thumbv6m
-mfloat-abi=soft
-mthumb
-fno-strict-aliasing
```

Include：

```text
${MSPM0_SDK}/source/third_party/CMSIS/Core/Include
${MSPM0_SDK}/source/third_party/CMSIS/DSP/Include
```

TI Arm Clang library：

```text
${MSPM0_SDK}/source/third_party/CMSIS/DSP/Lib/ticlang/m0p/arm_cortexM0l_math.a
```

CCS/SysConfig 正常工程应启用 `ProjectConfig.genLibCMSIS = true`，让 `device.cmd.genlibs` 选择正确库。离线 smoke 直接链接了同一 SDK 静态库。

### IQMath RTS

Include：`${MSPM0_SDK}/source/ti/iqmath/include/IQmathLib.h`  
Library：`${MSPM0_SDK}/source/ti/iqmath/lib/ticlang/m0p/rts/mspm0g1x0x_g3x0x/iqmath.a`

### IQMath MATHACL

Library：`${MSPM0_SDK}/source/ti/iqmath/lib/ticlang/m0p/mathacl/mspm0g1x0x_g3x0x/iqmath.a`

正常 SysConfig 工程还必须导入 MATHACL、选择 `genLibIQVersion="MATHACL"`，并在调用前完成系统/外设初始化。离线 smoke 只证明编译和最终链接，不证明初始化和运行周期。

## 6 PC 数值结果

四种 FFT 后端对 independent double radix-2 truth，覆盖 512/1024/2048/4096 和 clean sine、noisy sine、harmonic sine、two tone、DC-offset sine、clipped sine：

| Backend | Backend benchmark | Hann 0.5 V 恢复 | 完整旧回归 |
|---|---:|---:|---:|
| Reference C | 26/0 | 0.499998628 V | 234/0 |
| CMSIS Q15 | 26/0 | 0.500131542 V | 重点三批 78/2；未通过旧严格门槛 |
| CMSIS Q31 | 26/0 | 0.499998777 V | **234/0，已完整实跑** |
| CMSIS F32 | 26/0 | 0.499998747 V | 重点三批 80/0 |

Q15 的两个旧测试差异：

| 指标 | Expected | Q15 | Absolute error | 结果 |
|---|---:|---:|---:|---|
| THD percent | 11.180340767 | 11.183106422 | 0.002765655 percentage point | 旧阈值 FAIL |
| FFT Phase deg | 30.000000000 | 30.001083374 | 0.001083374° | 旧阈值 FAIL |

因此没有为了让 Q15 “看起来通过”而放宽旧测试。

## 7 Target Build/Link、RAM、Flash

TI Arm Clang 离线目标链接结果：

| Backend | 512 | 1024 | 2048 | 4096 | Flash smoke |
|---|---|---|---|---|---:|
| Reference | PASS | PASS | PASS | SRAM FAIL | 8,936 B |
| CMSIS Q15 | PASS | PASS | PASS | SRAM FAIL | 55,712 B |
| CMSIS Q31 | PASS | PASS | PASS | SRAM FAIL | 82,016 B |
| CMSIS F32 | PASS | PASS | PASS | SRAM FAIL | 101,872 B |

旧公开 real FFT API 的 RAM 与 backend 无关：输入 `4N` + 复输出 `8N`。2048 点 smoke 含 512 B stack 使用 25,096 B；4096 点 `.bss=0xC004`，需要约 49.7 KB，链接器正确拒绝。

标量 RMS+Phase smoke：Reference 3,192 B Flash，IQMath RTS 4,312 B，IQMath MATHACL 2,656 B；三者均最终链接成功。该比较不含应用其他代码，不能直接替换 8 个应用的最终 `.map`。

## 8 为什么稳定 Competition FFT 选 Q31

Q15 的优点是固定点宽度小、CMSIS Flash 比 Q31 少，理论上也更符合无 FPU M0+；但本轮没有开发板 cycle 数据，而且它没有保持原 THD/Phase 严格回归。Q31 同样是定点、完整 234/0、Hann 幅值误差接近 float，因此在现有四个重点应用上风险更低。

选择规则：

- 先让 Frequency Meter C、Spectrum Analyzer、THD Analyzer、Phase Meter 使用 Q31 做系统回归；
- 只有当最终 `.map` 或板上周期证明 Q31 不满足资源，且题目容许上述 Q15 数值误差，才切 Q15；
- F32 在无 FPU 上没有板上周期优势证据，且本次 Flash 最大，不作为默认。

## 9 FIR / IIR / 其他模块

FIR 和 IIR 没有迁移。原因不是 CMSIS 不支持，而是旧公开实例结构、系数顺序和状态布局已被应用依赖；强行把 CMSIS 实例塞进去会产生结构体/API breaking change。Reference 实现继续是 BUILD/PC verified。后续若做原生 Q15 filter，应新增并行实例类型和转换工具，不能改写旧结构体含义。

Correlation、Autocorrelation、Magnitude、Statistics 也暂保留 Reference。缺少 MSPM0G3507 cycle 证据时，仅把循环换成 CMSIS 函数不能证明系统收益，尤其当前公开数据仍是 float。

## 10 系统集成层下一步

现有 8 个应用不定义新宏时，行为和依赖保持原样。若要试 Competition Q31：

1. 仅改各应用的编译/链接配置，不改调用代码；
2. 定义 `SIGNAL_FFT_BACKEND=2` 和 `ARM_MATH_CM0`；
3. 加 CMSIS Core/DSP include 与 TI Clang M0+ 静态库；
4. 先重建 Frequency Meter C、Spectrum Analyzer、THD Analyzer、Phase Meter；
5. 对比 PRE-BACKEND `.map/.out`、Flash/SRAM、truth regression；
6. 1024 点优先；2048 点必须看整应用最终 SRAM；禁止当前 API 的 4096 点；
7. 未实测前保持 `BOARD_RUNTIME_VERIFIED=PENDING`。

本任务没有修改任何系统集成应用目录、外设模块、ADC/DMA/DAC/Timer/OPA/Comparator，也没有重新生成赛题工程。

## 11 验证状态总表

| 项目 | 状态 |
|---|---|
| Reference full PC | REFERENCE_VERIFIED，234/0 |
| CMSIS Q15 numeric benchmark | CMSIS_PC_VERIFIED，26/0；旧完整兼容门槛未通过 |
| CMSIS Q31 full PC | CMSIS_PC_VERIFIED，234/0 |
| CMSIS F32 key regressions | CMSIS_PC_VERIFIED，80/0 |
| CMSIS target 512/1024/2048 | CMSIS_TARGET_BUILD_VERIFIED（离线最终链接） |
| CMSIS target 4096 public API | EXPECTED LINK FAILURE / SRAM OVER BUDGET |
| IQMath RTS RMS+Phase | IQMATH_TARGET_BUILD_VERIFIED |
| IQMath MATHACL RMS+Phase | MATHACL_TARGET_BUILD_VERIFIED |
| SysConfig 本轮重新生成 | BLOCKED BY RESTRICTED TI USER CACHE；未伪造 |
| Board cycle/runtime | PENDING_BOARD |
| BOARD_RUNTIME_VERIFIED | NO |
