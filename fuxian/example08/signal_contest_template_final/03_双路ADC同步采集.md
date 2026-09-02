# 步骤 3：双 ADC + DMA 同步采样

复制 `signal_dual_adc_mspm0g3507` README 的三个主调用：

```c
g_adc_status = SignalDualADC_Init(&adc_config);
g_adc_status = SignalDualADC_Start(g_raw_a, g_raw_b, SIGNAL_SAMPLE_COUNT);
while (!SignalDualADC_IsFinished()) { __WFI(); }
```

SysConfig 中 ADC0 Mem0=`CH13 (OPA0 output)`、DMA_CH0，ADC1 Mem0=`CH14 (GPAMP output)`、DMA_CH2；两个 ADC 订阅 TIMG0 的两个 publisher。Timer 周期 2 us，即 500 kS/s；每次 DMA 取得 512 个同步点。10 kHz 正弦每周期约 50 个采样点，适合用 `DrawLine` 连续显示。

`g_raw_a` 是 OPA0 单位增益缓冲通道，`g_raw_b` 是 GPAMP 单位增益缓冲通道。二者均由 PA26 驱动、同频同相，可在下一步直接验收。
