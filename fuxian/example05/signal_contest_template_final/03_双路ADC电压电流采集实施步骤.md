# example05 步骤二：双路 ADC 电压/电流同步采集

## 1. SysConfig 配置

本步完全按 `adc_dual_sync/README.md` 配置：

| 对象 | 配置 |
|---|---|
| `SIGNAL_ADC_A` | ADC0、Memory0/Input2、PA25、Event、Repeat、DMA done |
| `SIGNAL_ADC_B` | ADC1、Memory0/Input2、PA17、Event、Repeat、DMA done |
| `SIGNAL_ADC_A_DMA` | DMA_CH0、Half Word、Fixed-to-Block、Single |
| `SIGNAL_ADC_B_DMA` | DMA_CH1、Half Word、Fixed-to-Block、Single |
| `SIGNAL_DUAL_ADC_TIMER` | TIMG0、Periodic、2 us、ZERO publisher 1/2 |

两个 ADC 的 Event 都来自 TIMG0 ZERO；A/B 数组的相同下标才可以拿来做相位差。
2 us 是本题对 README 10 us 基线的速度调整，运行时 `SignalDualADC_Init()` 仍把目标
采样率写成 500000U。

模块头文件对 ADC1 的默认 profile 注释写作“marker channel”；本题题面明确给出待测网络
两端电压和电流两路模拟采样，因此在 SysConfig/接线确认 ADC1.2 实际接的是电压前端后，
应用层把 B 数组解释为电压。这个语义适配写在 `main.c`，没有改模块注释或驱动。

## 2. 从 README 复制的 main 代码

```c
static uint16_t g_raw_current[SIGNAL_SAMPLE_COUNT];
static uint16_t g_raw_voltage[SIGNAL_SAMPLE_COUNT];

const signal_dual_adc_config_t adc_config = {
    SIGNAL_SAMPLE_RATE_HZ, CPUCLK_FREQ, 65536U
};
if (SignalDualADC_Init(&adc_config) != SIGNAL_RESULT_OK) {
    while (1) { }
}
```

采集一帧的顺序也逐字保持 README：

```c
if (SignalDualADC_Start(g_raw_current, g_raw_voltage,
        SIGNAL_SAMPLE_COUNT) != SIGNAL_RESULT_OK) return 0U;
for (i = 0U; i < APP_ADC_WAIT_LIMIT; ++i) {
    if (SignalDualADC_IsFinished()) break;
    __NOP();
}
if (!SignalDualADC_IsFinished()) {
    SignalDualADC_Stop();
    return 0U;
}
```

第一行设置两路 DMA 目的地址；循环只等待，不在 DMA 未完成时读数组；超时调用 README
规定的 `Stop()` 并放弃本帧。浮点计算放在主循环，不放进 DMA ISR。

## 3. 题目组合的物理换算

等待完成后，`main.c` 只新增以下少量应用逻辑：

1. 求两路均值，消除模拟前端的 1.65 V 偏置。
2. 用 `SIGNAL_VOLTAGE_SCALE_V_PER_CODE`、`SIGNAL_CURRENT_SCALE_A_PER_CODE` 换成 V/A。
3. 对平方和开根号，得到 `Vrms` 和 `Irms`。
4. 保存数组，交给下一步相位和阻抗计算。

组合代码还读取 `SignalDualADC_GetConfiguredRate()`，将 Timer 取整后的实际采样率传给
相位相关；若接口暂时返回 0，才回退到 `SIGNAL_SAMPLE_RATE_HZ`。这沿用 ADC README 对
“目标 Fs 与 Timer 实际 Fs 可能不同”的说明。

两路原始数组绝不在 DMA 完成前显示、拟合或传给 TFT。

## 4. 现场验收

先把同一正弦信号分接到两个模拟输入，确认 `g_raw_current`、`g_raw_voltage` 都随时间
变化且没有满量程饱和；再接入待测网络。若只有一路完成，回 SysConfig 检查对应 Event、
DMA trigger 和 DMA done，不改 `signal_dual_adc_mspm0g3507.c/.h`。
