# SOURCE_LOST Clean Reimplementation Round 1（历史检查点）

> 本文冻结的是第一轮当时状态，不是当前状态。9 个 SOURCE_LOST 模块现已全部 clean reimplementation；最新结论见 [SOURCE_LOST_REIMPLEMENTATION_FINAL.md](SOURCE_LOST_REIMPLEMENTATION_FINAL.md)。下文出现的 `REIMPLEMENTATION_REQUIRED` 仅表示当时尚未完成。

日期：2026-08-13  
本轮范围：只完成 Duty；不开始 Jacobsen、Quinn、Macleod、CZT 或其他源码丢失模块。

## 结论

- 确认 `SOURCE_LOST` 可执行模块：9 个。
- 本轮 clean reimplementation：1 个（Duty）。
- 仍为 `REIMPLEMENTATION_REQUIRED`：8 个。
- 从原源码恢复：0 个。
- Duty 当前验证等级：`BUILD_VERIFIED`；PC reference/differential test 为 PASS；Board 为 `NOT_RUN`。
- 四项永久迁移/删除门禁全部 PASS。

新 Duty 不是旧实现恢复，不继承旧 `BUILD_VERIFIED`，也不是旧 API 的 drop-in replacement。

## Duty 新实现

正式入口：[`03_measurement/duty/README.md`](../../03_measurement/duty/README.md)  
重实现规格：[`03_measurement/duty/REIMPLEMENTATION_SPEC.md`](../../03_measurement/duty/REIMPLEMENTATION_SPEC.md)  
验证真源：[`03_measurement/duty/VERIFICATION.yaml`](../../03_measurement/duty/VERIFICATION.yaml)

新 API：

```c
signal_algorithm_status_t SignalDuty_GetDefaultConfig(
    signal_duty_config_t *config);

signal_algorithm_status_t SignalDuty_Process(
    const float *samples,
    uint32_t count,
    float sample_rate_hz,
    const signal_duty_config_t *config,
    signal_duty_result_t *result);
```

测量链为：自动或显式高低电平 → 中间阈值 → 迟滞状态确认 → crossing 线性插值 → `rise-fall-next rise` 严格配对 → 多完整周期时间加权平均。定义与实现依据写入规格，参考入口包括 [IEEE 181-2025](https://standards.ieee.org/ieee/181/10551/)、[NI Reference and State Levels](https://www.ni.com/docs/en-US/csh?context=lvcore_lvwave_state_and_reference_levels) 和 [Keysight Duty Cycle](https://helpfiles.keysight.com/scopes/FlexDCA-UG/Content/Topics/Oscilloscope-Mode/Time-Measurements/duty_cycle.htm)。

旧到新 API 映射：

| 旧符号 | 新入口 | 兼容性 |
|---|---|---|
| `SignalDuty_F32` | `SignalDuty_Process` | `NOT_DROP_IN_COMPATIBLE` |
| `SignalDuty_GetModuleStatus` | `VERIFICATION.yaml` 中的证据元数据 | 不提供运行时兼容函数 |

## 验证结果

| 检查 | 结果 | 证据 |
|---|---|---|
| Python reference 扫描 | PASS | 540 cases，0 fail |
| Python 边界测试 | PASS | 6 tests |
| C/Python 同输入差分 | PASS | 3 vectors，36 field comparisons；含 7 类错误路径 |
| TI Arm Clang 编译 | PASS | Duty 与最小调用者均编译 |
| TI Cortex-M0+ 完整目标链接 | PASS | 生成 `.out` 与 `.map` |
| Board | `NOT_RUN` | 未伪造开发板验证 |

Reference 扫描中：无噪声 duty 最大绝对误差 `3.33e-16`；40 dB SNR RMSE `6.03e-4`；30 dB SNR RMSE `1.78e-3`。C/Python 的 `duty_ratio` 最大绝对差约 `7.92e-8`。

TI 专项完整链接 map（包含 startup、现有 SysConfig 生成对象、最小调用者和测试波形，不是 Duty 函数单独尺寸）：

| 资源 | 使用 | 总量 | 余量 |
|---|---:|---:|---:|
| Flash | 3048 B | 131072 B | 128024 B |
| SRAM | 720 B | 32768 B | 32048 B |
| Stack reservation | 512 B | 计入 SRAM | — |
| `.bss` | 208 B | 主要为 160 B 测试波形和 48 B result | — |

Map：[`10_tests/ticlang/duty_reimplementation/build/duty_reimplementation.map`](../../10_tests/ticlang/duty_reimplementation/build/duty_reimplementation.map)

## 永久门禁

以下门禁已经进入 `tools/canonical_repository/migration_delete_gates.py`，实际仓库与负向 fixture 均 PASS：

1. `EXECUTABLE_MODULE_DELETE_CHECK`：README/Recipe 不能代替 `.c/.h` 可执行模块。
2. `PUBLIC_API_PRESERVATION_CHECK`：删除或改 API 必须有显式迁移和兼容性结论。
3. `VERIFICATION_REGRESSION_CHECK`：新实现不能继承旧验证等级。
4. `EXECUTABLE_MODULE_LOSS_CHECK`：源码丢失必须进入 `SOURCE_LOST_MODULES.yaml`，不能被静默隐藏。

本轮门禁覆盖旧可执行模块 38 个，显式跟踪 `SOURCE_LOST` 9 个。

## 仍待重实现

| 优先级 | 模块 | 当前状态 |
|---|---|---|
| P0 | jacobsen_interpolation | `REIMPLEMENTATION_REQUIRED` |
| P0/P1 | quinn_interpolation | `REIMPLEMENTATION_REQUIRED` |
| P0/P1 | macleod_interpolation | `REIMPLEMENTATION_REQUIRED` |
| P1 | coherent_sampling | `REIMPLEMENTATION_REQUIRED` |
| P1 | frequency_response_correction | `REIMPLEMENTATION_REQUIRED` |
| P2 | czt | `REIMPLEMENTATION_REQUIRED` |
| P2 | dc_measure | `REIMPLEMENTATION_REQUIRED` |
| P2 | fft_peak | `REIMPLEMENTATION_REQUIRED` |

这些模块没有在本轮创建任何猜测性 `.c/.h`。

## 发现但未扩展处理的集成问题

Duty 专项完整链接已经形成独立、可复现的验证基线。与此同时，旧聚合 PC 测试和 Spectrum Analyzer 仍引用已删除的 `signal_frequency_interpolation.h`、`signal_hann_window.h`、`signal_fft_peak.h` 及旧函数名。这是已有 Application/aggregate migration 问题，不是 Duty 新实现失败。

本轮按“只完成 Duty 后停止”的范围没有改写这些 Application；后续应作为独立 `INTEGRATION_ISSUE` 迁移，并继续保持 one source of truth。
