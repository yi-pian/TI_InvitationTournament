# TI Arm Clang aggregate build check

这个测试用 CCS 当前实际的 TI Arm Clang 5.1.1.LTS 动态发现 `01_bsp`～`07_signal_frontend` 的 `signal_*.c`，将所有与具体 SysConfig 实例名无关的源码和回归主程序编译为 Cortex-M0+ 对象，再使用 `adc_dma_onboard_selftest` 的 device/linker 配置完成整库链接。文件名为 `*_mspm0g3507.c` 的硬件绑定源码必须使用各自匹配的 SysConfig Profile，因此本聚合镜像明确列出并排除它们；这些文件由 `platform_closure` 与 `copy_assembly` 的独立 full-link 覆盖。模块数量不在文档中硬编码，以脚本本次输出为准。

先在 CCS 中 Clean/Rebuild `adc_dma_onboard_selftest`，或在其 `Debug` 目录运行 gmake，再执行：

```powershell
Set-ExecutionPolicy -Scope Process Bypass
.\10_tests\ticlang\validate_link.ps1
```

通过标准是脚本退出码为 0 且输出 `library_sources=<动态数量> aggregate_sources=<动态数量> profile_specific_excluded=<动态数量> ... linked=1`。排除数不是漏测数，必须与独立 Profile 链接矩阵一起看。产物只是构建检查镜像，不是要下载的比赛应用。链接器可能报默认 `.sysmem` 0x800 的提示；正式代码不使用动态分配，比赛应用的 RAM/Flash 仍以其自身 `.map` 为准。

2026-08-07 整库检查镜像的 map 为 Flash `0x7988` (31,112 B)、SRAM `0x47A2` (18,338 B)。这些数字包含回归测试的大数组、stdio 和默认 heap/stack，只证明整库能在目标链接器下成像，不是任一单模块的 Flash/RAM 占用。
