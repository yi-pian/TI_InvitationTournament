# OPA 反相放大参数设计

这个模块按理想反相放大器公式计算反馈电阻，并生成一个 `signal_opa_config_t` 供软件预算使用。它不写 OPA 寄存器，也不会把任意电阻值自动映射成 MSPM0G3507 的硬件增益档。

## 什么时候用

- 题目要求信号反相，同时按指定倍数放大时。
- 已知输入电阻，希望计算所需反馈电阻时。
- 需要先估算增益和偏置，再去 SysConfig 选择实际可实现拓扑时。

反相放大输出极性相反：请求 `-5` 倍，表示输入 `+0.1 V` 的理想交流增量变为 `-0.5 V`。

## 输入与输出

公式为 `Rfeedback = abs(requested_gain) x Rinput`。

| 参数 | 单位/范围 | 含义 |
|---|---|---|
| `requested_gain` | 小于 0 | 目标反相增益，例如 `-5.0f` |
| `input_resistor_ohm` | 欧姆，必须大于 0 | 理想输入电阻 |
| `bias_voltage_v` | V，必须大于等于 0 | 设计偏置，仅写入 config |
| `config` | 输出 | 生成 `INVERTING` 软件配置 |
| `feedback_resistor_ohm` | 欧姆，输出 | 计算得到的反馈电阻 |

## SysConfig 配置（真实片上 OPA）

软件计算完成后，仍须在比赛工程的 `.syscfg` 配置 OPA：

1. `Add -> OPA`，选择当前芯片存在的 instance。
2. 在 `Basic Configuration` 的 `Mode/Topology` 选择 Inverting。
3. 在输入路由中核对正、负输入的来源；`PSEL/NSEL/MSEL` 可能来自外部 Pin、偏置源或片内节点。
4. 在 `Feedback/Routing` 选择内部电阻网络或外部反馈。GUI 的离散 gain 档不必等于函数算出的任意电阻值，选择最接近且满足题目误差的档位。
5. 需要观察或外接下游时，才在 PinMux 启用 Output Pin；若内部送 ADC，要在 ADC12 选择正确的 internal route。
6. Generate 后核对 `ti_msp_dl_config.h` 的实际 instance 和 Pin/IOMUX 宏，不直接编辑生成文件。

静态 OPA 不需要 OPA 专属时钟。ADC 的采样率、Timer、DMA 和 Event 只在 ADC 链路实际需要时配置。参见 [OPA/GPAMP 教程](../../00_docs/SYSCONFIG_BEGINNER_GUIDE.md#opa-gpamp)。

## 最小示例

```c
#include "signal_opa_inverting.h"

void opa_inverting_example(void)
{
    signal_opa_config_t config;
    float feedback_resistor_ohm = 0.0f;
    signal_result_t result = SignalOPAInverting_MakeConfig(
        -5.0f, 2000.0f, 1.65f, &config, &feedback_resistor_ohm);

    /* 成功时：feedback_resistor_ohm == 10000，config.mode 为 INVERTING。 */
    if (result != SIGNAL_RESULT_OK) {
        return;
    }
}
```

示例生成的是软件配置；下一步应在 SysConfig 选择实际的反相拓扑、路由与离散 gain，再验证真实输出。

## API

| API | 用途 |
|---|---|
| `SignalOPAInverting_MakeConfig` | 校验参数并计算反馈电阻、填充 config |
| `SignalOPAInverting_GetModuleStatus` | 返回软件模块状态 |

主函数在空指针、`requested_gain >= 0`、`input_resistor_ohm <= 0` 或负偏置时返回 `SIGNAL_RESULT_INVALID_ARGUMENT`。

最小和全 API 示例见 [`README_MINIMAL_EXAMPLE.c`](README_MINIMAL_EXAMPLE.c)、[`README_FULL_EXAMPLE.c`](README_FULL_EXAMPLE.c)，声明见 [`signal_opa_inverting.h`](signal_opa_inverting.h)。

## 模块链

```text
题目目标反相增益 -> MakeConfig -> 选择真实 OPA 拓扑/增益
    -> 检查输出范围 -> ADC 或后级电路
```

若输出送 ADC，使用 `opa_dac_bias` 做偏置预算，并用 `opa_to_adc` 检查最坏范围。

## 常见错误

- 传入 `+5`，而反相 API 必须传入 `-5`。
- 把算出的 `10 kOhm` 当成片上一定存在的内部电阻档。
- 忘记为反相输入提供合适偏置，导致单电源下信号贴地或饱和。
- 误以为 ADC 采样频率是 OPA 时钟。
- 只检查 ADC 范围，忽略 OPA 共模、摆幅、GBW 和压摆率。

## 资源与验证状态

- 不动态分配内存，执行为常数时间。
- 软件计算已纳入 PC 构建；当前没有可直接导入的、针对本开发板的 OPA Profile。
- `MODULE_STATUS_BUILD_VERIFIED` 不代表板级 OPA 已验证。
- 文档约定见 [BEGINNER_README_STANDARD.md](../../00_docs/BEGINNER_README_STANDARD.md)。
