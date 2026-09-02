# Integration tests

`test_integration_round1.c` 使用合成 ADC 数据验证第一轮 Glue：Signal Meter、Spectrum、
THD、FFT Phase 与 Correlation Phase。它不连接开发板。

```powershell
powershell -NoProfile -ExecutionPolicy Bypass -File .\tools\validate_integration_round1.ps1
```

脚本还用 TI Arm Clang `-Wall -Werror -fsyntax-only` 检查第一轮 8 个应用配置和 3 个
共享源。源码检查不是完整 compile/link，更不是 board test。
