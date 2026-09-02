# Integration Round 1 Build/Link Closure

> **HISTORICAL:** 本文保留第一轮构建证据；当时的双根目录描述已失效，当前路径以 Canonical Registry 为准。

关闭日期：2026-08-08。

结论：第一轮 8 个 build target 已全部完成 **SysConfig generate → TI Arm Clang 全量编译
→ final application link**，结果 8/8 PASS。当前已经形成可供后续 CMSIS-DSP / IQMath
Backend 优化前使用的稳定构建与资源基线；它是 `BUILD_VERIFIED` 基线，不是板级性能或
测量精度基线。本轮没有进入 Sweep Analyzer 及后续应用，也没有修改算法 Backend。

## A. SysConfig PASS

| 应用目标 | Profile | SysConfig |
|---|---|---:|
| Signal Meter | PROFILE_01_ADC_CAPTURE | PASS |
| Frequency Meter A | PROFILE_05_FREQUENCY | PASS |
| Frequency Meter B | PROFILE_01_ADC_CAPTURE | PASS |
| Frequency Meter C | PROFILE_01_ADC_CAPTURE | PASS |
| Spectrum Analyzer | PROFILE_01_ADC_CAPTURE | PASS |
| THD Analyzer | PROFILE_01_ADC_CAPTURE | PASS |
| Phase Meter | PROFILE_02_DUAL_ADC | PASS |
| DDS Generator | PROFILE_03_DAC_GENERATOR | PASS |

所有目标都生成了 `ti_msp_dl_config.c/.h`、`device.opt`、`device_linker.cmd` 和
`device.cmd.genlibs`。SysConfig 只有 ADC auto-power-down、DMA full channel、Timer retention
等 `info`，没有 warning/error。

## B. Compile PASS

8/8 使用 TI Arm Clang 5.1.1.LTS、`-std=c11 -O2 -Wall -Werror` 编译成功。编译单元数：

| 目标 | Translation units | Compile |
|---|---:|---:|
| Signal Meter | 27 | PASS |
| Frequency Meter A | 4 | PASS |
| Frequency Meter B | 27 | PASS |
| Frequency Meter C | 27 | PASS |
| Spectrum Analyzer | 27 | PASS |
| THD Analyzer | 27 | PASS |
| Phase Meter | 27 | PASS |
| DDS Generator | 8 | PASS |

这里的 PASS 是对象文件实际生成，不是单个 `.c` 的 `-fsyntax-only`。

## C. Link PASS

8/8 均链接 startup、SysConfig generated C、应用 main、正式外设/Adapter、正式算法源和
TI runtime library，生成 `.out` 与 `.map`。严格 projectspec 校验 8/8 PASS；全仓
projectspec 路径审计 14/14 PASS。没有未解析符号、重复定义或 section overflow。

每个 projectspec 使用 `action="link"` 引用正式模块；没有把模块 `.c` 复制到应用目录。
projectspec 与命令行构建共用 `tools/round1_integration_targets.ps1`，避免两套源清单漂移。

## D. Flash / SRAM / Stack / 大型 Buffer

器件容量来自 generated linker map：Flash 131,072 B，SRAM 32,768 B。`SRAM used` 包含
512 B 链接预留 `.stack`；没有运行时 stack watermark，因此不能把 512 B 解释成实测峰值。

| 应用 | Flash used / remain | SRAM used / remain | 静态 SRAM（不含栈） | Stack | 主要大型 Buffer（B） |
|---|---:|---:|---:|---:|---|
| Signal Meter | 7,656 / 123,416 | 14,926 / 17,842 | 14,414 | 512 | events 6,156；voltage 4,096；positions 2,052；raw 2,048 |
| Frequency A | 1,944 / 129,128 | 757 / 32,011 | 245 | 512 | 无 ≥256 B 全局 buffer |
| Frequency B | 6,264 / 124,808 | 14,896 / 17,872 | 14,384 | 512 | events 6,156；voltage 4,096；positions 2,052；raw 2,048 |
| Frequency C | 16,544 / 114,528 | 16,936 / 15,832 | 16,424 | 512 | FFT 8,192；voltage 4,096；magnitude 2,052；raw 2,048 |
| Spectrum | 16,480 / 114,592 | 17,045 / 15,723 | 16,533 | 512 | FFT 8,192；voltage 4,096；magnitude 2,052；raw 2,048 |
| THD | 17,936 / 113,136 | 16,961 / 15,807 | 16,449 | 512 | FFT 8,192；voltage 4,096；magnitude 2,052；raw 2,048 |
| Phase | 16,392 / 114,680 | 15,392 / 17,376 | 14,880 | 512 | FFT A/B 各 4,096；voltage A/B 各 2,048；raw A/B 各 1,024；correlation 516 |
| DDS | 10,560 / 120,512 | 3,244 / 29,524 | 2,732 | 512 | DMA buffer 2,000；lookup table 512 |

