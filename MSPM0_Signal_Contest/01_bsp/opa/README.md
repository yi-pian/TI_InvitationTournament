# OPA 基础 BSP 接口

这个目录提供 OPA 的**软件模型和平台适配接口**。它能帮你计算 buffer、同相放大和反相放大的理想增益，也能把配置交给一个由应用提供的 callback；它本身不会写 MSPM0G3507 的 OPA 寄存器。

## 什么时候用

- 需要先算放大倍数、检查电阻参数或给比赛方案做量程预算时使用。
- 需要把“配置对象”交给你自己的平台层时使用。
- 只是要在 CCS/SysConfig 里真正打开片上 OPA 时，不要只复制本目录；硬件配置要直接在 `.syscfg` 完成。

典型比赛链路如下：

```text
输入信号 -> OPA（buffer/同相/反相）-> ADC -> 测量或 DSP
```

## 它能算什么

`signal_opa_config_t` 中的电阻单位是欧姆，`bias_voltage_v` 单位是伏特：

| 模式 | 理想增益 |
|---|---:|
| `SIGNAL_OPA_MODE_BUFFER` | `1` |
| `SIGNAL_OPA_MODE_NONINVERTING` | `1 + Rfeedback / Rinput` |
| `SIGNAL_OPA_MODE_INVERTING` | `-Rfeedback / Rinput` |

这只是理想电阻公式，不会自动选择芯片的离散增益、输入 MUX、输出 Pin、GBW 或 chopping。

## SysConfig 配置（真实片上 OPA）

本模块不新增 SysConfig 页面。比赛工程应打开自己的 `.syscfg`，按下面顺序配置：

1. `Add -> OPA`，选择当前器件实际存在的 OPA instance。
2. 在 `Basic Configuration` 选择 Buffer、Non-Inverting 或 Inverting topology。
3. 配置 `PSEL/NSEL/MSEL`：逐项判断输入来自外部 Pin 还是片内节点。
4. 选择 GUI 中实际存在的离散 gain/feedback 档位，不要把上表计算出的任意电阻值当作枚举值。
5. 在 `PinMux Peripheral and Pin Configuration` 决定是否启用输入、输出 Pin。
6. 如果 OPA 输出直接进入 ADC，在 ADC12 中选择器件提供的 internal connection；如果走外部 Pin，必须按原理图接线。
7. 保存并 Generate，检查生成的 `ti_msp_dl_config.h/.c` 和资源冲突视图；不要手改生成文件。

详细路线见 [OPA/GPAMP SysConfig 入门教程](../../00_docs/SYSCONFIG_BEGINNER_GUIDE.md#opa-gpamp)，现场速查见 [SYSCONFIG_QUICK_REFERENCE.md](../../00_docs/SYSCONFIG_QUICK_REFERENCE.md)。教程中的 PA18/PA16 是 SDK 示例 Pin，不是本工程固定 Pin。

## 软件使用

### 1. 只计算增益（当前推荐用法）

```c
#include "signal_opa.h"

void opa_gain_example(void)
{
    const signal_opa_config_t config = {
        .mode = SIGNAL_OPA_MODE_NONINVERTING,
        .resistor_feedback_ohm = 9000.0f,
        .resistor_input_ohm = 1000.0f,
        .bias_voltage_v = 1.65f
    };
    float gain = 0.0f;
    signal_result_t result = SignalOPA_CalculateGain(&config, &gain);
    /* result == SIGNAL_RESULT_OK，gain == 10.0 */
    (void)result;
}
```

`BUFFER` 模式返回 `1.0`；同相和反相模式要求 `Rinput > 0`、`Rfeedback >= 0`。空指针或未知模式会返回错误码。

### 2. 交给平台 callback

`SignalOPA_Apply()` 会先调用 `SignalOPA_CalculateGain()` 做参数检查，随后调用：

```c
typedef signal_result_t (*signal_opa_apply_fn)(void *context,
    const signal_opa_config_t *config);
```

callback 必须由平台层实现。当前仓库没有把这个 callback 映射到 MSPM0G3507 `DL_OPA_Config` 的正式 adapter，因此不要在比赛应用里临时猜寄存器或复制旧 callback。

## API

| API | 用途 | 关键返回 |
|---|---|---|
| `SignalOPA_CalculateGain` | 计算理想增益 | 参数正确返回 `SIGNAL_RESULT_OK` |
| `SignalOPA_Apply` | 校验后调用平台 callback | callback 的返回值原样传出 |
| `SignalOPA_GetModuleStatus` | 查询软件模块状态 | 当前为 `MODULE_STATUS_BUILD_VERIFIED` |

完整声明见 [`signal_opa.h`](signal_opa.h)。

## 模块链

```text
题目指标/电阻选择 -> SignalOPA_CalculateGain
    -> SysConfig 配置真实 OPA -> ADC 采样 -> 测量或 DSP
```

不要把 `gain` 计算结果直接当成 ADC 已配置完成；还要检查实际 OPA 输出范围、输入共模范围、GBW、压摆率和负载。

## 常见错误

- 把 `bias_voltage_v` 当成硬件已经加上的偏置；当前 API 只保存它，不产生 DAC 电压。
- 用任意 `Rfeedback/Rinput` 推断 MSPM0 的离散 gain 档位。
- 把 SDK 示例 Pin 直接复制到比赛工程，未核对开发板原理图。
- OPA 输出和 ADC 都接同一外部 Pin，却忘记检查模拟节点和 PinMux 冲突。
- 只看到 `MODULE_STATUS_BUILD_VERIFIED` 就以为已经完成实板验证。

## 资源与验证状态

- 模块内不动态分配内存；计算是同步、确定性的。
- `signal_opa.c` 已纳入 PC 构建，但当前没有经过本开发板验证的 OPA SysConfig Profile。
- 当前硬件集成状态：`API_GAP`。正式说明见 [MODULE_INTEGRATION_GAPS.md](../../00_docs/MODULE_INTEGRATION_GAPS.md)。
- 新手文档约定见 [BEGINNER_README_STANDARD.md](../../00_docs/BEGINNER_README_STANDARD.md)。

在没有正式 platform adapter/Profile 之前，本目录适合做软件预算和参数验证，不适合作为比赛现场直接启用片上 OPA 的唯一入口。
