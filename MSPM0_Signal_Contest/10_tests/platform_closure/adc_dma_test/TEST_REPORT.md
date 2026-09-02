# ADC DMA 单通道采集测试报告

## 构建状态：BUILD_VERIFIED

| 项目 | 值 |
|------|-----|
| Flash 用量 | 2664 bytes (0x0a68) |
| SRAM 用量（含堆栈） | 668 bytes (0x029c / 0x0200) |
| ADC 通道 | ADC12_CH0 |
| 参考电压 | VREFP_INTERNAL_LV_PLUS (+1.2V), VREFM_GND |
| 满量程 | 3.3V |
| DMA 触发 | 是（DMA Channel 0） |
| ADC 内存缓冲 | MEM_0 |
| Timer 时钟源 | BUSCLK (SYSCLKOUT0) |
| Timer 分频器 | 1x |
| Timer 预分频器 | 1x |
| Event Fabric | TimerG0_CH0 -> ADC12_EVT_TRIGGER_START |

## 验证证据

- [SysConfig 生成](../build/adc_dma_minimum/ti_msp_dl_config.c.h)：PASS
- [编译](../build/adc_dma_minimum/*.o)：PASS（无警告、无错误）
- [链接](../build/adc_dma_minimum/adc_dma_minimum.map)：PASS
- Map 分析：
  - `.text` 代码段：1318 bytes（含中断向量表 240 bytes）
  - `.rodata` 常量数据：259 bytes
  - `.bss` 未初始化数据：512 bytes（含 512 byte 堆栈）
  - `.data` 已初始化数据：156 bytes

## 接口契约（signal_adc_dma.h）

```c
typedef struct {
    uint32_t sample_rate_hz;
    uint32_t timer_clock_hz;      // CPUCLK_FREQ
    uint32_t timer_max_count;     // 0xFFFF (65535)
} signal_adc_dma_config_t;

signal_result_t SignalADC_Init(const signal_adc_dma_config_t *config);
signal_result_t SignalADC_SetSampleRate(uint32_t sample_rate_hz);
signal_result_t SignalADC_Start(uint16_t *buffer, uint16_t sample_count);
void            SignalADC_Stop(void);
bool            SignalADC_IsFinished(void);
signal_status_t SignalADC_GetStatus(void);             // IDLE/RUNNING/DONE/ERROR
const uint16_t *SignalADC_GetBuffer(void);
uint16_t        SignalADC_GetSampleCount(void);
uint32_t        SignalADC_GetConfiguredTriggerRate(void);
```

## 关键设计点

1. **Timer→Event→ADC→DMA→RAM**：单次块采集流程完整，无 CPU 干预。
2. **采样率计算**：使用整数周期计数，`configured_trigger_rate_hz = timer_clock_hz / round(timer_clock_hz/sample_rate_hz)`。
3. **中断处理**：`SIGNAL_ADC_INST_IRQHandler()` 仅关闭 ADC/DMA/Timer 并置 `MODULE_DONE`，无耗时运算。
4. **DMA 通道号**：由 SysConfig 自动生成为 `DL_DMA_CHANNEL_0`，中断掩码在运行时根据实例推导。

## 限制与假设

- 采样率上限：`sample_rate_hz < timer_clock_hz / (timer_max_count+1)`。
- Timer 上电时从 0 开始计数，未配置上电清零；若需复位，需在启动前手动设置 `TimerCount = LoadValue - 1`。
- DMA 目标缓冲区在采集完成前不得释放或复用。

## 下一步

- 硬件验证：上板后使用函数发生器输入已知频率正弦波，比较连续多帧的 FFT 谱峰与预期基频。
- 压力测试：长时间连续采集（如 1 小时@100kS/s）检查内存泄漏和中断堆积。
- 时序测量：用逻辑分析仪抓取 Timer 翻转、ADC 采样、DMA 完成三个事件，计算端到端延迟。
