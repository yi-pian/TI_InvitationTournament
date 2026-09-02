# 外设层与算法层数据契约

目标：算法代码只接触明确的数据帧和参数，不知道 ADC 寄存器、DMA channel、Event route、ISR 或 SysConfig 宏。

## 2026-08-08 API 对账结论

`signal_u16_frame_t` 是跨层契约的描述结构，不是 `SignalADC_*` 当前返回的单一对象。
真实公开 API 分别提供：

- `SignalADC_GetBuffer()` → `const uint16_t *`
- `SignalADC_GetSampleCount()` → `uint16_t`
- `SignalADC_GetConfiguredTriggerRate()` → `uint32_t` Hz

应用/Glue 只在调用边界组合这些值，并通过正式 `SignalADCToVoltage_Process()` 完成 code→V。
Round 1 没有让算法 include 外设 `signal_types.h`，以规避 INT-001 的 complex typedef 冲突。
双 ADC 的真实集成输出是两块独立 `uint16_t[N]` buffer 和共同 configured rate；Timer Capture
在 P05 ISR 先把 down-count capture 转成正向时间戳；DAC/DDS 接收 `const uint16_t[] + count +
update_rate_hz + repeat`。这些结论已由 8 个完整 application link 验证。

## 单 ADC 输入

当前统一输入类型为 `01_bsp/common/signal_types.h` 中的 `signal_u16_frame_t`：

```c
typedef struct {
    const uint16_t *data;
    size_t count;
    uint32_t sample_rate_hz;
    uint8_t adc_bits;
    float reference_voltage_v;
} signal_u16_frame_t;
```

契约：

- `data[0..count-1]` 在算法调用期间稳定且只读。
- `count > 0`，且跨算法 ABI/文件格式时必须能安全转换为 `uint32_t`。
- `sample_rate_hz` 必须注明来源。默认是配置触发率，不得伪称外部实测率。
- MSPM0G3507 当前 ADC raw 为 12 bit 时，`adc_bits=12`，有效 raw 范围 0..4095。
- `reference_voltage_v` 是算法换算时采用的有效参考；未知时不允许默认为高精度标定值。
- buffer 所有权仍属于采集层。算法若需跨下一次 Start 保存数据，必须复制到自己的静态 workspace。

ADC_DMA 适配示例只负责构造 frame：

```c
signal_u16_frame_t frame = {
    .data = SignalADC_GetBuffer(),
    .count = SignalADC_GetSampleCount(),
    .sample_rate_hz = SignalADC_GetConfiguredTriggerRate(),
    .adc_bits = 12U,
    .reference_voltage_v = 3.3f,
};
```

上例的 Vref 仅在 profile 确认使用 VDDA=3.3 V 且应用接受该估计时成立。

## 双 ADC 输入

双通道算法接收两个独立 `signal_u16_frame_t`：

```c
typedef struct {
    signal_u16_frame_t channel_a;
    signal_u16_frame_t channel_b;
} peripheral_dual_adc_frame_t;
```

必须满足：

- 两帧 `count` 相等、`sample_rate_hz` 相等。
- 两个 ADC 由同一 Timer 周期的两个 Event publisher channel 触发；P02/P06 按此路由。
- “同一 Timer 触发”不等于已测得零相差。涉及相位的算法必须保留 ADC aperture、Event 延迟和模拟前端延迟校准项。
- 当前 `adc_dual_sync` public API 只做 interleaved 数据拆分；将两个独立 DMA buffer 组织成此结构由应用 adapter 完成。

## Timer Capture 输入

当前正式 API 接收 `uint32_t timestamps[]`，并由：

- `SignalTimerCapture_Delta()` 处理相邻时间戳和计数器回绕；
- `SignalTimerCapture_MeanPeriod()` 输出平均 ticks/period。

应用交给频率算法前还必须提供 `capture_clock_hz`。换算关系为 `frequency_hz = capture_clock_hz / mean_period_ticks`。P05/P06 的 COMP→Event→TIMG capture 只建立硬件路径，不替算法解释数据。

## DAC 输出输入

硬件输出层只接收：

- `const uint16_t *samples`
- `size_t count`
- `uint32_t update_rate_hz`
- `uint8_t dac_bits`（当前 DAC0 为 12 bit）
- repeat/one-shot 策略

生成算法负责波表内容，外设 adapter 负责 Timer/Event/DMA/DAC。`SignalDDS_GetConfiguredFrequency()` 和 DAC update rate 都是配置推导量，实板频率仍需独立验证。

## 生命周期与并发

```text
Peripheral Init -> Start -> ISR marks complete -> application acquires frame
-> algorithm hook(frame) -> optional output adapter -> release/restart
```

- ISR 只清硬件 flag、更新有限状态和发布完成事件；不在 ISR 内做 FFT、浮点统计、CSV 输出或波表生成。
- 采集开始前应用必须确认上一帧已经 release，或使用 ping-pong/ring buffer 明确所有权。
- 算法失败不能直接重写 DMA/Timer 寄存器；返回 `signal_result_t` 给应用状态机处理。
- UART 日志只是 debug consumer，不进入正式采集依赖图。

## 禁止跨层依赖

算法层不得：

- include `ti_msp_dl_config.h` 或 DriverLib 头文件；
- 引用 `ADC0`、`DMA_CHx`、`TIMGx`、Event channel 宏；
- 直接清中断标志或重新装 DMA；
- 假设 buffer 永远是某个全局符号；
- 把 configured sample rate 当成 calibrated/measured sample rate。