最低 SRAM 余量为 Spectrum 的 15,723 B（48.0% 容量仍空闲）；最高 Flash 使用为 THD
的 17,936 B（13.7% Flash 已用）。Map 的 used 数字已与同一 TI 工具链的 `tiarmsize`
交叉核对，8/8 一致。通用 memory-analysis 脚本能发现文件但不能解析 TI 专用 map，故未采用
它显示的 0 B 结果。

## E. 仍为 DRAFT 的范围

Round 1 的 8 个目标已从 `DRAFT` 升为 `BUILD_VERIFIED`。下列应用没有进入本轮，仍为
`DRAFT / NOT_RUN`：

- Sweep Analyzer
- Wave Capture Replay
- Signal Analyzer
- Signal Contest Template

全部 8 个 Round 1 应用的 Board Test 仍为 `NOT_RUN`；没有任何应用被写成
`BOARD_VERIFIED` 或 `CONTEST_VERIFIED`。

## F. 模块接口问题

没有阻断 compile/link 的接口问题，但仍保留以下真实事项：

- INT-001：complex typedef 冲突采用跨层数据隔离，状态 `MITIGATED`，未擅改冻结 API。
- INT-002：Round 1 已强制只链接 sibling 正式算法库；Round 1 source-set 风险已关闭，
  Contest 旧副本的仓库级迁移仍开放。
- INT-003/004/005：Dual ADC、down-count capture、DAC DMA 均已通过正式 Adapter 完成
  full link，状态 `MITIGATED`；动态行为仍需板测。
- INT-006：DDS repeat block 相位闭合/SFDR 是开放的板级质量问题，不是链接问题。

正式处理策略见 `INTEGRATION_ISSUES.md`，连接约束见 `MODULE_INTERFACE_MATRIX.md`。

## G. 资源冲突

当前 8 个目标均为独立固件，每个只使用一个明确 profile，SysConfig 未报告资源冲突：

- P01：单 ADC + DMA，用于 Signal/B/C/Spectrum/THD；
- P02：双 ADC + 两个 DMA channel，用于 Phase；
- P03：DAC + DMA + update Timer，用于 DDS；
- P05：Comparator + capture Timer，用于 Frequency A。

不存在同一目标内的 DMA channel、Event publisher、Timer 或 IRQ 重复占用。不同应用的资源
不能据此推断可同时合并；以后综合应用必须重新做 P06/裁剪 profile 的 SysConfig 冲突检查。
另有两个非冲突但需板测的硬件事实：ADC auto power-down 的 wake-up 时间可能限制高 Fs，
Timer 在 STOP/STANDBY 不保留寄存器。

## H. Backend 修改前基线判定

**是，已经形成稳定的 Backend 修改前 build/link 基线。** 判定依据：

1. 8 个 projectspec 均经过精确 linked-source-set 校验；
2. 8/8 SysConfig、compile、final link PASS；
3. 8 份 `.map`、`.out` 和资源余量已保存并由 `tiarmsize` 交叉核对；
4. Round 1 integration PC truth 4/4 PASS，TI source regression 11/11 PASS；
5. 独立算法库全量 PC 回归重新执行，234 PASS / 0 FAIL；
6. `round1_source_manifest.json` 保存每个目标实际链接 translation unit 的 SHA-256；
7. 工具版本固定为 MSPM0 SDK 2.11.00.07、SysConfig 1.28.0、TI Arm Clang 5.1.1.LTS。

这个基线适合在 Backend 修改前后比较 compile/link、PC truth、Flash 和 SRAM 差异。它尚不
覆盖 MCU 周期数、实时 deadline、栈高水位、模拟精度或板上 SFDR；这些不能从 link PASS 推导。

## 复现入口

```powershell
cd MSPM0_Signal_Contest
powershell -NoProfile -ExecutionPolicy Bypass -File .\tools\generate_round1_projectspecs.ps1
powershell -NoProfile -ExecutionPolicy Bypass -File .\tools\validate_round1_projectspecs.ps1
powershell -NoProfile -ExecutionPolicy Bypass -File .\tools\build_round1_integration.ps1
powershell -NoProfile -ExecutionPolicy Bypass -File .\tools\validate_integration_round1.ps1
```

机器证据位于 `10_tests/integration/round1_build_closure/`。本报告即本轮停止节点；下一阶段
未自动开始。
