# OPA 电压跟随器（Buffer）参数设计

这个模块生成一个理想的 OPA Buffer 软件配置：增益为 1，输出电压理想上跟随输入。它适合做信号隔离、提高驱动能力或把高阻传感器送入 ADC 前的设计预算；不会真正启用片上 OPA。

## 什么时候用

- 前级信号源驱动能力不足，不希望 ADC 采样或后级负载拉低信号时。
- 不需要放大，只希望隔离前后级时。
- 希望先把信号从外部 Pin 或片内 DAC/VREF 节点缓冲后再送 ADC 时。

不要用 Buffer 解决信号幅度太小的问题；它的理想增益是 1，需要放大时使用 `opa_noninverting_pga` 或 `opa_inverting`。

## 输入与输出

`SignalOPABuffer_MakeConfig()` 的输入只有 `bias_voltage_v`：必须大于等于 0，单位为 V。成功后写入：

```text
mode                    = SIGNAL_OPA_MODE_BUFFER
resistor_feedback_ohm   = 0
resistor_input_ohm      = 0
bias_voltage_v          = 输入参数
```

这个 config 只是软件描述，不代表硬件反馈路径已经闭合。

## SysConfig 配置（真实片上 OPA）

1. 在比赛工程 `.syscfg` 中执行 `Add -> OPA`。
2. 在 `Mode/Topology` 或 Quick Profile 中选择 Buffer/Voltage Follower。
3. 设置 `PSEL/NSEL/MSEL`，确认输入到底来自外部 Pin、DAC、VREF 还是其他内部节点。
4. 在 feedback/routing 页面确认负输入确实接到输出，形成电压跟随器。
5. 只有需要把缓冲结果引出板外时才启用 OPA Output Pin；若直接送 ADC，要在 ADC12 选择正确 internal connection。
6. Generate 后检查 OPA instance、Pin/IOMUX 和 ADC 输入路由；不要手改生成文件。

详见 [OPA/GPAMP 教程](../../00_docs/SYSCONFIG_BEGINNER_GUIDE.md#opa-gpamp)。实际可用字段和 Pin 必须以当前芯片 GUI 与开发板原理图为准。

## 最小示例

```c
#include "signal_opa_buffer.h"

void opa_buffer_example(void)
{
    signal_opa_config_t config;
    signal_result_t result = SignalOPABuffer_MakeConfig(1.65f, &config);

    /* 成功时：config.mode 为 BUFFER，理想增益为 1。 */
    if (result != SIGNAL_RESULT_OK) {
        return;
    }
}
```

## API

| API | 用途 |
|---|---|
| `SignalOPABuffer_MakeConfig` | 生成 Buffer 软件配置 |
| `SignalOPABuffer_GetModuleStatus` | 返回软件模块状态 |

`config == NULL` 或 `bias_voltage_v < 0` 时，主函数返回 `SIGNAL_RESULT_INVALID_ARGUMENT`。

## 模块链

```text
高阻信号源/内部模拟节点 -> OPA Buffer -> ADC 或后级
```

先用本模块生成软件设计参数，再通过 SysConfig 实现真实拓扑；如果输出送 ADC，可用 `opa_to_adc` 检查最坏量程。

## 常见错误

- 认为 Buffer 能把信号放大；它只能提供理想 1 倍增益。
- 只打开 OPA Output Pin，没有在 feedback/routing 中形成跟随反馈。
- 同时按内部 route 和外部跳线的方式配置同一链路。
- 忽略 OPA 输入共模、输出摆幅和对 ADC 采样电容的建立时间。

## 资源与验证状态

- 不动态分配内存，执行为常数时间。
- 当前通用 config 缺少真实 MUX、反馈、输出、GBW 和 chopping 字段，硬件集成状态为 `API_GAP`。
- 不能仅凭本模块完成 MSPM0G3507 OPA Buffer 实板配置；以 SysConfig/原理图为准。
- 文档约定见 [BEGINNER_README_STANDARD.md](../../00_docs/BEGINNER_README_STANDARD.md)。
