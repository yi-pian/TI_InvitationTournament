# example01-2 双路同步 ADC 采集

## SysConfig

按 `adc_dual_sync/README.md` 的路径添加 ADC12 两实例、DMA、Timer/Event；本工程沿用 22_X 已验证角色：ADC0/PA25 为 A，ADC1/PA17 为 B，DMA_CH0/CH1，TIMG0 产生 10 us 触发。与 README 的差异只有实例已经在母版中存在，点击 GUI 后核对，不改生成的 `ti_msp_dl_config.*`。

## 复制到 main 的代码

复制 README 最小示例的 `signal_dual_adc_config_t`、`SYSCFG_DL_init()`、`SignalDualADC_Init()`、`SignalDualADC_Start()` 和 `SignalDualADC_IsFinished()` 顺序。`__WFI()` 也按示例保留，用来等待 DMA 中断。

## 自写代码逐行解释

```c
static uint16_t g_raw_x[SIGNAL_SAMPLE_COUNT]; /* A 路整帧缓冲。 */
static uint16_t g_raw_y[SIGNAL_SAMPLE_COUNT]; /* B 路整帧缓冲。 */
const signal_dual_adc_config_t adc_config = { /* 创建配置对象。 */
    SIGNAL_SAMPLE_RATE_HZ, CPUCLK_FREQ, 65536U /* Fs、Timer 时钟、计数上限。 */
};
g_adc_status = SignalDualADC_Init(&adc_config); /* 让模块保存硬件参数。 */
g_adc_status = SignalDualADC_Start(g_raw_x, g_raw_y, SIGNAL_SAMPLE_COUNT); /* 启动一帧。 */
while (!SignalDualADC_IsFinished()) { __WFI(); } /* 完成前低功耗等待。 */
```

这些数组和配置属于应用层；API 名称、参数顺序和返回值检查来自 README。
