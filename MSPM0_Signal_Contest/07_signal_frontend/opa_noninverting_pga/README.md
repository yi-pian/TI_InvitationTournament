# OPA 同相放大 / PGA 参数设计

这个模块按理想同相放大器公式计算反馈电阻，并生成软件配置对象。它适合规划“放大但不反相”的前端，不能直接替代 SysConfig 对 OPA MUX、离散 gain 和 Pin 的实际配置。

## 什么时候用

- 传感器或输入信号太小，需要放大但不希望相位翻转时。
- 已选择接地电阻，希望计算理想反馈电阻时。
- 需要在比赛中先确定增益和偏置，再选择片上 OPA/PGA 可实现档位时。

同相放大最小增益为 1。典型用途是把 `0.1 V` 信号放大到约 `1 V` 后送 ADC。

## 输入与输出

公式为 `Rfeedback = (requested_gain - 1) x Rground`。

| 参数 | 单位/范围 | 含义 |
|---|---|---|
| `requested_gain` | 大于等于 1 | 目标同相增益 |
| `resistor_to_ground_ohm` | 欧姆，必须大于 0 | 理想接地电阻 |
| `bias_voltage_v` | V，必须大于等于 0 | 设计偏置，仅写入 config |
| `config` | 输出 | 生成 `NONINVERTING` 软件配置 |
| `feedback_resistor_ohm` | 欧姆，输出 | 计算得到的反馈电阻 |

## SysConfig 配置（真实片上 OPA）

1. 打开比赛工程的 `.syscfg`，执行 `Add -> OPA`。
2. 在 `Mode/Topology` 选择 Non-Inverting 或 PGA。
3. 在输入路由中选择正输入和参考/负输入，检查 `PSEL/NSEL/MSEL` 对应的是外部 Pin 还是内部节点。
4. 在 Gain/Feedback 中选择 GUI 实际提供的内部网络或外部反馈方式。函数算出的电阻只用于设计核对，不是硬件配置值。
5. 根据接线选择 Input/Output Pin；如果输出直接进入 ADC，优先确认是否有 internal connection，避免无意义的外部跳线。
6. 保存 Generate，检查 OPA instance、Pin/IOMUX 宏和资源冲突。

SDK 例子中的 PA18 输入、PA16 输出只能用来理解流程，最终必须以开发板原理图和当前 SysConfig 的合法选项为准。详见 [OPA/GPAMP 教程](../../00_docs/SYSCONFIG_BEGINNER_GUIDE.md#opa-gpamp)。

## 最小示例

```c
#include "signal_opa_noninverting_pga.h"

void opa_noninverting_example(void)
{
    signal_opa_config_t config;
    float feedback_resistor_ohm = 0.0f;
    signal_result_t result = SignalOPANoninvertingPGA_MakeConfig(
        11.0f, 1000.0f, 1.65f, &config, &feedback_resistor_ohm);

    /* 成功时：反馈电阻为 10000 Ohm，理想增益为 11。 */
    if (result != SIGNAL_RESULT_OK) {
        return;
    }
}
```

## API

| API | 用途 |
|---|---|
| `SignalOPANoninvertingPGA_MakeConfig` | 校验参数并计算反馈电阻、填充 config |
| `SignalOPANoninvertingPGA_GetModuleStatus` | 返回软件模块状态 |

主函数在空指针、`requested_gain < 1`、接地电阻不为正或偏置为负时返回 `SIGNAL_RESULT_INVALID_ARGUMENT`。

可查看 [`README_MINIMAL_EXAMPLE.c`](README_MINIMAL_EXAMPLE.c)、[`README_FULL_EXAMPLE.c`](README_FULL_EXAMPLE.c) 和 [`signal_opa_noninverting_pga.h`](signal_opa_noninverting_pga.h)。

## 模块链

```text
目标输入幅度/ADC 量程 -> MakeConfig -> SysConfig 选择实际 PGA 档位
    -> OPA 输出范围检查 -> ADC 采样 -> 电压还原
```

交流信号有负半周时，先用 `opa_dac_bias` 计算偏置后的范围，再决定增益。

## 常见错误

- 传入小于 1 的增益；同相放大不支持这个目标。
- 忘记“增益 11”对应 `Rfeedback = 10 x Rground`，不是 11 倍。
- 把理想电阻值直接填到片上 OPA，未检查 GUI 是否有相应的离散档。
- 选了外部 Output Pin 又在 ADC 中选内部 route，造成链路与接线不一致。
- 增益过大导致输出摆幅超出 ADC 或 OPA 允许范围。

## 资源与验证状态

- 模块不动态分配内存，执行为常数时间。
- 软件逻辑已纳入 PC 构建；没有针对当前开发板固定 Pin 的实板 Profile。
- `MODULE_STATUS_BUILD_VERIFIED` 不等于已完成 OPA 实板验证。
- 文档约定见 [BEGINNER_README_STANDARD.md](../../00_docs/BEGINNER_README_STANDARD.md)。
