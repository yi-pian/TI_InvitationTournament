# 外设构建矩阵

验证日期：2026-08-07。环境见 `TOOLCHAIN_ENVIRONMENT.md`。`BOARD NOT_RUN` 表示本轮工程收口没有重新烧写，不是失败。

## 正式库

| Target | Sources | SysConfig | TI Arm Clang 5.1.1 `-Wall -Werror` | Link | Board |
|---|---:|---:|---:|---:|---:|
| peripheral aggregate | 40/40 | P01 baseline PASS | PASS | PASS | NOT_RUN |
| peripheral_system_template | 2 template sources | P01 baseline PASS | PASS | PASS | NOT_RUN |

聚合链接证明所有正式 `.c` 可在同一目标构建中共存。链接器会移除未引用 section，因此最终 map 不能当作“40 个模块全部运行时 Flash 大小”。该 smoke image 使用 FLASH 0x758、SRAM 0x201（含 0x200 stack），只作为链接证据。

## 集成 profile

| Profile | SysConfig 1.28 | Generated C compile | Link | Board | 工具 info |
|---|---:|---:|---:|---:|---|
| P01 ADC_CAPTURE | PASS | PASS | PASS | NOT_RUN | ADC wakeup 通用提示；DMA0 Full |
| P02 DUAL_ADC | PASS | PASS | PASS | NOT_RUN | 两个 ADC wakeup 提示；DMA0/1 Full |
| P03 DAC_GENERATOR | PASS | PASS | PASS | NOT_RUN | TIMG6 retention；DMA1 Full |
| P04 ADC_DAC | PASS | PASS | PASS | NOT_RUN | ADC wakeup；TIMG6 retention；DMA0/1 Full |
| P05 FREQUENCY | PASS | PASS | PASS | NOT_RUN | TIMG6 retention |
| P06 FULL_SIGNAL | PASS | PASS | PASS | NOT_RUN | ADC wakeup；TIMG6/7 retention；DMA0/1/2 Full |

## 典型组合覆盖

| 典型组合 | 对应证据 | SysConfig | Compile/Link | 状态 |
|---|---|---:|---:|---|
| ADC only | bsp/adc callback abstraction | N/A | PASS | BUILD |
| ADC + DMA | P01 + adc_dma | PASS | PASS | BUILD；adc_dma 另有 BOARD 证据 |
| Dual ADC | P02 | PASS | PASS | BUILD，hardware adapter pending |
| DAC | bsp/dac + dac_dc | N/A | PASS | BUILD |
| DAC + DMA | P03 | PASS | PASS | BUILD，hardware adapter pending |
| Timer Capture | timer_capture data module | N/A | PASS | BUILD |
| Comparator + Capture | P05 | PASS | PASS | BUILD，ISR adapter pending |
| ADC + DAC | P04 | PASS | PASS | BUILD |
| ADC + UART | P01 | PASS | PASS | BUILD |
| ADC + DAC + UART | P04 | PASS | PASS | BUILD |
| Full Profile | P06 | PASS | PASS | BUILD |

## ADC_DMA Demo

| Demo | SysConfig | Actual main compile | ADC_DMA source compile | Link | Board evidence |
|---|---:|---:|---:|---:|---|
| adc_dma_demo | PASS | PASS | PASS | PASS | PA25 dynamic input NOT_RUN |
| adc_buffer_uart_dump | PASS | PASS | PASS | PASS | UART dump not rerun this round |
| adc_dma_onboard_selftest | PASS | PASS | PASS | PASS | Previous BOARD PASS retained |

三个 `.projectspec` 还通过 `validate_projectspec_paths.ps1`：每个都链接唯一正式 `signal_adc_dma.c/.h` 和 common header，compiler include 指向真实物理目录，未使用 `${PROJECT_ROOT}/modules/*`。

此外，已导入的 selftest CCS 工程实际执行 Clean + Rebuild 成功。Console 明确显示：

- `D:/TI/CCS/.../ti-cgt-armllvm_5.1.1.LTS/bin/tiarmclang.exe`
- `-Wall -Werror`
- 真实 include path：`02_acquisition/adc_dma` 与 `01_bsp/common`
- 完整 `.out` 链接成功

## 已知非错误提示

- selftest 启用 LFXT 后 LFOSC 不能在不 BOOTRST 的情况下重新启用。
- ADC 自动 power-down 的通用 wakeup 信息。
- TIMG6/TIMG7 STOP/STANDBY retention 信息。

这些是 SysConfig `info`，不是编译 warning；但涉及低功耗或极限采样率时必须转为设计检查项。

## 复现命令

```powershell
powershell -NoProfile -ExecutionPolicy Bypass -File .\tools\validate_peripheral_library.ps1
powershell -NoProfile -ExecutionPolicy Bypass -File .\tools\validate_peripheral_profiles.ps1
powershell -NoProfile -ExecutionPolicy Bypass -File .\tools\validate_existing_adc_demos.ps1
powershell -NoProfile -ExecutionPolicy Bypass -File .\tools\validate_projectspec_paths.ps1
```

生成结果分别写入 `10_tests/peripheral_library/build`、`10_tests/peripheral_profiles/build`、`10_tests/existing_adc_demos/build`。
