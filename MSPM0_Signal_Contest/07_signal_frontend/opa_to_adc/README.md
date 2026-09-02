# OPA 输出到 ADC 的量程检查

这个模块只检查“预期 OPA 输出范围”是否落在“ADC 允许范围”内，并计算上下余量。它是纯软件 helper：不会配置 OPA、ADC、Pin、时钟或内部路由。

## 什么时候用

- 已算出 OPA 输出的最小/最大电压，想在比赛前防止 ADC 过压或截幅时。
- 修改放大倍数、偏置或 ADC 参考电压后，需要快速复核量程时。
- 设计 OPA -> ADC 链路时，作为 SysConfig 配置之前或之后的数字化安全检查。

## 输入与输出

```c
typedef struct {
    float expected_min_v;
    float expected_max_v;
    float adc_low_limit_v;
    float adc_high_limit_v;
} signal_opa_to_adc_budget_t;
```

成功后：

```text
low_margin_v  = expected_min_v - adc_low_limit_v
high_margin_v = adc_high_limit_v - expected_max_v
```

两个余量都大于等于 0，函数返回 `SIGNAL_RESULT_OK`；任一余量为负，返回 `SIGNAL_RESULT_OUT_OF_RANGE`，但余量数值仍可用于判断是哪一侧超范围。

## SysConfig 配置（真实硬件）

**本模块不需要 SysConfig。** 它不应因为自身被加入工程就自动添加 `OPA` 或 `ADC12`。

真实 OPA -> ADC 链路要单独配置：

1. 按所选 OPA 拓扑，在 `.syscfg` 添加 OPA，配置输入、反馈、增益和输出路由。
2. 添加 ADC12，设置输入通道、参考源、采样时间和触发方式。
3. 重点确认 ADC 输入是 OPA 的 internal connection 还是外部 Pin。前者不需要虚构 OPA_OUT 到 ADC_IN 的外部导线；后者必须按原理图实际连接。
4. Generate 并检查生成宏、PinMux 和资源冲突；不要编辑生成文件。

配置流程见 [OPA/GPAMP 教程](../../00_docs/SYSCONFIG_BEGINNER_GUIDE.md#opa-gpamp)。

## 最小示例

```c
#include "signal_opa_to_adc.h"

void opa_to_adc_example(void)
{
    const signal_opa_to_adc_budget_t budget = {
        .expected_min_v = 0.45f,
        .expected_max_v = 2.85f,
        .adc_low_limit_v = 0.0f,
        .adc_high_limit_v = 3.3f
    };
    float low_margin_v = 0.0f;
    float high_margin_v = 0.0f;
    signal_result_t result = SignalOPAToADC_CheckRange(
        &budget, &low_margin_v, &high_margin_v);

    /* result 为 OK，低端余量 0.45 V，高端余量 0.45 V。 */
    (void)result;
}
```

## API

| API | 用途 |
|---|---|
| `SignalOPAToADC_CheckRange` | 校验范围并计算上下余量 |
| `SignalOPAToADC_GetModuleStatus` | 返回软件模块状态 |

`budget == NULL`、余量指针为空、ADC 上限不大于下限、或预期最大值小于最小值时，函数返回 `SIGNAL_RESULT_INVALID_ARGUMENT`。

可查看 [`README_MINIMAL_EXAMPLE.c`](README_MINIMAL_EXAMPLE.c)、[`README_FULL_EXAMPLE.c`](README_FULL_EXAMPLE.c) 和 [`signal_opa_to_adc.h`](signal_opa_to_adc.h)。

## 模块链

```text
同相/反相增益设计 + 偏置计算
    -> 得到 Vout_min/Vout_max
    -> SignalOPAToADC_CheckRange
    -> SysConfig 配置实际 OPA/ADC 链路
    -> ADC 采样
```

## 常见错误

- 以 `3.3 V` 直接作为 ADC 上限，却忽略实际参考电压或板级限制。
- 只检查 ADC 输入范围，没检查 OPA 输出摆幅、输入共模和供电范围。
- 输入 `expected_min_v > expected_max_v`。
- 把本 helper 当作 OPA/ADC 驱动，期待生成外设宏或初始化硬件。
- 量程检查通过后又修改了 gain/bias，却没有重新检查。

## 资源与验证状态

- 无硬件依赖、不动态分配内存、单次调用为常数时间。
- 可用于 PC 单元测试和比赛参数预检；实际 OPA/ADC 接线仍需单独实测。
- `MODULE_STATUS_BUILD_VERIFIED` 只表示软件构建状态。
- 文档约定见 [BEGINNER_README_STANDARD.md](../../00_docs/BEGINNER_README_STANDARD.md)。
