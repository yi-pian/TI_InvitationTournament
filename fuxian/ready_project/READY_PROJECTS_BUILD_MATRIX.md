# READY_PROJECTS BUILD MATRIX

审计日期：2026-08-23  
状态：`BUILD_PASS / BOARD_NOT_RUN`

## 范围与授权

八个工程均从同一个空母版 `template_original/signal_contest_template_final` 独立复制建立，没有从另一个 ready project 派生。

本轮按用户追加授权，从已验证的 `fuxian/example01`～`example04` 复制各工程所需的 `modules/*` 与 `signal_contest_template.syscfg`。模块允许复制但禁止在目标工程内改写；`.syscfg` 允许按各仪器的外设需求调整。最终允许存在差异的文件只有：

- `main.c`：ready project 应用代码；
- `modules/*`：已验证示例的逐文件原样副本，禁止编辑算法或接口；
- `signal_contest_template.syscfg`：允许复制并按需调整；当前八个工程无需额外调整，实际仍为已验证示例的原样副本；
- 各工程的 `READY_PROJECT_GUIDE.md`：用户明确要求新增的工程作用、使用方法及跨工程复用说明，不参与固件构建；
- 本目录中的两份最终报告。

所有工程的工程配置、`signal_config.h`、README、`targetConfigs/*` 和 `Debug/*` 均与空母版逐文件同哈希。为遵守 `Debug/*` 不得修改的要求，Generate/Clean/Compile/Link 在 `tmp/ready_projects_build` 的隔离副本中完成。

## 构建环境与判定

- Device：MSPM0G3507，LQFP-64；
- SDK：MSPM0 SDK 2.11.0.07；
- SysConfig：1.28.0；
- Compiler：TI Arm Clang 5.1.1 LTS；
- Flags：`-O2 -std=c11 -Wall -Werror`；
- Build：每个工程均执行 `gmake clean all`；
- Generate：隔离 makefile 的路径已改指向该 ready project 的隔离副本，实际读取对应副本中的 `.syscfg`；
- Static SysConfig audit：八个工程运行 `check_syscfg.py --json`，均为 PASS、0 warning；
- Warnings：编译阶段 0；`-Werror` 下任一编译 warning 会使该工程失败；
- Flash/Board：未连接实板，统一标记 `NOT_RUN`。

## 构建与内存矩阵

MSPM0G3507 可用 Flash 131072 B、SRAM 32768 B。下表占用来自最终链接生成的 `.map`，SRAM 数字包含链接脚本预留栈。

| 工程 | Generate | Clean | Compile | Link | Warnings | Flash 使用 | SRAM 使用 | 烧录 | Board |
|---|---:|---:|---:|---:|---:|---:|---:|---:|---:|
| 01_dual_waveform_scope | PASS | PASS | PASS | PASS | 0 | 28232 B (21.54%) | 10950 B (33.42%) | NOT_RUN | NOT_RUN |
| 02_dual_spectrum_thd | PASS | PASS | PASS | PASS | 0 | 47992 B (36.61%) | 18550 B (56.61%) | NOT_RUN | NOT_RUN |
| 03_programmable_signal_generator | PASS | PASS | PASS | PASS | 0 | 34960 B (26.67%) | 3707 B (11.31%) | NOT_RUN | NOT_RUN |
| 04_dual_measurement_meter | PASS | PASS | PASS | PASS | 0 | 30616 B (23.36%) | 11700 B (35.71%) | NOT_RUN | NOT_RUN |
| 05_bode_sweep_analyzer | PASS | PASS | PASS | PASS | 0 | 42592 B (32.50%) | 8026 B (24.49%) | NOT_RUN | NOT_RUN |
| 06_trigger_burst_capture | PASS | PASS | PASS | PASS | 0 | 28088 B (21.43%) | 4639 B (14.16%) | NOT_RUN | NOT_RUN |
| 07_digital_filter_lab | PASS | PASS | PASS | PASS | 0 | 47512 B (36.25%) | 16430 B (50.14%) | NOT_RUN | NOT_RUN |
| 08_precision_single_tone_meter | PASS | PASS | PASS | PASS | 0 | 50368 B (38.43%) | 13401 B (40.90%) | NOT_RUN | NOT_RUN |

八个工程均未超出 Flash 或 SRAM。大数组全部是文件作用域 `static` 对象，没有把双通道、FFT、扫频或触发缓冲放到函数栈上。

## 局部刷新与按键改造验收

- 八个工程进入稳定运行后的周期性刷新均不再执行 `TFT_ST7789_FillScreen()`；
- 整屏清除只保留在上电首次绘制或实际翻页的静态 UI 分支；
- 波形、频谱、Bode 曲线和触发捕获只清除各自图框内部，再绘制新 trace；
- 数值变化前只用 `TFT_ST7789_FillRect()` 清除对应字符区域，避免旧位数残留；
- 按键检测复用 `moni01` 方法：1 ms SysTick、每 5 ms 扫描、ISR 写入 8 项环形队列、主循环逐项消费；
- ISR 不调用 TFT、ADC、DDS、扫频或测量算法；队列满时保留已排队事件并丢弃最新事件。

## 文件边界审计

| 工程 | 模块来源 | 模块代码文件 | module 与来源同哈希 | `.syscfg` 与来源同哈希 | 已构建 main 同哈希 | 其余受保护文件 | `Debug/*` |
|---|---|---:|---:|---:|---:|---:|---:|
| 01 | example03 | 24 | YES | YES | YES | 母版同哈希 | 母版同哈希 |
| 02 | example04 | 93 | YES | YES | YES | 母版同哈希 | 母版同哈希 |
| 03 | example04 | 93 | YES | YES | YES | 母版同哈希 | 母版同哈希 |
| 04 | example01 | 20 | YES | YES | YES | 母版同哈希 | 母版同哈希 |
| 05 | example02 | 41 | YES | YES | YES | 母版同哈希 | 母版同哈希 |
| 06 | example04 | 93 | YES | YES | YES | 母版同哈希 | 母版同哈希 |
| 07 | example04 | 93 | YES | YES | YES | 母版同哈希 | 母版同哈希 |
| 08 | example04 | 93 | YES | YES | YES | 母版同哈希 | 母版同哈希 |

这里的“模块代码文件”排除了 README；各工程只保留母版原有的 README，复制闭包中附带的说明 README 已移除。

最终审计结论：

- Existing fuyong/example source modified：`NO`；
- Existing module source edited：`NO`，目标目录内是授权的原样副本；
- SysConfig modification permission：`ALLOWED`；当前实际 hand-edited：`NO`，因为授权副本已满足构建与外设闭包；
- `signal_config.h` modified：`NO`；
- Project configuration modified：`NO`；
- README modified：`NO`；
- Final project `Debug/*` modified：`NO`；
- Unauthorized file mismatch：`0`。

## 烧录说明

隔离构建生成的可烧录文件位于：

`tmp/ready_projects_build/<工程名>/Debug/signal_contest_template_final.out`

这些 `.out` 已由对应交付版 `main.c/modules/.syscfg` 构建，可用于后续实板烧录。由于本轮没有探针和实板，报告只确认 `BUILD_PASS`，不宣称功能已实板验证。交付工程自身保留了母版 `Debug/*`；若从 CCS 工程目录烧录，应先在 CCS 中执行一次 Clean/Build，让 CCS 在本地生成同一版本的 Debug 产物。
