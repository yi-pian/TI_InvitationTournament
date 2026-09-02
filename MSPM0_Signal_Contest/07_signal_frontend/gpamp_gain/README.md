# GPAMP 增益参数设计

这个模块保存目标 GPAMP 增益和偏置，便于比赛方案做参数预检。它不会将任意浮点增益映射为 MSPM0G3507 的实际 GPAMP 配置。

## 什么时候用

- 已有目标增益和偏置，需要统一保存、校验这些软件参数时。
- 在决定使用 GPAMP 前，需要先评估输入范围和输出量程时。
- 要真正改变 GPAMP 增益时，必须去 SysConfig 选择当前器件实际提供的 gain/feedback 选项。

## 输入与输出

`SignalGPAMPGain_MakeConfig(requested_gain, bias_voltage_v, config)` 要求：

- `requested_gain > 0`；
- `bias_voltage_v >= 0`；
- `config` 非空。

成功后把两个参数写入 `signal_gpamp_config_t`。函数不检查带宽、共模、输出摆幅、增益档或稳定性。

## SysConfig 配置（真实 GPAMP）

1. 在比赛工程 `.syscfg` 添加 `GPAMP`。
2. 根据原理图选择输入、反馈、偏置和输出路径。
3. 在 GUI 中选择真实可用的 gain 或外部反馈方案，不能直接填任意 `requested_gain`。
4. 若接 ADC，确认 ADC 输入是 internal route 还是外部 Pin，并配置参考、采样时间和触发。
5. 根据输入频率决定是否可用 GPAMP，必要时改用 OPA/外置运放；再 Generate 并检查资源冲突。

详细路线见 [OPA/GPAMP 教程](../../00_docs/SYSCONFIG_BEGINNER_GUIDE.md#opa-gpamp)。

## 最小示例

```c
#include "signal_gpamp_gain.h"

void gpamp_gain_example(void)
{
    signal_gpamp_config_t config;
    signal_result_t result = SignalGPAMPGain_MakeConfig(
        4.0f, 1.65f, &config);
    if (result != SIGNAL_RESULT_OK) {
        return;
    }
    /* 这里只保存目标 4 倍，不代表硬件已选择 4 倍档。 */
}
```

## API

| API | 用途 |
|---|---|
| `SignalGPAMPGain_MakeConfig` | 校验并保存目标 gain/bias |
| `SignalGPAMPGain_GetModuleStatus` | 返回软件模块状态 |

## 模块链

```text
题目目标增益/偏置 -> MakeConfig -> SysConfig 选实际 GPAMP 档位
    -> 输出范围检查 -> ADC/后级
```

## 常见错误

- 认为传入 4.0 就一定配置出硬件 4 倍。
- 忽略 GPAMP 带宽和压摆率，导致高频信号失真。
- 只检查 gain，不检查偏置后最坏输出范围。
- 修改外设 Pin 或 ADC 通道后，未重新 Generate。

## 资源与验证状态

- 不动态分配内存，执行为常数时间。
- 当前 API 缺少硬件 MUX、chopping、RRI 和输出配置，硬件集成状态为 `API_GAP`。
- 文档约定见 [BEGINNER_README_STANDARD.md](../../00_docs/BEGINNER_README_STANDARD.md)。
