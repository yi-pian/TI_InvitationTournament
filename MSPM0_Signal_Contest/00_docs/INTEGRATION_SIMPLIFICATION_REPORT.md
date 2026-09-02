# Integration Library Simplification Report

> **历史记录，已被新架构取代：** 本报告记录的是早期“所有功能先选模块”的精简过程。当前唯一功能实现总入口是 [CONTEST_IMPLEMENTATION_GUIDE.md](CONTEST_IMPLEMENTATION_GUIDE.md)，先判断 Direct DriverLib / Complex Hardware Module / Algorithm / External Device / Application；本报告中的旧主流程与结论不再是现行要求。

完成日期：2026-08-08。

本轮只整理 Integration 的文档、导航和应用说明；没有新增算法、外设或赛题复现，也没有修改算法 Backend。

## 1. 文档收敛结果

- 整理前：`00_docs/` 根目录有 **48 份** Integration 文档，存在多个入口、Recipe、参数说明、资源表和历史报告。
- 离线流程补强后：比赛现场保留 **12 份操作文档**，但只有 1 条主流程。另有本次审计与本报告各 1 份，它们是过程证据，不属于现场导航。
- 历史/重复文档：**41 份**移入 `00_docs/archive/`。
- 赛题分析材料：**24 份**移入 `00_docs/archive_analysis/`，其中包含原 `contest_reproductions/_TEMPLATE/`。
- `contest_reproductions/` 现在只保留一份简短 README；它不再是 Integration 主入口。

### 当前 12 份操作文档

1. `START_HERE.md`
2. `OFFLINE_MODULE_ASSEMBLY_WORKFLOW.md`（当时的唯一主流程；现只用于 B/C 路线）
3. `OFFLINE_ASSEMBLY_CHECKLIST.md`（可选的四项现场勾选版）
4. `MODULE_CATALOG.md`
5. `MODULE_INTERFACE_MATRIX.md`
6. `MODULE_ASSEMBLY_GUIDE.md`
7. `PARAMETER_MODIFY_GUIDE.md`
8. `SYSCONFIG_MODIFY_GUIDE.md`
9. `RESOURCE_CONFLICT_GUIDE.md`
10. `MEMORY_GUIDE.md`
11. `BUILD_ERROR_GUIDE.md`
12. `KNOWN_LIMITATIONS.md`

## 2. 合并内容

| 原文档组 | 新的唯一权威入口 |
|---|---|
| `START_HERE`、`INTEGRATION_START_HERE`、`PERIPHERAL_START_HERE`、`LEARNING_ORDER` | `START_HERE.md` |
| Peripheral/Integration Module Index、Module Tree、Status Matrix | `MODULE_CATALOG.md`；`MODULE_INDEX.md` 仅保留兼容跳转 |
| 原 Interface Matrix、Hardware/Algorithm Contract、Connection/Dependency Guide | `MODULE_INTERFACE_MATRIX.md` |
| System/Peripheral/Integration Recipes 与 Connection Guide | `MODULE_ASSEMBLY_GUIDE.md` |
| Peripheral/Contest Quick Modify 与参数计算说明 | `PARAMETER_MODIFY_GUIDE.md` |
| Hardware Resource Map、Reallocation、Conflict Matrix | `SYSCONFIG_MODIFY_GUIDE.md` 与 `RESOURCE_CONFLICT_GUIDE.md` |
| FFT/Backend/System memory budget | `MEMORY_GUIDE.md` |
| Known System Limitations、Issues、Missing Capabilities | `KNOWN_LIMITATIONS.md` |
| 分散的现场检查项 | `OFFLINE_ASSEMBLY_CHECKLIST.md`；旧 checklist 已归档 |

旧文件没有永久删除；需要追溯时到 `archive/` 或 `archive_analysis/` 查找。

## 3. 归档内容

以下内容已经退出 START_HERE、主导航和现场拼装流程：

- 题目分析模板、Requirement Map、关键词到模块映射、自动 Recipe 选择、Contest Reproduction Workflow；
- 原完整 `contest_reproductions/_TEMPLATE/`；
- Round 1 checkpoint/closure、Backend migration、Final integration、旧 Build Matrix、SDK/backend audit、API hash 等历史证据；
- 已被新权威文档吸收的旧 Recipe、Quick Modify、资源表和模块图；
- Contest Template 内重复的局部 `QUICK_MODIFY.md` 与 `MEMORY_MAP.md`。

归档后，活动主文档和应用 README 对上述旧分析入口的导航引用为 **0**。

## 4. 完全保留内容

- 正式外设源码：`01_bsp/`、`02_acquisition/`、`06_generator/`、`07_signal_frontend/`；
- 正式算法唯一源码：相邻仓库 `../MSPM0_Signal_Contest/`；
- 正式集成 Adapter/Glue：`08_applications/common/`；
- 10 个核心应用目录的源码、配置、SysConfig profile 引用和 projectspec；
- `09_examples/integration_profiles/` 中的 SysConfig source of truth；
- `10_tests/` 下的 PC regression、build、map、source manifest 与 backend benchmark 证据。

