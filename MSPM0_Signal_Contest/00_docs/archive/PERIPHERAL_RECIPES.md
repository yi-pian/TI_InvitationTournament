# 外设组合 Recipes

这些 recipe 只描述模块与资源怎样拼接，不实现 FFT、RMS、Vpp、频率估计等算法。

## Recipe 1：先确认 ADC_DMA 板载基线

适用：新电脑、新 CCS 导入、怀疑 DMA/Include Path 被改坏。

1. 导入 `09_examples/adc_dma_onboard_selftest/ticlang/*.projectspec`。
2. 让 SysConfig 生成，Clean，Rebuild，下载运行。
3. 在 `_BKPT(0)` 停住后看：`g_acceptance_complete=1`、`g_acceptance_pass=1`、`g_total_completed_blocks=500`、`g_sentinel_residue_count=0`、`g_module_status=MODULE_DONE`。
4. 该测试用 PB24/ADC0.5 板载 TMP6131，不是正式 PA25 动态输入。

## Recipe 2：单 ADC block → 算法 hook

选择 P01 和 `02_acquisition/adc_dma`。

```text
SYSCFG_DL_init
-> SignalADC_Init(config)
-> SignalADC_Start(buffer, N)
-> WFE until SignalADC_IsFinished
-> construct signal_u16_frame_t
-> algorithm_hook(frame)
```

算法只收到 frame。N、configured Fs、Vref、ADC bits 都由应用集中提供，算法不得 include `ti_msp_dl_config.h`。

## Recipe 3：双 ADC 同步输入

选择 P02。硬件配置提供 ADC0/ADC1 两条 DMA 路径，但当前没有一个正式模块包办两个 DMA 的 Start/ISR。

应用 adapter 应：

1. 在同一临界区装载两个 DMA destination/count；
2. 清两个 ADC/DMA flag；
3. 使能两个 ADC，再启动共同 TIMG0；
4. 两个 block 都完成后发布一对 frame；
5. 验证 `count` 和 `sample_rate_hz` 相等，再调用双通道算法 hook。

若硬件输出是交织数组，才调用 `SignalADCDualSync_Deinterleave()`；两个独立 DMA buffer 不需要先交织再拆开。

## Recipe 4：DAC 波表连续输出

选择 P03。生成器模块输出静态 `uint16_t table[]`，应用 adapter 把 table 交给 DAC DMA。

```text
wave-table generator -> uint16_t table/count
-> load DMA_CH1 source/count
-> enable DAC0 FIFO/HWTRIG0
-> start TIMG6
```

`SignalDACDMA_*` 是回调状态包装器，不会替你写 TI DMA 寄存器。波表长度、update rate 和 repeat 策略必须在应用配置中明确。

## Recipe 5：ADC + DAC 环回

选择 P04。没有杜邦线时只能验证资源共存和目标端 build。未来连接 PA15/DAC_OUT → PA25/ADC0.2 后：

1. DAC 先装表但保持 Timer 停止；
2. ADC DMA 先装 destination；
3. 启动 ADC TIMG0 与 DAC TIMG6；
4. 采满一帧后停采集，不在 ISR 打 CSV；
5. 在 CCS Graph/UART dump 比较 samples-per-cycle。

100/200/500 kSPS 的动态性能必须实板记录，不能由 `GetConfiguredTriggerRate()` 替代。

## Recipe 6：Comparator → Timer Capture

选择 P05。COMP0 edge 通过 Event4 到 TIMG6 Capture。

```text
capture ISR stores uint32_t timestamps[]
-> SignalTimerCapture_MeanPeriod(...)
-> application supplies capture_clock_hz
-> algorithm hook receives ticks + clock metadata
```

正式 ISR/DriverLib adapter 尚未建立，当前 profile 只证明资源路由能生成和链接。

## Recipe 7：全功能资源骨架

选择 P06，并从 `08_applications/peripheral_system_template` 开始。

