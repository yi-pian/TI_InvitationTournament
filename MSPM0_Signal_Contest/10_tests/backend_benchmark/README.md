# Backend benchmark

这是 TEST ONLY 工程，用于比较 Reference C、CMSIS-DSP、IQMath RTS 和 IQMath
MathACL。它不是应用 recipe，也不会让正式算法依赖 SysTick、CCS 或 UART。

## 已生成结果

- `backend_benchmark_host_results.csv`：PC 真实运行的误差结果。
- `build_target/fft_target_build_matrix.csv`：TI Clang 目标 Flash/SRAM 和可链接性。
- `build_target/target_backend_build_results.json`：RTS/MathACL 完整工程构建结果。
- cycle 字段在板上运行前一律是 `PENDING_BOARD`。

## PC 数值测试

```powershell
cmake -S . -B build -G Ninja -DMSPM0_SDK_ROOT=C:/TI/mspm0_sdk_2_11_00_07
cmake --build build
./build/backend_benchmark_host.exe
```

该程序直接编译 SDK 内 CMSIS-DSP sources，并与现有 Reference FFT/IFFT 对照。

## 自动目标构建矩阵

从仓库根目录运行：

```powershell
powershell.exe -NoProfile -ExecutionPolicy Bypass -File 10_tests/backend_benchmark/build_target.ps1
```

脚本要求当前机器具有：

- `C:\TI\mspm0_sdk_2_11_00_07`
- `D:\TI\CCS\ccs\tools\compiler\ti-cgt-armllvm_5.1.1.LTS`
- CCS 自带 SysConfig 1.28.0

## CCS 实板 cycle benchmark

导入以下两个项目，必须分别运行，不能只运行其中一个就替另一个标 VERIFIED：

- `ticlang/backend_benchmark_rts_LP_MSPM0G3507_nortos_ticlang.projectspec`
- `ticlang/backend_benchmark_mathacl_LP_MSPM0G3507_nortos_ticlang.projectspec`

若工作区已经有同名项目，先从工作区移除（不要勾选删除磁盘内容），再从新的
`.projectspec` 重新导入。随后执行 SysConfig generation、Project → Clean、
Project → Rebuild、Debug。

程序在测试结束后停在 `__BKPT(0)`。在 CCS Expressions/Watch 中观察：

| 变量 | 期望/含义 |
|---|---|
| `g_benchmark_cpu_hz` | 80000000 |
| `g_benchmark_fft_size` | 当前 512/1024/2048/4096 |
| `g_benchmark_fft_backend` | `benchmark_config.h` 中的后端编号 |
| `g_benchmark_complete` | `true` |
| `g_cycles_valid` | `true`，仅表示本次板上 SysTick 计数已写入 |
| `g_fft_pass` | `true` |
| `g_iq_pass` | `true`；RTS 和 MathACL 都要分别成立 |
| `g_fft_cycles` | 本次 FFT 的 CPU cycles |
| `g_magnitude_cycles` | 当前后端支持时的 magnitude cycles |
| `g_rms_cycles` | CMSIS Q15 RMS cycles |
| `g_iq_*_cycles` | IQMath 标量各操作 cycles |
| `g_iq_*_abs_error` | Q24 raw LSB 的绝对误差 |
| `g_last_result` | 最后一次 wrapper 结果 |

换点数只修改 `SIGNAL_BENCHMARK_FFT_SIZE`；换 FFT 后端只修改
`SIGNAL_BENCHMARK_FFT_BACKEND`。合法点数为 512/1024/2048/4096。4096 点只应
尝试 Q15；其他 4096 点方案已经由目标链接器证明 SRAM 不可行。

## 判定规则

- 编译链接 PASS 不等于板上 runtime PASS。
- RTS runtime PASS 不代表 MathACL runtime PASS，反之亦然。
- 周期必须来自 80 MHz 板上本程序的 `g_*_cycles`；不得填写 PC 时间或估算值。
- 修改编译优化、FFT 点数或固定表链接策略后，旧 cycle 不再可直接比较。
