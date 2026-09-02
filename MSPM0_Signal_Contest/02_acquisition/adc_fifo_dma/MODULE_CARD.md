# MODULE CARD: adc_fifo_dma

| 项目 | 内容 |
|---|---|
| 作用 | ADC 自动连续转换，FIFO 将两个 12-bit 样本打包，32-bit DMA 采完整一帧 |
| 输入 | SysConfig ADC/FIFO/DMA、名义 Fs、4-byte aligned `uint16_t buffer[N]`、偶数 N |
| 输出 | 顺序排列的 raw 帧、样本数、名义 Fs、运行状态 |
| 依赖 | `PROFILE_08_ADC_FIFO_MAX`；`signal_status.h`；生成的 `ti_msp_dl_config.*` |
| 资源 | 1 个 ADC、1 个 DMA channel、ADC IRQ；不占 Timer/Event |
| RAM | `2N` bytes + 常数状态；无动态分配 |
| 重复启动 | 每次 Start 先 reset/re-init ADC，以清除可能残留的 FIFO 数据 |
| 当前状态 | `MODULE_STATUS_BUILD_VERIFIED`；隔离 SysConfig/compile/full-link 已通过，Board 未验证 |
| 不适合 | 需要任意精确 Fs、相干采样、双缓冲无缝连续流 |
| 移除 | 删除本模块 c/h 和调用，再删除 SysConfig 的 SIGNAL_ADC_FIFO / DMA 资源 |

这里的“满速”是 ADC 连续转换吞吐模式，不是 `Init` 参数设置出的速率。时间轴使用的 nominal Fs 必须与 SysConfig 的 ADC Conversion Period 对应，并应在实板上校验。