`08_applications/` 的功能应用在当时均标记为 **REFERENCE ASSEMBLY EXAMPLE**。当前 `signal_contest_template` 已升级为混合实现模板：先判断 Direct/复杂模块/算法/外部驱动，再组合；仍不自动分析题目。

## 5. 源码与 API 不变性

- 正式 `.c/.h` 源码是否移动：**否**。
- 冻结公开 API 是否变化：**否**。
- `.syscfg`、projectspec source/include 列表和 SDK/toolchain 路径是否因本轮整理改变：**否**。
- 是否新增或复制 ADC、DMA、Timer、DAC、FFT、RMS、THD、Phase 等模块：**否**。
- 归档目录中的 `.c/.h/.syscfg/.projectspec` 数量：**0**。
- 应用层 CMSIS/IQMath/MATHACL intrinsic 泄漏扫描：**0 命中**；应用仍只调用正式公共 API。

## 6. 完整 Regression

环境：TI Arm Clang `5.1.1.LTS`，MSPM0 SDK `2.11.00.07`，SysConfig `1.28.0`。以下均执行了 SysConfig generate、所有 translation units compile 和 final application link，并产生 `.out` 与 `.map`。SysConfig warning 计数均为 0，`tiarmsize` 与 map 交叉检查均 PASS。

| Application target | SysConfig | Compile | Link | Flash used | SRAM used（含 stack） | Stack | SRAM remaining |
|---|---:|---:|---:|---:|---:|---:|---:|
| Signal Meter | PASS | PASS | PASS | 7,656 B | 14,926 B | 512 B | 17,842 B |
| Frequency Meter A | PASS | PASS | PASS | 1,944 B | 757 B | 512 B | 32,011 B |
| Frequency Meter B | PASS | PASS | PASS | 6,264 B | 14,896 B | 512 B | 17,872 B |
| Frequency Meter C | PASS | PASS | PASS | 16,560 B | 16,936 B | 512 B | 15,832 B |
| Spectrum Analyzer | PASS | PASS | PASS | 16,512 B | 17,045 B | 512 B | 15,723 B |
| THD Analyzer | PASS | PASS | PASS | 17,968 B | 16,961 B | 512 B | 15,807 B |
| Phase Meter | PASS | PASS | PASS | 16,424 B | 15,392 B | 512 B | 17,376 B |
| DDS Generator | PASS | PASS | PASS | 10,560 B | 3,244 B | 512 B | 29,524 B |
| Sweep Analyzer | PASS | PASS | PASS | 18,352 B | 9,687 B | 512 B | 23,081 B |
| Wave Capture Replay | PASS | PASS | PASS | 7,456 B | 18,173 B | 512 B | 14,595 B |
| Signal Analyzer（default Q31 profile） | PASS | PASS | PASS | 91,032 B | 9,999 B | 512 B | 22,769 B |
| Signal Contest Template（basic Q31 config） | PASS | PASS | PASS | 8,880 B | 9,505 B | 512 B | 23,263 B |

结果：**12/12 SysConfig PASS，12/12 Compile PASS，12/12 Link PASS**。

附加验证：

- Q31 Integration Round 1：PASS；
- TI target source compile 检查：12/12 PASS；
- PC `ctest`：1/1 PASS；
- Board test：本轮未执行，状态仍为 `PENDING_BOARD`，不写 `BOARD_VERIFIED`。

新回归证据：

- `10_tests/integration/simplification_regression_round1/`：8 组 `.out/.map`、结果 JSON、source manifest；
- `10_tests/integration/simplification_regression_final/`：4 组 `.out/.map`、结果 JSON、source manifest；
- `10_tests/integration/simplification_pc_q31/`：Q31 Integration Round 1 结果；
- `10_tests/pc/build/`：PC test executable 与 CTest 结果。

## 7. 最终比赛现场入口

本段是旧流程记录。当前入口仍是 `00_docs/START_HERE.md`，但它先进入 `CONTEST_IMPLEMENTATION_GUIDE.md` 判断五类实现层级，不再默认进入四步模块流程。

推荐查阅顺序：

1. 打开 `START_HERE.md`。
2. （旧）进入 `OFFLINE_MODULE_ASSEMBLY_WORKFLOW.md`。
3. （旧）执行四步模块流程；当前只有 B/C 路线才这样做。
4. （旧）使用 `OFFLINE_ASSEMBLY_CHECKLIST.md`；当前先完成五类实现判断。

## 8. 结论

这是当时“模块拼装工具箱”的阶段结论。现行设计进一步收敛为“功能需求 → 最简单实现层级 → 再拼装”；正式源码、公开 API、工程引用和原验证证据不因本历史报告更新而改变。

本节点停止：未分析 2024 C 题，未开始真题复现，未新增算法或外设。
