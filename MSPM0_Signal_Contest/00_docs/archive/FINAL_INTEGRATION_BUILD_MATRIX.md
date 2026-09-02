# Final Integration Build Matrix

工具链：TI Arm Clang 5.1.1.LTS；SDK `C:\TI\mspm0_sdk_2_11_00_07`；SysConfig 1.28.0。每个 PASS 均执行了 SysConfig generate、全部 translation units compile、final application link，并生成 `.out` 与 `.map`。资源为 Flash / SRAM（含 512 B 保留栈）。

| Application | SysConfig | Compile | Link | PC Truth | Flash | SRAM | Backend | Status | Board |
|---|---|---|---|---|---:|---:|---|---|---|
| Signal Meter | PASS | PASS | PASS | PASS（meter/time chain） | 7,656 | 14,926 | Math Reference | BUILD_VERIFIED | PENDING_BOARD |
| Frequency A | PASS | PASS | PASS | PASS（capture math） | 1,944 | 757 | no FFT | BUILD_VERIFIED | PENDING_BOARD |
| Frequency B | PASS | PASS | PASS | PASS（zero-cross chain） | 6,264 | 14,896 | Math Reference | BUILD_VERIFIED | PENDING_BOARD |
| Frequency C | PASS | PASS | PASS | PASS（Q31 integration） | 89,368 | 16,936 | CMSIS Q31 | BUILD_VERIFIED | PENDING_BOARD |
| Spectrum Analyzer | PASS | PASS | PASS | PASS（Q31 integration） | 89,320 | 17,045 | CMSIS Q31 | BUILD_VERIFIED | PENDING_BOARD |
| THD Analyzer | PASS | PASS | PASS | PASS（Q31 integration） | 90,776 | 16,961 | CMSIS Q31 | BUILD_VERIFIED | PENDING_BOARD |
| Phase Meter | PASS | PASS | PASS | PASS（Q31 integration） | 89,168 | 15,392 | CMSIS Q31，N=512 | BUILD_VERIFIED | PENDING_BOARD |
| DDS Generator | PASS | PASS | PASS | PASS（DDS/table math） | 10,560 | 3,244 | Math Reference | BUILD_VERIFIED | PENDING_BOARD |
| Sweep Analyzer | PASS | PASS | PASS | PASS（gain/phase glue） | 18,352 | 9,687 | FFT none / Math Reference | BUILD_VERIFIED | PENDING_BOARD |
| Wave Capture Replay | PASS | PASS | PASS | PASS（auto-range/resample glue） | 7,456 | 18,173 | no FFT | BUILD_VERIFIED | PENDING_BOARD |
| Signal Analyzer | PASS | PASS | PASS | PASS（formal algorithms；5/5 Profile link） | 91,032* | 9,999* | CMSIS Q31 | BUILD_VERIFIED | PENDING_BOARD |
| Contest Template | PASS | PASS | PASS | PASS（formal algorithms；4/4 Profile link） | 8,880* | 9,505* | CMSIS Q31 configured | BUILD_VERIFIED | PENDING_BOARD |

`*` 主表列默认 Profile：Signal Analyzer=Spectrum，Contest Template=Basic。所有 Profile 的资源见 `SYSTEM_RESOURCE_MAP.md`。

## Build Evidence

- Pre-backend 8 targets：`10_tests/integration/round1_build_closure/`
- Q31 system regression：`10_tests/integration/round1_backend_q31/`
- Q31 N=512/1024：`round1_backend_q31_n512/`, `round1_backend_q31_n1024/`
- Final 4 targets：`10_tests/integration/final_integration/`
- Analyzer Profile 1~5：`final_profile_signal_analyzer_1/` … `_5/`
- Template Profile 1~4：`final_profile_contest_template_1/` … `_4/`
- Source manifest 与 JSON/CSV 资源结果随每个目录保存。

## Negative Result（保留真实失败）

| Target | Compile | Link | 原因 | 状态 |
|---|---|---|---|---|
| Frequency C Q31 N=2048 | PASS | FAIL | `.bss=0x8010`，超过 32 KiB SRAM；无 `.out` | UNSUPPORTED_CURRENT_SIMPLE_PIPELINE |
| Simple FFT N=4096 | NOT_RUN_AS_PASS | NOT_RUN | float input + complex spectrum 已超 SRAM 模型 | `UNSUPPORTED_BY_CURRENT_SIMPLE_FFT_API` |

状态定义固定为 `DRAFT / BUILD_VERIFIED / PC_VERIFIED / BOARD_VERIFIED / CONTEST_VERIFIED`。本轮没有开发板运行数据，因此没有任何应用被提升为 BOARD/CONTEST_VERIFIED。
