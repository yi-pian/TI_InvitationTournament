# 信号数据模型：先弄清数组里装的是什么

同一个波形在工程中会经过三种表示。很多“算法算错”其实是把其中两种混在了一起。

## 1. ADC RAW：原始码值

```c
uint16_t raw_codes[] = {2047U, 2172U, 2295U, ...};
```

- 单位：ADC code，不是 V。
- 典型 12 位范围：0~4095。
- 通常含有模拟偏置，例如双极性正弦被抬到 1.65 V 后采样。
- 可直接判断是否撞到 0/满量程，但不应直接把 `2047` 称作 `1.65 V`。

## 2. 零偏/有符号离散信号

```c
float centered_v[] = {-0.10f, 0.00f, 0.10f, ...};
```

- 类型：通常 `float`。
- 这里仍然可以用 V；“零偏”只表示平均值已被减掉，不表示无单位。
- 可以含负数。
- 常由 `ADC_ToVoltage -> RemoveDC` 得到。
- 适合过零、FFT、相关、交流 RMS。

## 3. 实际物理量

```c
float voltage_v[] = {1.55f, 1.65f, 1.75f, ...};
```

- 单位明确为 V；也可能是经过传感器比例换算后的 A、Pa 等。
- 本库第一阶段主要使用 V。
- ADC_ToVoltage 的 `input_scale` 可补偿外部衰减/放大；其含义必须与前端电路一致。

## 4. 典型流向

```text
uint16_t ADC RAW (code)
        │ ADC_ToVoltage
        ▼
float 物理电压 (V，通常仍含偏置)
        │ RemoveDC
        ▼
float 零偏电压 (V，均值约为 0)
        │ Hann / FFT / ZeroCross / RMS
        ▼
带单位结果：Hz、V、deg、%、dB
```

## 5. 如何判断手里是哪一种

在调用算法前回答四个问题：

1. 数组元素类型是 `uint16_t` 还是 `float`？
2. 数值单位是 code、V，还是无量纲？
3. 信号是否仍含直流偏置？
4. `count` 和 `sample_rate_hz` 分别是多少？

回答不出就先停下来检查上游，不要靠“结果看起来差不多”继续拼。

## 6. 是否采用通用 signal_buffer_t

当前基础算法**不强制**使用带 `void *` 和数据类型枚举的通用 buffer。理由：

- `void *` 会隐藏编译期类型检查；
- 小白容易把 RAW 直接交给需要 `float` 的算法；
- 多数基础 API 用三个参数就能表达清楚。

因此当前推荐：

```c
SignalRMS_Process(voltage_v, count, &result);
SignalZeroCross_Process(centered_v, count, &cfg, events, event_capacity, &result);
```

若以后出现大量流式 pipeline，需要统一传递 timestamp、通道标识和校准信息，再评估增加“强类型 frame”，而不是直接用不可检查的 `void *`。

## 7. 常见错误

- 把 12 位满量程除以 4096，但配置中却声明 `adc_max_code=4095`；本库换算使用“最大可出现码值”4095。
- 已经经过 RemoveDC 的数据又拿去测 DC，得到接近 0 是必然的。
- 用总 RMS 代替 AC RMS；带 1.65 V 偏置时两者差别很大。
- 把每通道采样率误写成双通道总转换吞吐率，导致频率结果翻倍。
- 把相位弧度当角度显示；字段名必须用 `_rad` 或 `_deg`。
