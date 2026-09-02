# example02 步骤 03：双路同步 ADC 测量

## 一、SysConfig

沿用 `02_acquisition/adc_dual_sync/README.md`：ADC A=ADC0/PA25、ADC B=ADC1/PA17，
同一个 Timer 事件触发两路转换，DMA 使用 CH0/CH1，TIMG0 设为 `CPUCLK_FREQ` 基线和
`65536U` 最大计数。采样率为 `SIGNAL_SAMPLE_RATE_HZ=100000U`，每频点采样
`SIGNAL_SAMPLE_COUNT=1024U` 点。

## 二、复制的采集代码

```c
const signal_dual_adc_config_t adc_config = {
    SIGNAL_SAMPLE_RATE_HZ, CPUCLK_FREQ, 65536U
};
SignalDualADC_Init(&adc_config);
SignalDualADC_Start(g_raw_in, g_raw_out, SIGNAL_SAMPLE_COUNT);
while (!SignalDualADC_IsFinished()) { __WFI(); }
```

第一行结构体来自 ADC README 的配置顺序：目标 Fs、Timer 时钟、Timer 最大计数；
`Init` 只初始化一次；`Start` 把两路目标数组交给 DMA；`IsFinished` 为假时睡眠等待，
避免主循环忙等。`g_raw_in/g_raw_out` 和处理顺序是本题自写。

## 三、自写 Vpp 计算

`App_PeakToPeak()` 逐点维护最小值和最大值，最后返回 `max-min`。`min=4095U`、
`max=0U` 是 12 位 ADC 的边界初值；函数不改变原始数组。增益计算为
`out_vpp/in_vpp`，当输入 Vpp 为零时返回 0，防止除零。

## 四、与 README 的差异

README 给的是通用双路采集流程，本题额外在每个扫频点重启一次采集，并把两路 Vpp 与
频点关联保存到 `g_gain[]`；没有修改 ADC 模块。正式比赛应把固定等待改成带超时模板，
避免外设故障时永久停在 `while`。
