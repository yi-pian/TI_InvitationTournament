# 步骤03：双 ADC 同步采集

`main.c` 的 `App_Capture()` 对应 README 的最小流程：调用 `SignalDualADC_Start(g_in_raw,g_out_raw,SIGNAL_SAMPLE_COUNT)`，循环等待 `SignalDualADC_IsFinished()`，再进入算法。`g_in_raw` 是未知网络输入，`g_out_raw` 是未知网络输出；换算电压的 for 循环是本题新增的单位转换逻辑。

双 ADC 的完成状态来自 `DMA_IRQHandler()`，而不是 ADC 外设中断。经上板验证，旧 example05 副本只打开了 `DMA_INT_IRQn` 的 NVIC，漏开了 DMA_CH0、DMA_CH2 的完成中断位；集成库和 22_X 的 `signal_dual_adc_mspm0g3507.c` 已在 `SignalDualADC_Init()` 内调用 `DL_DMA_enableInterrupt()` 打开两路完成中断，example07 已同步为该集成库版本。否则 `App_Capture()` 会永远等待：按 `1` 停在第一个扫频点 500 Hz，按 `3` 停在第一个谐波点 1 kHz。该修复不改变 SysConfig 的 DMA 通道分配，main 只需正常调用 `SignalDualADC_Init()`。

先单独 Build 这一阶段，示波器确认两路采样触发同步，再继续显示和测量。
