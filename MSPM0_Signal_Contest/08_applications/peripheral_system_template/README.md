# Peripheral System Template

这是比赛 application 骨架，不是新外设模块，也不包含算法实现。

> **LEGACY CALLBACK REFERENCE**：本模板保留早期 callback 结构，不再作为 MSPM0G3507 比赛新工程默认入口。简单硬件动作应直接 SysConfig + DriverLib；混合工程优先参考 `signal_contest_template`。

## 本 Application 的实现层级

| 功能 | 类型 | 当前实现 |
|---|---|---|
| 简单硬件动作 | A Direct DriverLib | 新工程应直接写在薄 Application 层，本模板 callback 不是必需层 |
| 复杂采集/输出 | B Complex Hardware Module | 由使用者替换 hook；模板自身未提供正式实现 |
| 信号计算 | C Algorithm Module | 由使用者链接正式算法；模板自身不含算法 |
| 骨架 | E Application Reference | 仅维护旧 callback 工程时参考 |

## 文件职责

- `signal_hw_config.h`：集中选择 P01..P06、N、Fs、Vref、UART baud。
- `peripheral_system_template.h/.c`：用 callback 连接采集、算法、输出，管理 frame 计数和错误。
- `main_template.c`：只保留 hardware、algorithm hook、output/debug 三个替换点。

## 5 步使用

1. 在 CCS 通过 File → Import Projects 导入 `ticlang/peripheral_system_template_LP_MSPM0G3507_nortos_ticlang.projectspec`；它默认创建 P01 application。
2. 在 `signal_hw_config.h` 选择 profile；若不是 P01，删除工程内默认 `profile.syscfg`，把对应 `09_examples/integration_profiles/.../profile.syscfg` 复制进工程并保持此文件名。
3. 这是 Legacy 维护流程：将需要的正式模块以 linked source 接入，并让 include search path 指向真实目录。新赛题请改用 `signal_contest_template` 的冻结复制流程。
4. 在 `App_Acquire` 中接 hardware adapter：初始化、Start、等待完成、构造 `signal_u16_frame_t`。
5. 把算法任务提供的入口放进 `App_AlgorithmHook`；输出放 `App_Output`。

`main_template.c` 默认 `App_Acquire` 返回 `NOT_SUPPORTED`，因此可编译但不会伪装成可运行应用。接入硬件时再加入 `ti_msp_dl_config.h` 和 `SYSCFG_DL_init()`；`peripheral_system_template.c` 本身保持与 DriverLib 解耦。

`.projectspec` 只复制 application 自己拥有的 skeleton 和 `.syscfg`；本 Legacy 模板通过 linked resource 指向正式 common。这个约束只用于维护该模板，不是新比赛工程策略；新赛题使用 `signal_contest_template` 并按模块 README 冻结复制。

## 组合规则

- output callback 可以为 NULL；采集和算法 hook 必须存在。
- frame 在一次 `RunOnce` 内只读有效。需要跨帧保存时，算法使用自己的静态 workspace。
- 不在 callback 内动态分配内存。
- 不在 DMA/ADC ISR 中直接调用 `RunOnce` 或算法。
- 双 ADC、Timer Capture、纯 DAC 应建立同样薄的应用 adapter，必要时扩展 application 自己的 context；不要把 profile 分支塞回正式模块。

## 编译 include

模板需要：

```text
01_bsp/common
08_applications/peripheral_system_template
```

硬件 Adapter 再按所选模块增加真实目录。完整接口边界见 `00_docs/MODULE_INTERFACE_MATRIX.md`。
