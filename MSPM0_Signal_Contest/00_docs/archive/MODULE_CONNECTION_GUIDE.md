# 模块连接指南

模块连接只依赖数据契约，不依赖对方内部寄存器。未实现模块标为“计划”，示意 API 不能复制到当前工程中直接编译。

## 已实现：ADC_DMA

输入：

```text
模拟电压：0~VDDA
sample_rate_hz：Hz
sample_count：1~65535
buffer：uint16_t[sample_count]
```

输出：

```c
const uint16_t *samples;
uint16_t sample_count;
uint32_t configured_trigger_rate_hz;
signal_status_t status;
```

可连接：

| 下游模块 | 需要的额外转换 | 状态 |
|---|---|---|
| VoltageConvert | raw、VDDA、ADC 满量程码 | 计划 |
| DC/Mean/Max/Min/Vpp | 可直接处理 raw，输出换算单位需标注 | 计划 |
| RMS | AC RMS 前通常先去 DC；电压 RMS 先做校准/换算 | 计划 |
| Frequency Zero Cross | raw + 点数 + 明确标注语义的配置触发率 | 计划 |
| FFT | raw -> 电压/定点 -> RemoveDC -> Window | 计划 |
| Trigger / WaveReplay | raw + 阈值 + 环形缓冲策略 | 计划 |

当前可编译连接：

```c
SignalADC_Start(adc_buffer, SIGNAL_SAMPLE_COUNT);
while (!SignalADC_IsFinished()) {
    __WFE();
}

const uint16_t *samples = SignalADC_GetBuffer();
uint16_t count = SignalADC_GetSampleCount();
uint32_t configured_trigger_rate_hz =
    SignalADC_GetConfiguredTriggerRate();
```

## 计划契约：FFT

预计输入为预处理后的定点或浮点样本、点数和已完成物理验收的采样率信息；输出为复频谱/幅度谱、频率分辨率和窗增益信息。最终数据类型要先做 MSPM0G3507 RAM/算力验证，不能现在锁死成多个 4096 点 float 数组。

可连接：FundamentalDetect、Harmonic、THD、FFTPeakInterpolation。

## 计划契约：DDS

预计输入为频率、幅度、偏置、初相和波形类型；输出为 DAC 波形。可连接 DUT、ADC_DMA、SweepMeasurement。必须先完成 DAC DC 和 WaveTable -> DMA -> DAC 的独立验证。

## 连接规则

1. 上游必须把“数组指针 + 点数 + 单位/标度 + 采样率语义”一起交付；配置触发率不得冒充外部实测值。
2. 下游不能假设输入一定是理想正弦波，也不能擅自修改上游缓冲区。
3. 需要原地处理的模块必须在 API 和 README 中明确写出会覆盖输入。
4. 算法不得读取采集模块的私有 flag 或 DMA channel。
5. 只有状态为 `MODULE_DONE` 的块缓冲才可进入当前阶段的离线算法链。
