# Portable Application Build Matrix

验证日期：2026-08-09。

验证环境：CCS `21.0.0.00014`，TI Arm Clang `5.1.1.LTS`，MSPM0 SDK `2.11.0.07`，CCS bundled SysConfig `1.28.0`，目标器件 `MSPM0G3507`。

状态含义：

- `PASS`：在不同于原 `08_applications` 层级的复制目录导入，并真实完成该阶段。
- `STATIC PASS`：projectspec 已通过 XML、路径变量、正式唯一源、include、SysConfig/SDK/CMSIS 路径门禁，但本轮没有再做一份独立搬运构建。
- `NOT RUN`：本轮没有对该变体执行独立搬运构建，不表示原有 BUILD_VERIFIED 被取消。

## 指定搬运测试

| Application / 导入目标 | 复制到原目录外 | CCS Import | SysConfig | Compile | Final Link | `.out` | `.map` | PORTABLE_IMPORT_BUILD |
|---|---:|---:|---:|---:|---:|---:|---:|---:|
| Signal Meter / `signal_meter_round1` | PASS | PASS | PASS | PASS | PASS | PASS | PASS | **PASS** |
| Frequency Meter / `frequency_meter_c_q31` | PASS | PASS | PASS | PASS | PASS | PASS | PASS | **PASS** |
| THD Analyzer / `harmonic_thd_analyzer_q31` | PASS | PASS | PASS | PASS | PASS | PASS | PASS | **PASS** |
| Spectrum Analyzer / `spectrum_analyzer_q31` | PASS | PASS | PASS | PASS | PASS | PASS | PASS | **PASS** |
| Contest Template / `signal_contest_template_final` | PASS | PASS | PASS | PASS | PASS | PASS | PASS | **PASS** |

五个工程均为 CCS full build，均报告 `0 errors, 0 warnings`。验证时，每个 Debug 输出目录均真实存在 SysConfig 生成的 `ti_msp_dl_config.c/.h`、编译对象、最终 `.out` 和 `.map`；验证结束后已清理临时复制件和临时 CCS workspace。

搬运位置采用两种与原目录无关的深层路径：

```text
_portable_application_test/relocated_sources/level_a/level_b/signal_meter/
_portable_application_test/relocated_sources/unrelated_branch/deep/applications/<application>/
```

## 全部正式 Application projectspec 策略检查

| Application projectspec | Portable policy | 本轮独立搬运 Build |
|---|---:|---:|
| `signal_meter_round1` | PASS | PASS |
| `frequency_meter_a_round1` | PASS | NOT RUN |
| `frequency_meter_b_round1` | PASS | NOT RUN |
| `frequency_meter_c_round1` | PASS | NOT RUN |
| `frequency_meter_c_q31` | PASS | PASS |
| `spectrum_analyzer_round1` | PASS | NOT RUN |
| `spectrum_analyzer_q31` | PASS | PASS |
| `harmonic_thd_analyzer_round1` | PASS | NOT RUN |
| `harmonic_thd_analyzer_q31` | PASS | PASS |
| `dual_channel_phase_meter_round1` | PASS | NOT RUN |
| `dual_channel_phase_meter_q31` | PASS | NOT RUN |
| `dds_generator_round1` | PASS | NOT RUN |
| `sweep_analyzer_final` | PASS | NOT RUN |
| `waveform_capture_replay_final` | PASS | NOT RUN |
| `signal_analyzer_final` | PASS | NOT RUN |
| `signal_contest_template_final` | PASS | PASS |
| `peripheral_system_template` | PASS | NOT RUN |

静态门禁合计：**17/17 PASS**。指定真实搬运构建：**5/5 PASS**。

## 本轮修复的问题

1. 原 include 使用 `${PROJECT_ROOT}/../../..`，导入到新位置后从新工程目录解析；linked `.c` 却在导入时按原 projectspec 位置解析，因此出现“源码存在、头文件找不到”。
2. Q31 projectspec 原先把 CMSIS archive 写成裸路径。CCS 导入后将其与 `libc.a` 合并为一个错误库名；现已改成 TI projectspec 使用的 `-l<archive>` 形式并完成真实 final link。
3. `signal_status.h`、`signal_algorithm_status.h`、`signal_window.h`、`signal_adc_dma.h` 的 include 目录均已在搬运工程生成的 `ccsIncludes.opt` 中解析为正式仓库绝对位置，并通过实际编译。

验证范围截止于 SysConfig、目标编译和链接；本轮未执行开发板运行，因此没有新增 `BOARD_VERIFIED`。
