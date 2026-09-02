# Memory Guide

## 先算 Buffer

| Buffer | 预算 |
|---|---:|
| ADC raw | `2N` B |
| one float signal | `4N` B |
| one float complex FFT | `8N` B |
| one-sided magnitude | `4 × (N/2+1)` B，约 `2N` B |
| dual raw | `4N` B |
| dual float signal | `8N` B |
| dual complex FFT | `16N` B |
| Ring 保存 N 点 | `2 × (N+1)` B |

单通道 Simple FFT 常见基础总量约 `16N` B，尚未包括 events、correlation、DriverLib、result、UART 和 stack。不要只计算 FFT 函数内部。

## 当前已验证边界

| 配置 | 真实结果 |
|---|---|
| Q31 FFT N=512，Frequency/Spectrum/THD | 约 8.7~8.9 KiB SRAM，PASS |
| Q31 FFT N=1024，Frequency/Spectrum/THD | 约 16.9~17.0 KiB SRAM，PASS |
| Phase N=512 | 15,392 B SRAM，PASS |
| Phase N=1024 | 29,728 B SRAM，只余 3,040 B，谨慎 |
| Frequency C N=2048 | compile PASS，link FAIL；`.bss=0x8010` 超 32 KiB |
| Simple FFT N=4096 | `UNSUPPORTED_BY_CURRENT_SIMPLE_FFT_API` |

默认：单通道 FFT N=1024；双通道 Phase N=512；FFT Backend Q31。

## 怎样看 `.map`

1. Build 必须产生 `<target>.out` 和 `<target>.map`。
2. 在 map 的 `MEMORY CONFIGURATION` 查 `FLASH` 与 `SRAM` 的 used/unused。
3. 查 `.stack` 保留量；当前 512 B 只是 linker reservation，不是实测 high-water。
4. 搜索 `.bss.<symbol>` / `.data.<symbol>`，按 size 找大 Buffer。
5. 用 TI `tiarmsize <target>.out` 交叉检查 Flash/SRAM；项目构建脚本会自动做一致性校验。
6. 改 N、Profile、Backend、UART/UI Buffer 后必须重新 full link；不要沿用旧 map 数字。

构建证据保存在：

- `10_tests/integration/round1_build_closure/`
- `10_tests/integration/round1_backend_q31/`
- `10_tests/integration/final_integration/`
- `10_tests/integration/final_profile_signal_analyzer_*/`
- `10_tests/integration/final_profile_contest_template_*/`

历史资源表已移到 `00_docs/archive/`，日常判断以当前 target 的新 `.map` 为准。

## RAM 不够时的处理顺序

1. 关闭未使用的 Profile/feature，让预处理器不编译无关 workspace。
2. 降低 N，重新核对分辨率和记录时间。
3. 复用生命周期不重叠的应用 workspace，但不要破坏模块输入输出契约。
4. 减少 peak/event/lag/output table 容量。
5. 若仍不够，回正式算法库评估 Native Q15 pipeline；不要在应用层写第二套 FFT。
