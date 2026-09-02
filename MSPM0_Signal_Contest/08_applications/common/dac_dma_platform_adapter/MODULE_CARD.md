# DAC DMA Platform Adapter Module Card

- 正式源码：`../signal_dac_dma_platform.c/.h`
- 功能：把通用 DAC DMA 回调接到 MSPM0 Timer/Event/DMA/DAC。
- 输入：`uint16_t code[count]`、更新率、repeat。
- 输出：DAC pin 波形、one-shot finished、配置更新率。
- 调用：`SYSCFG_DL_init -> Platform_Init -> DACDMA_Init/Start -> Stop`。
- 硬件：1 Timer、1 DMA、1 DAC、Event、IRQ、output pin。
- SysConfig：参考 P03/P04/P06。
- Buffer：播放期间只读且必须持续有效，RAM=`2*count` bytes。
- 详细说明：[README.md](README.md)
