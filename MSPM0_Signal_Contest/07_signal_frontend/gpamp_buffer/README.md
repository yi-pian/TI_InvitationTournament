# GPAMP 电压跟随器参数设计

这个模块生成“单位增益 GPAMP Buffer”的软件配置。它用于低频信号隔离和驱动能力预算，不会启动 GPAMP 硬件。

## 什么时候用

- 慢速传感器或直流信号需要缓冲，且不要求放大时。
- 希望先检查偏置参数，再在 SysConfig 建立 GPAMP 跟随器时。
- 不适合高频或高增益链路；这类场景应先评估 OPA 或外置运放。

## 输入与输出

`SignalGPAMPBuffer_MakeConfig(bias_voltage_v, config)` 要求 `config` 非空、偏置非负；成功后设置：

```text
requested_gain = 1.0
bias_voltage_v = 输入参数
```

## SysConfig 配置（真实 GPAMP）

1. 在 `.syscfg` 添加 `GPAMP`。
2. 选择输入正端，并让负端/反馈路径回接输出，形成 Buffer。
3. 确定输出是内部送 ADC 还是接到 Output Pin；若输出到 ADC，核对 ADC 的真实输入 route。
4. 只有了解 offset、带宽和外部滤波需求后才选择 chopping。
5. Generate 后检查生成宏、PinMux 和资源冲突。

官方 `gpamp_buffer_to_adc` 例子可帮助理解流程，但实际 Pin 必须以开发板原理图为准。详见 [OPA/GPAMP 教程](../../00_docs/SYSCONFIG_BEGINNER_GUIDE.md#opa-gpamp)。

## 最小示例

```c
#include "signal_gpamp_buffer.h"

void gpamp_buffer_example(void)
{
    signal_gpamp_config_t config;
    signal_result_t result = SignalGPAMPBuffer_MakeConfig(1.65f, &config);
    if (result != SIGNAL_RESULT_OK) {
        return;
    }
    /* config.requested_gain 为 1.0。 */
}
```

## API

| API | 用途 |
|---|---|
| `SignalGPAMPBuffer_MakeConfig` | 生成 1 倍 Buffer 软件配置 |
| `SignalGPAMPBuffer_GetModuleStatus` | 返回软件模块状态 |

## 模块链

```text
低频输入 -> GPAMP Buffer -> ADC/后级
```

本模块之后仍要完成 SysConfig，并用实际信号验证输出范围和采样建立时间。

## 常见错误

- 误以为 Buffer 可以提高幅值。
- 打开 chopping 却未验证噪声、带宽和滤波影响。
- 对同一链路同时按内部 ADC 路由和外部 Pin 回接配置。
- 把软件 config 当作 GPAMP 寄存器配置。

## 资源与验证状态

- 不动态分配内存，执行为常数时间。
- 缺少真实 GPAMP MUX、output、RRI、chopping 等字段，硬件集成状态为 `API_GAP`。
- 文档约定见 [BEGINNER_README_STANDARD.md](../../00_docs/BEGINNER_README_STANDARD.md)。