- ADC A：DMA0
- DAC：DMA1
- ADC B：DMA2
- ADC Timer：TIMG0
- DAC Timer：TIMG6
- Capture Timer：TIMG7
- Event：1..4

先在 `signal_hw_config.h` 关闭不需要的 feature。三个 Full DMA 已全部占用，任何新增 DMA 功能都要先删或降级现有链路。

## Recipe 8：UART CSV debug

使用 `adc_buffer_uart_dump`，而不是把 UART 放进 ADC_DMA 正式模块。

1. Windows 设备管理器找到 `XDS110 Class Application/User UART` COM 口。
2. 使用 115200、8 data bits、no parity、1 stop bit、no flow control。
3. 保存终端文本为 `.csv`，确保开头是 `INDEX,ADC_RAW`。
4. 运行 `python tools/pc/plot_adc_csv.py capture.csv`。

UART 带宽远低于 100 kSPS 原始流；策略是采满一帧、停止采样、再慢速导出。

## Recipe 9：ADC PingPong → Buffer A/B

- 模块：硬件 ADC/DMA adapter + `adc_pingpong_dma`。
- Profile：单通道从 P01 开始；双通道应按 P02 另做每通道 ownership，不共用一个对象。
- 初始化：准备两个同尺寸静态 buffer → `SignalADCPingPong_Init` → 装 buffer A 到 DMA → Start。
- 完成：DMA ISR 只调用/通知 `SignalADCPingPong_OnDmaComplete` 并装下一目标；主循环 `Acquire`、处理、`Release`。
- 资源：与 P01 相同，但 RAM 为 `2*N*sizeof(uint16_t)`。
- 主要 API：`Init/OnDmaComplete/Acquire/Release`。

N=4096 时单通道 ping-pong 占 16 KiB。双通道 ping-pong N=4096 会占满 32 KiB，无法给 stack/状态留空间，不可采用。

## Recipe 10：Trigger → Ring Buffer → Capture

- 模块：连续 ADC adapter + `adc_ring_buffer` + `trigger_capture`。
- Profile：从 P01 开始；若要持续不断采样，需要把 one-block adapter 扩成 ping-pong/连续 DMA，而不是修改 trigger 算法。
- 初始化：调用者提供 ring storage → `SignalADCRing_Init` → 启动采集 → 每个完成 block 分批 `Push`。
- 触发：从可读窗口调用 `SignalTrigger_Find`；确定 index 后用 `SignalTrigger_Extract` 拷到独立 capture buffer。
- 资源：ADC0/DMA0/TIMG0/Event1；额外 RAM = ring capacity + capture capacity。
- 主要 API：Ring `Init/Push/Pop/Count/Clear`，Trigger `Find/Extract`。

该 recipe 的 trigger 是 ADC 数据阈值触发，不是 P05 的 Comparator/Timer Capture。

## Recipe 快速对照

| 数据流 | 正式模块 | Profile | 初始化顺序 |
|---|---|---|---|
| Analog→ADC→DMA→RAM | adc_dma | P01 | SysConfig→Init→Start→Wait→GetBuffer |
| Dual Analog→ADC0/1→DMA→RAM | app adapter + adc_dual_sync（按需） | P02 | 两 DMA→两 ADC→共用 Timer |
| Comparator→Timer Capture→Period | comparator frontend + timer_capture + app ISR | P05 | COMP→Capture→Event route→start capture |
| RAM→DMA→DAC | dac_wave_table/dds + dac_dma callback wrapper | P03 | table→DMA→DAC FIFO→Timer |
| ADC→RAM→DAC | adc_dma + generator/output adapter | P04 | ADC destination 与 DAC source 都装好后启动 |
| ADC PingPong→A/B | adc_pingpong_dma + hardware adapter | P01 | Init A/B→DMA A→Start→ISR swap |
| Trigger→Ring→Capture | adc_ring_buffer + trigger_capture | P01 | Ring→continuous acquire→Find→Extract |
