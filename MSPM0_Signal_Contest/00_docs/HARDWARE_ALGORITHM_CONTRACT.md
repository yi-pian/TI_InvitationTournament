# 硬件层—算法层接口契约

## 1. 这份契约解决什么问题

硬件层负责“按正确时刻把 ADC 数据搬进 RAM”，算法层负责“解释 RAM 里的数字”。两层只交换数据和元信息，不交换 DMA 通道号、Timer 实例或 ADC 寄存器地址。

```text
模拟输入
   ↓
[硬件采集层] ── RAW数组 + 点数 + 采样率 ──> [算法层]
 ADC/Timer/DMA                              电压/频率/FFT/THD
```

这样做的直接收益是：PC 测试可以用文件或合成数据替代 ADC；比赛换采样通道时不必修改算法。

## 2. 单通道 ADC 必须交付的数据

算法层期望得到：

| 项目 | C 类型 | 单位 | 说明 |
|---|---|---|---|
| `buffer` | `const uint16_t *` | ADC code | 已完成采集且在算法运行期间保持有效 |
| `count` | `uint32_t` 或可安全提升的 `uint16_t` | 点 | 有效样本数，不是数组字节数 |
| `sample_rate_hz` | `float`/`uint32_t` | Hz | 本帧真实或配置采样率，必须说明来源 |
| `adc_max_code` | `uint32_t` | code | 12 位 ADC 通常为 4095，而不是 4096 |
| `reference_voltage_v` | `float` | V | 换算使用的参考电压 |

硬件层最终公开函数名尚未由本算法工作区决定。下面只是 **Expected Interface 伪代码**；应用层按另一个硬件任务最终 API 做薄适配，不要求硬件层为了本文改实现：

```c
HardwareADC_Start(adc_buffer, sample_count);
while (!HardwareADC_IsFinished()) {
    /* MCU 中可由应用层等待事件；算法层不关心等待方式。 */
}

const uint16_t *raw = HardwareADC_GetBuffer();
uint32_t count = (uint32_t)HardwareADC_GetSampleCount();
float sample_rate_hz = (float)HardwareADC_GetSampleRateHz();
```

若 `HardwareADC_GetSampleRateHz()` 只是配置推导值，不应在结果中冒充外部仪器实测采样率。若以后硬件层提供校准后的采样率，应用层只需把新值传给算法。

## 3. 双通道同步 ADC 必须交付的数据

算法层期望：

```c
const uint16_t *channel_a_raw;
const uint16_t *channel_b_raw;
uint32_t count;          /* 两通道各自都有 count 点 */
float sample_rate_hz;    /* 每通道采样率，不是两通道合计吞吐率 */
```

还必须说明：

- 两通道样本 `a[n]` 与 `b[n]` 是否来自同一触发时刻；
- 若是轮流采样，固定通道时间差是多少秒；
- 两通道参考电压、增益和零偏是否相同；
- 缓冲区是分离数组还是 `A0,B0,A1,B1...` 交错数组。

若最终硬件接口输出交错 RAW，应用层需要一个明确的 Deinterleave Adapter；该 Adapter 名称由集成时决定。算法不读取其内部 DMA 配置，本独立库也不擅自向硬件目录添加它。

## 4. 缓冲区所有权与生命周期

1. 硬件层拥有并填写 RAW 缓冲区；采集完成前算法不得读取。
2. 算法收到的 RAW 指针按只读处理。
3. 应用层保证算法调用结束前缓冲区不会被下一次 DMA 覆盖。
4. 算法输出缓冲区由应用层提供；算法不 `malloc/calloc/free`。
5. 若模块支持原地处理，必须在头文件和 README 明确写出；否则输入与输出不得重叠。

## 5. 错误与状态边界

- 硬件层报告：未初始化、忙、DMA/ADC/触发错误、采集完成。
- 算法层报告：空指针、点数不足、参数越界、无可测特征、数值错误。
- 算法不能通过读取硬件私有 flag 判断数据是否有效；应用层只在采集完成后调用算法。
- 算法失败时，应用层不得使用对应 result 结构中的数据。

## 6. 算法层明确不要求知道的内容

算法 API 不得要求以下参数：

- DMA Channel 编号；
- Timer Instance、LOAD 值或时钟树寄存器；
- ADC MEMCTL 编号、IRQ 名、Event Fabric 路由；
- SysConfig 生成宏；
- GPIO 引脚复用。

## 7. 最小适配示例

```c
#include "signal_adc_to_voltage.h"
#include "signal_rms.h"

static uint16_t raw_buffer[1024];
static float voltage_buffer_v[1024];

void MeasureOnce(void)
{
    signal_adc_to_voltage_config_t convert_cfg = {
        .adc_max_code = 4095U,
        .reference_voltage_v = 3.3f,
        .input_scale = 1.0f,
        .offset_voltage_v = 0.0f
    };
    signal_rms_result_t rms_result;

    /* 下列 HardwareADC_* 均为契约占位名，不是本库提供的函数。 */
    if (!HardwareADC_Start(raw_buffer, 1024U)) {
        return;
    }
    while (!HardwareADC_IsFinished()) {
    }

    if (SignalADCToVoltage_Process(HardwareADC_GetBuffer(),
            voltage_buffer_v,
            (uint32_t)HardwareADC_GetSampleCount(),
            &convert_cfg) != SIGNAL_ALGORITHM_OK) {
        return;
    }
    (void)SignalRMS_Process(voltage_buffer_v, 1024U, &rms_result);
}
```

上例只是连接契约，不能单独编译。算法模块不包含、也不修改任何 ADC/DMA 正式实现。
