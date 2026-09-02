# Portable Application Guide：比赛母版可搬运用法

## 推荐结论

新赛题优先复制 `08_applications/signal_contest_template/`，而不是复制旧的 linked-source Application。

复制后的母版可以放在任意可读写目录；它不需要保持到两个正式仓库的相对位置，也不需要设置 `MSPM0_SIGNAL_LIBRARY_ROOT`。正常情况下不需要手工补 Include Path。

## 正确步骤

1. 复制整个 `signal_contest_template/` 到你的赛题目录。
2. 在 CCS 中 Import `ticlang/signal_contest_template_final_LP_MSPM0G3507_nortos_ticlang.projectspec`。
3. 按模块 README 修改本副本的 `.syscfg`。
4. 把 README 明列的正式模块文件复制到本副本 `modules/`。
5. Refresh，确认 `.c` 参与 Build。
6. 每加一个模块 Build 一次。

母版 projectspec 使用：

- `${PROJECT_ROOT}` 定位母版自己的文件；
- `${PROJECT_ROOT}/modules` 定位冻结模块副本；
- `${COM_TI_MSPM0_SDK_INSTALL_DIR}` 定位 TI SDK；
- SysConfig 产品机制定位生成工具。

因此新赛题工程不再依赖仓库根变量、`../../../../` 或正式库 linked source。

## 需要保持什么目录关系

只要保留母版内部结构：

```text
your_problem/
├─ main.c
├─ signal_config.h
├─ signal_contest_template.syscfg
├─ modules/
├─ COPIED_MODULES.md
└─ ticlang/*.projectspec
```

正式模块只有 `MSPM0_Signal_Contest` 一个根。比赛前由 Canonical Registry 生成所需模块的复制清单；可搬运工程不需要维护第二个算法根。

## 旧 Application 怎么办

已有 `signal_meter`、`spectrum_analyzer`、`harmonic_thd_analyzer` 等仍是集成参考和历史 build baseline。它们可能继续使用 workspace 根变量与 linked source；不要把这套维护方式当作新比赛工程默认流程。

如果你只是复现/维护旧 Application，可按其 projectspec 的旧 portable policy 设置一次 `MSPM0_SIGNAL_LIBRARY_ROOT`。如果你要新建赛题工程，直接用新母版，不必设置该变量。

## 常见错误

- `signal_xxx.h not found`：确认文件已复制到 `modules/`、工程已 Refresh、`${PROJECT_ROOT}/modules` 仍在 Include Path。
- 新 `.c` 没编译：检查 Exclude from Build。
- `SIGNAL_ADC_*` 宏不存在：SysConfig 实例名没有按模块 README 命名。
- SDK 头文件找不到：CCS 工程未正确识别 MSPM0 SDK product；这不是模块仓库路径问题。
- 从文件管理器只复制了 `main.c`：必须复制整个母版目录和 projectspec/syscfg。

真实模块搬运构建证据见 `COPY_ASSEMBLY_READINESS.md`。
