# Dual ADC Platform Adapter Module Card

- 正式源码：`../signal_dual_adc_platform.c/.h`
- 功能：共享 Timer 触发两路 ADC+DMA，输出两块独立 `uint16_t raw[N]`。
- 调用：`SYSCFG_DL_init -> Init -> Start -> IsFinished -> GetConfiguredRate/Stop`。
- RAM：调用者两块 raw，共 `4N` bytes。
- 硬件：2 ADC、2 DMA、1 Timer、Event、2 IRQ、2 analog pins。
- SysConfig：参考 P02 或 P06。
- 典型下游：两次 ADC To Voltage，再接 FFT Phase/Correlation。
- 详细说明：[README.md](README.md)
