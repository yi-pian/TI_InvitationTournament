# Integration Backend Migration Matrix

更新时间：2026-08-08。本表比较保留的 PRE-BACKEND SYSTEM BASELINE 与完整应用使用 `SIGNAL_FFT_BACKEND=2`（CMSIS-DSP Q31）后的真实 TI Arm Clang 链接结果。公共算法 API 未改变，应用层未直接调用 CMSIS。

## Round 1 回归

| Application | Old → New Backend | Before Flash | After Flash | Δ Flash | Before SRAM | After SRAM | Δ SRAM | PC Truth | SysConfig / Compile / Link |
|---|---|---:|---:|---:|---:|---:|---:|---|---|
| Frequency Meter C | Reference C → CMSIS Q31 | 16,544 B | 89,368 B | +72,824 B | 16,936 B | 16,936 B | 0 B | PASS | PASS / PASS / PASS |
| Spectrum Analyzer | Reference C → CMSIS Q31 | 16,480 B | 89,320 B | +72,840 B | 17,045 B | 17,045 B | 0 B | PASS | PASS / PASS / PASS |
| THD Analyzer | Reference C → CMSIS Q31 | 17,936 B | 90,776 B | +72,840 B | 16,961 B | 16,961 B | 0 B | PASS | PASS / PASS / PASS |
| Phase Meter | Reference C → CMSIS Q31 | 16,392 B | 89,168 B | +72,776 B | 15,392 B | 15,392 B | 0 B | PASS | PASS / PASS / PASS |

PC Truth 为 `validate_integration_round1.ps1 -FftBackend 2` 的 4/4 集成真值回归；算法库 Q31 总回归为 234 PASS / 0 FAIL。构建产物位于 `10_tests/integration/round1_backend_q31/`，原基线仍位于 `round1_build_closure/`，未被覆盖。

Q31 静态库在 M0+ 上带来约 72.8 KiB Flash 固定成本，但未增加应用静态 SRAM。四个应用仍低于 MSPM0G3507 的 128 KiB Flash。

## FFT 点数完整应用验证

| N | Frequency C | Spectrum | THD | Phase | 结论 |
|---:|---|---|---|---|---|
| 512 | PASS，89,368 / 8,744 B | PASS，89,304 / 8,853 B | PASS，90,776 / 8,769 B | PASS，89,168 / 15,392 B | 推荐低延迟、低 RAM 配置 |
| 1024 | PASS，89,368 / 16,936 B | PASS，89,320 / 17,045 B | PASS，90,776 / 16,961 B | PASS，89,168 / 29,728 B | 单通道默认推荐；双通道相位仅余 3,040 B SRAM |
| 2048 | Frequency C 完整编译后链接失败；`.bss=0x8010` 已超过 32 KiB | 不再伪测 PASS | 不再伪测 PASS | 不再伪测 PASS | 当前完整 Simple Pipeline 不支持 |
| 4096 | 未作为 PASS 目标执行 | 未执行 | 未执行 | 未执行 | `UNSUPPORTED_BY_CURRENT_SIMPLE_FFT_API` |

表中每个数值均为 `Flash / SRAM（含 512 B 保留栈）`。1024 点 Phase 虽可链接，但余量不足以容纳较大运行时栈、通信缓存或 UI；比赛默认采用 512 点 Phase。

## 系统策略

- 稳定比赛 FFT Backend：`CMSIS Q31`。
- 标量 Math Backend：保持 `Reference math.h`；IQMath/MATHACL 等待实板 cycle benchmark 后再按题切换。
- Q15：保留为 RAM/性能实验候选；旧 THD 与 Phase 严格阈值各有小误差，不能设为稳定默认。
- 4096 点若要落地，必须由算法库提供端到端 Native Q15 数据通路；集成层不得重新发明第二套 FFT。
- Backend 依赖只出现在 projectspec/构建清单和算法实现内部；`08_applications/**` 禁止出现 `arm_cfft_*`、`_IQ*`、`DL_MATHACL_*` 调用。
