# Reuse Test Build Matrix

## main.c 函数 COPY 复验（2026-08-22）

三个独立工程已改为复制完整 `static` 函数，再以 SysConfig 1.28.0 → `-Wall -Werror` Compile → Link 复验：

| 复用测试工程 | Generate | Compile | Link | Flash | SRAM | FFT/frame |
|---|---|---|---:|---:|---:|---:|
| `reuse_test_01_signal_analyzer` | PASS | PASS | PASS | 45,720 B | 19,784 B | 1 |
| `reuse_test_02_dual_channel` | PASS | PASS | PASS | 24,544 B | 5,068 B | N/A |
| `reuse_test_03_ui_dds` | PASS | PASS | PASS | 31,032 B | 2,594 B | N/A |

`reuse_test_01_signal_analyzer` 只有 `RunFFTCommon()` 可调用 `arm_cfft_q15`，因此同一 DMA frame 的 FFT 为 1 次。Board：全部 `NOT_RUN`。

验收日期：2026-08-21。工具：SysConfig 1.28.0、TI Arm Clang 5.1.1.LTS、MSPM0 SDK 2.11.0.07。每项均从 `template_original/signal_contest_template_final` 的独立副本开始，按“SysConfig Generate → Clean → Compile → Link”执行；未烧录或连接实板。

| 复用测试工程 | Generate | Clean | Compile | Link | 编译告警 | Flash | SRAM | Flash/Board |
|---|---|---|---|---|---|---:|---:|---|
| `reuse_test_01_signal_analyzer` | PASS | PASS（17 个再生文件） | PASS | PASS | 0（`-Werror`） | 45,600 B | 19,784 B | NOT_RUN / NOT_RUN |
| `reuse_test_02_dual_channel` | PASS | PASS（10 个再生文件） | PASS | PASS | 0（`-Werror`） | 24,568 B | 5,068 B | NOT_RUN / NOT_RUN |
| `reuse_test_03_ui_dds` | PASS | PASS（18 个再生文件） | PASS | PASS | 0（`-Werror`） | 31,032 B | 2,594 B | NOT_RUN / NOT_RUN |

SysConfig 在三项生成时都给出 ADC 自动掉电唤醒、STOP/STANDBY 外设保持和 DMA Full Channel 的 `info` 提示；这些不是编译器 warning，也未阻止生成或链接。

RAM/Flash 数值来自各项目的最终 `Debug/*.map`，包含链接器映射的已占用段；不是实板运行测量。

静态项目检查另给出同一项 CCS 管理提示：`Debug` 下的 IDE makefile 需由 CCS/CCS Theia 首次 GUI Build 产生。本次验收未手工维护该 makefile，而是直接使用上述同版本 TI Arm Clang 完成 Compile/Link。
