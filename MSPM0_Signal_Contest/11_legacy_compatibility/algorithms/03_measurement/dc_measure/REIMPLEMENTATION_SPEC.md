# DC Measure clean reimplementation specification

状态：`SPECIFICATION_COMPLETE`。类型：`SOURCE_LOST → CLEAN_REIMPLEMENTATION`。

## Design

DC 就是一帧线性量的算术平均。为了避免重复 Primitive：

- `SignalDCMeasure_FromVoltage` 直接调用现有 `SignalMean_Process`。
- `SignalDCMeasure_FromRawLinear` 只在 raw 域求补偿均值，再应用与 `ADC_ToVoltage` 相同的线性标定式，因此不需要分配 `float[N]` 临时数组。

```text
mean_code = mean(raw)
dc_v = mean_code / adc_max_code × Vref × input_scale + offset_v
```

该等价只对线性增益/偏置成立；非线性传感器换算必须先逐点转换。输入 code 必须在量程内，所有配置必须有限。失败时结果不变，O(N) 时间、O(1) RAM。

旧 `SignalDCMeasure_Raw/Voltage` 签名和单位契约丢失；新 `FromRawLinear/FromVoltage` 是明确的新 API。验证含 raw/voltage 已知均值、满量程、非法 code、NaN、空帧以及 C/Python 差分。
