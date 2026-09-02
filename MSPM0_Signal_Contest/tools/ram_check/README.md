# RAM Check

用显式 phase 生命周期计算：

```text
最大同时存活 Buffer + globals + stack reservation + library workspace
```

并可从 TI Arm Clang `.map` 的 `MEMORY CONFIGURATION / SRAM` 行读取真实链接容量与 used/unused：

```powershell
python .\tools\ram_check\ram_check.py `
  .\tools\ram_check\spectrum_example.json `
  --map .\10_tests\tft_ili9341\build\tft_ili9341_example.map
```

示例 manifest 与 TFT map 不是同一工程，只用于分别演示两个输入能力，不能据此得出应用内存结论。对已有完整 build，优先采用 map 的链接 SRAM used；生命周期表用于检查同时存活与复用计划。Map 中预留的 `.stack` 仍不是运行时栈高水位证明，实板需额外测量。

