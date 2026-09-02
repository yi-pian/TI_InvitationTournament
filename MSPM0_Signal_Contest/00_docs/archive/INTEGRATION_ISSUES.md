# Integration Issues

> **HISTORICAL:** 本文记录合并前问题与处置过程，不作为当前路径或模块选择依据。

| ID | 问题 | 正式处理策略 | 状态 / 证据 |
|---|---|---|---|
| INT-001 | 外设 `signal_types.h` 与算法 `signal_complex.h` 都定义 `signal_complex_f32_t`，同 TU include 会冲突 | 不做破坏性 API 重命名；硬件边界只发布 raw/N/Fs，FFT TU 只使用算法 complex；未来统一 ABI 必须单独迁移 | MITIGATED；Round1/Final full links PASS |
| INT-002 | Contest 仓库存在与 sibling 算法仓库同名旧源码，include/source 顺序可能选错 | projectspec 与 CLI build 共用唯一 manifest；严格校验器拒绝链接 Contest 旧 `03_measurement/04_dsp/05_precision` 源；旧目录只在完整依赖迁移后受控清理 | MITIGATED；所有 final projectspec strict PASS；repo cleanup OPEN |
| INT-003 | DualADC 真实输出是两块 DMA buffer，不是旧 interleaved 形式 | `signal_dual_adc_platform.*` 发布 A/B 独立 raw buffer；各做一次 RawToVoltage | MITIGATED；Phase/Analyzer/Template link PASS，PENDING_BOARD |
| INT-004 | P05 Capture Timer 向下计数，而 MeanPeriod 需要正向时间戳 | ISR 集中转换 `modulus-1-capture`，再调用正式 API | MITIGATED；Frequency A link PASS，PENDING_BOARD |
| INT-005 | DACDMA 是 callback wrapper，不直接驱动 TI 硬件 | `signal_dac_dma_platform.*` 是唯一 Timer/Event/DMA/DAC Adapter | MITIGATED；DDS/Sweep/Replay links PASS |
| INT-006 | DDS repeat block 非整数周期闭合会产生边界跳变与杂散 | 配置要求检查 `f × block_count / update_rate`；真实 SFDR 等待频谱仪 | OPEN；不阻塞 build，阻止 BOARD_VERIFIED |
| INT-007 | Generic memory-analysis parser 不识别 TI Arm Clang map，返回 0 B | 使用项目 TI-map parser 并以 `tiarmsize` 逐目标交叉验证；generic 结果不进入报告 | MITIGATED；全部成功链接目标 cross-check PASS |
| INT-008 | Simple FFT N=2048 完整应用 SRAM 溢出 | 保留真实失败；默认 N=1024 单通道/N=512 phase；4096 标 unsupported | CLOSED_AS_LIMITATION；见 Backend Matrix |

原则：应用中不复制修改模块；新赛题暴露问题时在这里登记，然后回正式唯一模块修复并重跑 projectspec、full link、PC truth 与 map。
