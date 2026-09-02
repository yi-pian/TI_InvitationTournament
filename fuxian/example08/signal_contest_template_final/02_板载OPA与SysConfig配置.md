# 步骤 2：GPAMP、两路 OPA 和 COMP0 的 SysConfig

在 CCS 双击 `signal_contest_template.syscfg`，按 `modules/README.md` 的路径添加 GPAMP、两路 OPA、COMP0。以下是已实际写入并经 SysConfig CLI 验证的字段。

| 模块 | 硬件/引脚 | 关键配置 | 输出去向 |
|---|---|---|---|
| GPAMP | `GPAMP`、PA26 | ADC Buffer profile；IN_POS、INTERNAL_OUTPUT、ADC-assisted 16 kHz | ADC1 CH14 内部 |
| OPA0 Buffer | `OPA0`、PA26 | TI Buffer profile；PSEL=IN0_POS、NSEL=RTOP、MSEL=OPEN、N1/P2、HIGH | ADC0 CH13 内部 |
| OPA1 DAC Buffer | `OPA1` | PSEL=DAC8_OUT、NSEL=RTOP、MSEL=OPEN、N1/P2、HIGH | 缓冲 COMP.DAC8，输出不出引脚 |
| COMP0 | `COMP0`、PA26 | POS、ULP、20 mV、VDDA DAC、NEG、DAC=128、双边沿 IRQ | DAC8 同时给 OPA1；实板 IRQ 不稳定，main 保持 NVIC 关闭并按同门限 ADC 帧统计 |

PA26 是 COMP0、GPAMP 和 OPA0 的高阻输入，非输出冲突；使用 `assignAllowConflicts`/`scripting.suppress` 标注共享。OPA 的 Output Pin 均 `DISABLED`，因为 ADC 读取内部模拟通道，**不接 PA22→PA25，也不接 PA26→PA24**。

保存并 Generate 后，应看到 ADC0/ADC1、COMP0、OPA0、OPA1、GPAMP 的宏；不得改生成的 `.c/.h`。
