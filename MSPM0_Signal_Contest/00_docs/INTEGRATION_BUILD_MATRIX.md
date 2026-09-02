# Integration Build Matrix

更新时间：2026-08-13。本文是当前 Application Build/Link 的唯一人工入口；历史阶段矩阵保存在 `00_docs/archive/`，不再代表当前仓库状态。

> CMSIS 标准化增量：第一轮 8 个 Application 已在默认 CMSIS Q15 FFT 路径重新完成 SysConfig/Compile/Full Link。下面 Frequency C、Spectrum、THD、Phase 的旧低 Flash 数字已由最新验证替换；其余 Final Application 的数字仍是上一轮有效基线。

验证环境：MSPM0G3507、MSPM0 SDK 2.11.00.07、SysConfig 1.28.0、TI Arm Clang 5.1.1.LTS。Flash 总量按 131072 B、SRAM 总量按 32768 B 计算；SRAM 已含 512 B linker stack。所有数据来自本轮真实 `.map` 与 `tiarmsize` 交叉检查。

| Application | SysConfig | Compile | Full link | Flash used / remaining | SRAM used / remaining | Stack | Board |
|---|---|---|---|---:|---:|---:|---|
| Signal Meter | PASS | PASS | PASS | 7648 / 123424 B | 14926 / 17842 B | 512 B | NOT_RUN |
| Frequency Meter A | PASS | PASS | PASS | 2368 / 128704 B | 780 / 31988 B | 512 B | NOT_RUN |
| Frequency Meter B | PASS | PASS | PASS | 6256 / 124816 B | 14896 / 17872 B | 512 B | NOT_RUN |
| Frequency Meter C | PASS | PASS | PASS | 63048 / 68024 B | 16936 / 15832 B | 512 B | NOT_RUN |
| Spectrum Analyzer | PASS | PASS | PASS | 62984 / 68088 B | 17045 / 15723 B | 512 B | NOT_RUN |
| THD Analyzer | PASS | PASS | PASS | 64456 / 66616 B | 16961 / 15807 B | 512 B | NOT_RUN |
| Phase Meter | PASS | PASS | PASS | 62848 / 68224 B | 15392 / 17376 B | 512 B | NOT_RUN |
| DDS Generator | PASS | PASS | PASS | 10560 / 120512 B | 3244 / 29524 B | 512 B | NOT_RUN |
| Sweep Analyzer | PASS | PASS | PASS | 18336 / 112736 B | 9687 / 23081 B | 512 B | NOT_RUN |
| Wave Capture Replay | PASS | PASS | PASS | 7448 / 123624 B | 18173 / 14595 B | 512 B | NOT_RUN |
| Signal Analyzer | PASS | PASS | PASS | 91016 / 40056 B | 9999 / 22769 B | 512 B | NOT_RUN |
| Signal Contest Template | PASS | PASS | PASS | 2728 / 128344 B | 838 / 31930 B | 512 B | NOT_RUN |

## 主要静态 Buffer

| Application | `.map` 中较大 Buffer |
|---|---|
| Signal Meter | crossing events 6156 B；voltage 4096 B；crossing positions 2052 B；raw 2048 B |
| Frequency Meter A | 无 256 B 以上命名静态 Buffer |
| Frequency Meter B | events 6156 B；voltage 4096 B；positions 2052 B；raw 2048 B |
| Frequency Meter C / Spectrum / THD | FFT 8192 B；voltage 4096 B；magnitude 2052 B；raw 2048 B |
| Phase Meter | FFT A/B 各 4096 B；voltage A/B 各 2048 B |
| DDS Generator | DMA buffer 2000 B；lookup table 512 B |
| Sweep Analyzer | voltage 4096 B；raw 2048 B；DDS block 2000 B；table 512 B |
| Wave Capture Replay | ring 4098 B；period/order/DMA buffers 各 4096 B |
| Signal Analyzer | FFT A 4096 B；voltage A 2048 B；magnitude 1028 B；raw B 1024 B |
| Signal Contest Template | 无 256 B 以上命名静态 Buffer |

## 证据与边界

- Round 1 原始结果：`10_tests/integration/round1_build_closure/round1_build_results.json`。
- Final Application 原始结果：`10_tests/integration/final_build_closure/round1_build_results.json`。
- 17 个正式 projectspec 路径检查 PASS，Round 1 的 8 个 projectspec 结构检查 PASS。
- 本表只证明当前源码、SysConfig、编译、完整链接和静态资源闭合。未执行上板、模拟输入、DAC 输出、屏幕、相位同步或竞赛指标测试，因此全部保持 `Board = NOT_RUN`。
