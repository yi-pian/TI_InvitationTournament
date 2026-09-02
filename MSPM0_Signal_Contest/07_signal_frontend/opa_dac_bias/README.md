# OPA + DAC 偏置预算

这个模块用于计算“先加直流偏置、再放大”后的理论输出电压：`Vout = Vbias + Gain x Vin`。它适合在比赛设计阶段检查双极性信号是否能安全送入单极性 ADC；它不会初始化 DAC、OPA 或 ADC。

## 什么时候用

- 输入信号有负半周，ADC 却只能测 `0 V` 以上时。
- 需要把信号抬到 1.65 V 等中点附近，再做放大和 ADC 采样时。
- 需要先计算最坏情况下的输出范围，避免 OPA/ADC 饱和时。

不要用它直接输出 DAC 电压，也不要认为调用函数后片上 OPA 已经配置完成。

## 输入与输出

| 参数 | 单位 | 含义 |
|---|---:|---|
| `input_voltage_v` | V | 当前输入电压，可为负数 |
| `gain` | 无 | 设计的总电压增益，可为负数 |
| `dac_bias_v` | V | 叠加的偏置，必须大于等于 0 |
| `output_voltage_v` | V | 成功后写入理论输出 |

函数只校验输出指针和 `dac_bias_v`，不检查计算结果是否超过供电或 ADC 范围。因此计算完还应使用 `opa_to_adc` 检查量程。

## SysConfig 配置（真实硬件）

本 helper **不需要 SysConfig**。但实际的“DAC + OPA + ADC”链路需要分别配置：

1. 打开比赛工程自己的 `.syscfg`，`Add -> DAC12`（或当前器件和拓扑实际使用的 DAC）。
2. 选择参考源，按真实参考电压把目标 `dac_bias_v` 换成 DAC code；固定偏置不需要 Timer、DMA 或 Event。
3. `Add -> OPA`，在 `Basic Configuration` 选择需要的 topology。
4. 在 OPA 的输入、反馈和路由页面确认：信号输入、DAC 偏置来源、反馈方式、增益和输出路径。
5. 只有必须把 DAC/OPA 输出接到板外时才启用对应 Output Pin；片内路由到 ADC 时，ADC 要选择实际存在的 internal connection。
6. 配好 ADC12 的输入通道、参考源、采样时间和触发方式，再 Generate 并检查生成宏与 Pin 冲突。

不要把偏置计算公式反推出不存在的 SysConfig 选项，也不要手工修改 `ti_msp_dl_config.c/.h`。详细路线见 [OPA/GPAMP 教程](../../00_docs/SYSCONFIG_BEGINNER_GUIDE.md#opa-gpamp)。

## 如何使用

```c
#include "signal_opa_dac_bias.h"

void opa_dac_bias_example(void)
{
    float output_voltage_v = 0.0f;
    signal_result_t result = SignalOPADACBias_Calculate(
        -0.10f, 4.0f, 1.65f, &output_voltage_v);

    /* 成功时：output_voltage_v = 1.25 V。 */
    if (result != SIGNAL_RESULT_OK) {
        return;
    }
}
```

比赛时应至少用 `Vin_min` 和 `Vin_max` 各算一次，得到 `Vout_min/Vout_max` 后交给 `SignalOPAToADC_CheckRange()`。

## API

| API | 用途 |
|---|---|
| `SignalOPADACBias_Calculate` | 计算一次理论输出电压 |
| `SignalOPADACBias_GetModuleStatus` | 返回软件模块状态 |

`SignalOPADACBias_Calculate()` 在 `output_voltage_v == NULL` 或 `dac_bias_v < 0` 时返回 `SIGNAL_RESULT_INVALID_ARGUMENT`；其他情况下返回 `SIGNAL_RESULT_OK`。

可直接查看 [`README_MINIMAL_EXAMPLE.c`](README_MINIMAL_EXAMPLE.c) 和 [`README_FULL_EXAMPLE.c`](README_FULL_EXAMPLE.c)，声明见 [`signal_opa_dac_bias.h`](signal_opa_dac_bias.h)。

## 模块链

```text
输入范围 + 目标 gain + 目标 bias
    -> SignalOPADACBias_Calculate（算最小/最大值）
    -> SignalOPAToADC_CheckRange
    -> SysConfig 配置 DAC、OPA、ADC
    -> ADC 采样和电压换算
```

## 常见错误

- 把 `1.65 V` 当成固定答案；偏置应随 ADC 参考、输入幅度和增益重新计算。
- 只算正峰值，忘记负峰值。
- 把 `DAC12` 物理输出、片内 DAC 路由和外部回接当成同一种连接。
- 需要固定偏置却无故加入 DMA/Timer，增加资源冲突。
- 只验证 ADC 范围，未验证 OPA 的输入共模、输出摆幅和供电范围。

## 资源与验证状态

- 模块不动态分配内存，单次调用为常数时间。
- 软件计算已纳入 PC 构建；硬件 OPA/DAC/ADC 链尚未针对具体比赛开发板实测。
- `MODULE_STATUS_BUILD_VERIFIED` 只说明源码构建通过，不等于实板可用。
- 文档约定见 [BEGINNER_README_STANDARD.md](../../00_docs/BEGINNER_README_STANDARD.md)。
