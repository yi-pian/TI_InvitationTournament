# SOURCE_LOST 模块清单

上一次合并删除了 9 个仍含正式 `.c/.h` 的旧模块，且原源码已无法从备份或版本历史取回。它们必须走 `SOURCE_LOST → CLEAN_REIMPLEMENTATION`，新代码只能称为 **REIMPLEMENTED**，不能称为 **RESTORED**。

机器可读真源是同目录的 `SOURCE_LOST_MODULES.yaml`。

| 优先级 | 模块 | 旧状态 | 当前处理 |
|---|---|---|---|
| P0 | duty | BUILD_VERIFIED | `CLEAN_REIMPLEMENTED / BUILD_VERIFIED`；新 API，非旧 API drop-in |
| P0 | jacobsen_interpolation | BUILD_VERIFIED | `CLEAN_REIMPLEMENTED / BUILD_VERIFIED`；新 API |
| P0/P1 | quinn_interpolation | BUILD_VERIFIED | `CLEAN_REIMPLEMENTED / BUILD_VERIFIED`；锁定 Quinn Second |
| P0/P1 | macleod_interpolation | BUILD_VERIFIED | `CLEAN_REIMPLEMENTED / BUILD_VERIFIED`；新 API |
| P1 | coherent_sampling | BUILD_VERIFIED | `CLEAN_REIMPLEMENTED / BUILD_VERIFIED`；新 API |
| P1 | frequency_response_correction | BUILD_VERIFIED | `CLEAN_REIMPLEMENTED / BUILD_VERIFIED`；新 API |
| P2 | czt | BUILD_VERIFIED | `CLEAN_REIMPLEMENTED / BUILD_VERIFIED`；unit-circle direct 基线 |
| P2 | dc_measure | BUILD_VERIFIED | `CLEAN_REIMPLEMENTED / BUILD_VERIFIED`；复用 Mean |
| P2 | fft_peak | BUILD_VERIFIED | `CLEAN_REIMPLEMENTED / BUILD_VERIFIED`；复用 PeakDetect |

历史 `.o` 只允许确认符号名称；禁止反编译后冒充源码。旧哈希、API 名、Recipe、Card 和构建记录只用于恢复规格，不足以证明新实现的算法正确性。

当前统计：`SOURCE_LOST=9`，其中 `CLEAN_REIMPLEMENTED=9`，`REIMPLEMENTATION_REQUIRED=0`，`RESTORED=0`。
