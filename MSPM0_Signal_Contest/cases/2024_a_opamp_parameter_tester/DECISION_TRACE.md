# Decision Trace

本文件严格区分两类内容：

- `ACTUAL_DECISION`：当前源码、已有教程/调试记录或本次用户声明能够证明的历史选择。
- `POST_HOC_ENGINEERING_NOTE`：从最终工程得到的复盘建议，不声称开发时真的比较过该方案。

## 真实历史决策

### D1：单帧 ADC DMA，而不是持续 Ping-Pong

`ACTUAL_DECISION`

需求：Q2 每个频点只需要采一帧，算完后再进入下一点；Q3 固定频率后只采一帧。历史对话中实际比较过 `adc_dma` 与 `adc_pingpong_dma`，最终采用前者。

选择：`SignalADC_Start(raw, 3072)` → 等待 DMA 完成 → 处理整帧。

原因：Buffer 所有权清楚，和阻塞式 `Acquire → Process → Decide` 流程一致，不需要无缝连续流、半缓冲回调和丢帧管理。

结果：最终 `.map` 中存在 `SignalADC_*`，最终 Link 包含 `signal_adc_dma.o`；Q2/Q3 均使用同一采集模块并按 Mode 改采样率。

### D2：Q2 用去直流 AC RMS，不用原始均值

`ACTUAL_DECISION`

原始想法曾使用“输出均值降到输入均值的 0.707”。但调理信号有约 1.65 V 直流偏置，原始均值主要反映偏置，不反映正弦交流幅度。

候选：

1. 原始平均值：被 1.65 V 偏置主导，淘汰。
2. 普通 `max-min` Vpp：高频小信号时对极端码敏感，实测淘汰。
3. 1%/99% Robust Vpp：仍在实测中把约 970 kHz 误到约 1.3 MHz，淘汰。
4. 去直流 AC RMS：选择。

选择理由：`AC_RMS = sqrt(mean((x-mean(x))^2))`。同一种正弦波中，RMS 和 Vpp 的比例固定，所以比较 RMS 的 0.707 与比较 Vpp 的 0.707 在理论上对应同一 `-3 dB` 点；同时 RMS 使用全帧样本，降低极端值支配。

实际结果：最终源码 `App_MeasureACRMS` 每频点采三帧并平均，`App_RunQuestion2` 使用 0.70710678 门限和相邻频点插值。用户最后确认 A 题整体完成。

### D3：扫频顺序是“设置 → 等待 → 完整计算 → 下一频点”

`ACTUAL_DECISION`

曾怀疑 DDS 步进太快、CPU 还没算完导致停止延迟。最终源码证明不存在后台自动扫频：只有完成当前频点三帧采集和 AC RMS 计算后，程序才调用下一次 `AD9850_SetFrequencyHz`。

结论：计算慢只会增加总时间，不会让程序落后多个 DDS 频点。模拟建立时间仍由 `MODE2_SETTLE_US` 单独验证。

### D4：Q3 固定 5 kHz，不扫频，也不用 100 kHz

`ACTUAL_DECISION`

最初方案考虑 0～500 kHz 扫频，后来改成固定 100 kHz，再根据慢端量程检查改为最终 5 kHz。

原因：原题压摆率最低 0.1 V/us，若输出跨度约 9.6 V，20%～80% 需要约 `0.6×9.6/0.1=57.6 us`。100 kHz 半周期只有 5 us，边沿尚未完成就换向，无法得到高低平台；5 kHz 半周期为 100 us，才有形成完整平台的可能。

结果：最终源码 `MODE3_TEST_FREQUENCY_HZ=5000`，单帧约覆盖 7.68 个周期，并分别平均多条完整上升沿和下降沿。

### D5：Q3 改用名义精确 2 MSPS 时间基准

`ACTUAL_DECISION`

症状：示波器在实际 ADC 测点手动测得 20%～80% 时间约 24.9 us，程序用名义 3.555556 MSPS 换算得到 14.81 us；幅度读数接近，时间读数明显不对。

错误做法：继续相信 `32 MHz/9` 的 Timer 名义触发率就等于 ADC/DMA 实际保留的点间隔。

选择：Q3 前调用 `SignalADC_SetSampleRate(2000000)`，形成 `32 MHz/16` 的整数时基；时间公式使用 `SignalADC_GetConfiguredTriggerRate()` 返回的配置值。

结果：用户确认修改后与示波器一致。该结果只证明这个工程在 2 MSPS 下的测量闭环，不证明芯片“最大只能 2 MSPS”。

### D6：Q3 分别测上升沿和下降沿，并使用实测输出跨度

`ACTUAL_DECISION`

选择：5%/95% 分位数估计平台；基于平台建立 20% 和 80% 门限；分别平均完整上升沿和下降沿的交点时间；将 ADC 侧 robust Vpp 乘以调理比例，得到 DUT 实际输出 Vpp。

原因：压摆率定义需要“输出电压变化量 / 输出变化时间”。DDS 输入幅度不能代替 DUT 输出幅度；上升沿和下降沿也可能不同，不能混成一个数。

### D7：Q4 使用 ADC1 软件触发，不新增 DMA 链

`ACTUAL_DECISION`

需求：DDS 关闭、输入输出为 0 V 条件下，读取一个缓慢的直流分流电压并平均。

选择：物理 ADC1、SysConfig 名 `POWER_ADC`，循环 256 次单次转换，code→V 后用 Mean，再算电流和功耗。

原因：不需要固定波形时基或无缝流；再加 Timer/Event/DMA 会增加资源和排错面。

### D8：DAC0 直接 DriverLib 写码，VCA820 公式留在应用层

`ACTUAL_DECISION`

历史开发中明确放弃为了简单 DAC 动作再引入多层 wrapper，最终用 `DL_DAC12_output12(DAC0, code)`；VCA820 没有正式 Device Card，理论反解函数留在本题应用层。

复用限制：这不是 VCA820 公共 API。换电路必须重新读 Datasheet、测 DDS 幅度、拟合控制曲线并检查 DAC 输出范围。

## 事后工程复盘

### P1：新的同类题应先看当前 Measurement Recipe

`POST_HOC_ENGINEERING_NOTE`

当前知识库已经存在 `measurement_bandwidth`、`measurement_slew_rate` 等 Recipe。它们不是这个历史工程开发时已调用的代码依赖。新任务应先读当前 Recipe/Card，再决定是否复用这里的应用 Glue 思路。

### P2：新工程不应默认选择历史母版

`POST_HOC_ENGINEERING_NOTE`

该工程来自 `peripheral_system_template` lineage，但当前 README 已把它标为 `LEGACY CALLBACK REFERENCE`。新任务必须重新经过 Router/Template Registry；历史成功不覆盖当前推荐。

### P3：可以把自动三参数流程拆成状态机

`POST_HOC_ENGINEERING_NOTE`

最终工程是人工模式选择和阻塞测量。若新题严格要求一键、无人工干预和 60 秒上限，可在应用层采用 `INIT → UGBW → SR → POWER → DISPLAY → DONE` 状态机，并给每阶段设时间预算。这个结构不是本历史工程已经实现的事实。

### P4：Q2 应增加前端平坦度/参考通道证明

`POST_HOC_ENGINEERING_NOTE`

本例单通道把 DDS、VCA820、夹具、调理和 ADC 的共同衰减都算进响应。更严格的新题应根据资源比较参考通道、校准表或独立前端带宽验证；不能把本例的单通道链当成通用真值。
