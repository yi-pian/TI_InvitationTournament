# Integration Simplification Audit

审计日期：2026-08-08。审计范围：`00_docs/`、`08_applications/`、`contest_reproductions/`、构建 manifests/projectspec 及其文档引用。

## 当前规模

- `00_docs/` 根目录：48 份文件。
- `08_applications/`：16 个目录，其中 10 个目录承载 12 个核心 Integration build targets，`common/` 保存 3 组正式 Adapter/Glue；3 个目录为空。
- `contest_reproductions/`：根 README + `_TEMPLATE/` 15 份分析/预算/结果模板文件。
- `10_tests/`：完整 build/map/backend/PC regression 证据，本次不移动、不删除。

## A. 必须保留

| 内容 | 原因 | 处理 |
|---|---|---|
| `01_bsp/`, `02_acquisition/`, `06_generator/`, `07_signal_frontend/` 正式源码 | 外设唯一源码 | 原路径保留 |
| `../MSPM0_Signal_Contest/` 正式算法源码 | 算法唯一源码 | 原路径保留 |
| `08_applications/common/` | raw→V、DualADC、DAC DMA 等已验证 Adapter/Glue | 原路径保留 |
| 10 个核心 Application 目录及 projectspec | 已 BUILD_VERIFIED 的拼装参考 | 原路径保留；README 降级为 Reference Assembly Example |
| `signal_contest_template/` | 手工拼装母工程 | 原路径保留；删除题目分析导航，只保留 config/pipeline/build 使用说明 |
| `09_examples/integration_profiles/` | SysConfig profile source of truth | 原路径保留 |
| `10_tests/`、`.out/.map`、source manifest、backend benchmark | 验证证据 | 完全保留 |
| Build Matrix 与 Backend/历史报告 | 可追溯证据 | 移入 `00_docs/archive/`，不作为现场入口 |

## B. 精简或合并

| 原内容 | 重复问题 | 新的唯一权威文档 |
|---|---|---|
| `START_HERE`, `INTEGRATION_START_HERE`, `PERIPHERAL_START_HERE`, `LEARNING_ORDER` | 多个入口、包含选题逻辑 | `START_HERE.md` |
| `PERIPHERAL_MODULE_INDEX`, `INTEGRATION_MODULE_INDEX`, `MODULE_TREE`, `MODULE_STATUS_MATRIX` | 模块位置分散 | 后续由完整 `MODULE_CATALOG.md` 取代；`MODULE_INDEX.md` 仅兼容跳转 |
| 原 `MODULE_INTERFACE_MATRIX`, Hardware/Algorithm Contract、Connection/Dependency Guide | 接口与 Adapter 重复 | `MODULE_INTERFACE_MATRIX.md` |
| `SYSTEM_RECIPES`, `RECIPES`, `PERIPHERAL_RECIPES`, Connection Guide | 既讲选题又讲拼装 | `MODULE_ASSEMBLY_GUIDE.md`，只保留拼装方法和示例链 |
| `PERIPHERAL_QUICK_MODIFY`, `CONTEST_QUICK_MODIFY`, 参数计算说明 | 参数入口重复、理论过多 | `PARAMETER_MODIFY_GUIDE.md` |
| Hardware Resource Map、Reallocation、Conflict Matrix | SysConfig 操作与冲突分散 | `SYSCONFIG_MODIFY_GUIDE.md` + `RESOURCE_CONFLICT_GUIDE.md` |
| FFT/Backend/System resource budget | RAM 信息分散 | `MEMORY_GUIDE.md` |
| `KNOWN_SYSTEM_LIMITATIONS`, issues、missing capabilities | 日常限制分散 | `KNOWN_LIMITATIONS.md` |
| 多份 checklist | 使用场景不同、重复 Build 检查 | 一页 `OFFLINE_ASSEMBLY_CHECKLIST.md`；旧版归档 |

## C. 当前阶段不需要作为主入口

以下内容不会删除，只归档：

- 题目分析与自动映射：`CONTEST_PROBLEM_ANALYSIS_TEMPLATE`、`CONTEST_REQUIREMENT_TO_MODULE`、`SYSTEM_RECIPE_SELECTION`、`CONTEST_REPRODUCTION_WORKFLOW`、`CONTEST_MODULE_SELECTION`、复杂 Requirement/Recipe 选择材料。
- 完整题目模板：`contest_reproductions/_TEMPLATE/`。
- 历史集成与 Backend 证据：Round 1 checkpoint/closure、Backend migration、Final integration report、Build matrices、SDK/backend audits、resource reallocation、API hash。
- 旧教学/Recipe/Quick Modify/Module map 文档：内容被新权威入口吸收后移入 archive，供追溯。

归档位置：

- 赛题分析材料：`00_docs/archive_analysis/`
- 历史、验证与被合并文档：`00_docs/archive/`

## 不变项

- 不修改正式 `.c/.h` 模块路径。
- 不修改冻结的公开 API。
- 不修改 `.syscfg`、projectspec source/include 列表或 SDK/toolchain 路径。
- 不删除测试和构建证据。
- 精简完成后必须重跑 12 个核心 Application 的 SysConfig、compile、final link 与 PC regression。
