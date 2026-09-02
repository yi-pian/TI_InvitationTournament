# ADC To Voltage 原始码转电压

**等级：LEVEL A — DIRECT RECIPE。** 它只做线性换算，不访问 ADC 寄存器或 SysConfig。

## 1. 它解决什么问题

输入：`uint16_t raw[N]`，单位 code。输出：`float voltage_v[N]`，单位 V。换算同时支持前端比例 `input_scale` 和固定偏移 `offset_v`。

## 2. 最简单实现

### 比赛现场直接复制这一段

<!-- DIRECT_COPY_BEGIN -->
```c
#include <stdint.h>

static void recipe_adc_to_voltage(const uint16_t *raw, float *voltage_v,
                                  uint32_t n, float vref_v,
                                  uint32_t adc_max_code,
                                  float input_scale, float offset_v)
{
    uint32_t i;
    float volts_per_code = (vref_v * input_scale) / (float)adc_max_code;

    for (i = 0U; i < n; ++i) {
        voltage_v[i] = (float)raw[i] * volts_per_code + offset_v;
    }
}
```
<!-- DIRECT_COPY_END -->

前提：指针有效、`n > 0`、`adc_max_code > 0`。

## 3. 这段代码放哪里

DMA 完成一帧 raw 后调用，随后再做 Vpp/RMS/FFT：

```text
ADC DMA raw[] -> ADC To Voltage -> voltage_v[] -> Measurement/DSP
```

## 4. 每一行什么意思

- `vref_v / adc_max_code` 是 ADC 每一个 code 对应的电压。
- `input_scale` 把 ADC 引脚电压还原到被测端，例如 1:2 分压常取 2。
- `offset_v` 处理固定线性偏移；没有偏移就填 0。
- 比例先算一次，循环中只乘加。

## 5. main / processing 实际例子

```c
#define SIGNAL_ADC_VREF_V       (3.3f)
#define SIGNAL_ADC_MAX_CODE     (4095U)
#define SIGNAL_INPUT_SCALE      (1.0f)
#define SIGNAL_INPUT_OFFSET_V   (0.0f)

recipe_adc_to_voltage(raw, voltage_v, SIGNAL_SAMPLE_COUNT,
    SIGNAL_ADC_VREF_V, SIGNAL_ADC_MAX_CODE,
    SIGNAL_INPUT_SCALE, SIGNAL_INPUT_OFFSET_V);
```

## 6. 题目里需要改什么

必须核对 ADC 分辨率对应的最大码、实际参考电压、前端分压/增益和偏移。12-bit 单端 ADC 常见最大码是 4095，但应以当前 SysConfig/数据格式为准，不能机械照抄。

## 7. 什么情况下这种方法会不准

VREF 实际值不准、前端增益/分压误差、ADC 差分/有符号格式、削顶或非线性都会让简单线性公式失效。`input_scale` 的方向写反是最常见错误。

## 8. 精度不够怎么办

先用已知电压检查 0 点和满量程附近。稳定线性误差用正式 ADC Gain/Offset Calibration；差分/有符号 ADC 应按真实编码格式写专用换算，不能继续使用本无符号公式。

## 9. 完整例子

`vref=3.3 V`、12-bit、scale=1、offset=0 时，raw=0 输出 0 V，raw=4095 输出 3.3 V。该代码通过 PC 真值测试；真实 VREF 准确度仍需实板校准。
