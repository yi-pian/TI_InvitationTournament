# Integration Round 1 Checkpoint

> **HISTORICAL:** 本文是合并前检查点，不作为当前模块路径或源码来源。

> 2026-08-08：本检查点中的 compile/link `NOT_RUN` 已由
> `INTEGRATION_ROUND_1_BUILD_CLOSURE.md` 关闭。本文保留为进入闭环前的历史记录，
> 当前状态请以 Closure 和 `INTEGRATION_BUILD_MATRIX.md` 为准。

暂停日期：2026-08-08。暂停点：完成 DDS Generator 的源码级集成与第一轮可重复验证，
尚未进入 Sweep Analyzer。

## 本轮实际完成

- 扫描并核对关键真实 API：ADC_DMA、DualADC 数据形状、TimerCapture、ADC_ToVoltage、
  FFT complex/magnitude、DDS、DAC/DAC_DMA。
- 新增轻量 Glue：`signal_integration.*`，集中处理 Raw→Voltage、时域测频、FFT 链、THD、
  双通道相位；没有复制算法实现。
- 新增必要硬件 Adapter：双 ADC 两路 DMA、DAC DMA；应用不再重复寄存器 for-loop。
- 建立带 `main.c + signal_config.h + README` 的 Signal Meter、Frequency Meter、Spectrum、
  THD、Phase、DDS 源码入口。
- 新增 `INTEGRATION_ISSUES.md` 和本 Build Matrix，如实保留未闭环项。

## 验证证据

`validate_integration_round1.ps1` 本轮结果：

- PC 合成真值 4/4 PASS：Signal Meter、Spectrum、THD、FFT/Correlation Phase。
- TI Arm Clang 源码检查 11/11 PASS：3 个共享源 + Signal Meter + Frequency A/B/C +
  Spectrum + THD + Phase + DDS。
- 完整 application compile/link：NOT_RUN。
- 开发板：NOT_RUN。

因此所有应用仍标 `DRAFT`，没有伪写 BUILD/BOARD/CONTEST_VERIFIED。

## 下一轮从这里继续

1. 生成并校验第一轮每个应用的 projectspec，链接独立算法仓库唯一源码。
2. 运行 SysConfig、完整 compile/link、读取 `.map`，收紧实际 RAM 数字。
3. 完成 `INTEGRATION_MODULE_INDEX.md` 和最终 `MODULE_INTERFACE_MATRIX.md`，把 INT-001/002
   纳入正式迁移策略。
4. 第一轮 link 闭环后再进入 Sweep Analyzer、Wave Capture Replay、Signal Analyzer、
   Contest Template。

当前不应直接上板声称测量精度；先完成第 1~2 项。
