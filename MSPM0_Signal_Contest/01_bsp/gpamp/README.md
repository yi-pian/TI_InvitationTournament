# GPAMP 基础 BSP 接口

GPAMP 是片上通用放大器。这个目录提供增益/偏置的软件参数校验和 callback 适配接口，不直接初始化 MSPM0G3507 GPAMP 硬件。

## 什么时候用

- 低频传感器、慢变直流或需要低频精度前端时，先做 GPAMP 参数预算。
- 需要验证目标 gain 与 bias 参数是否为正/非负时。
- 要真正使用 GPAMP 时，应在 SysConfig 配置；不要把通过软件校验当成硬件已经启用。

对需要较高频率或较大增益的链路，应先比较 GPAMP 的带宽/压摆率与 OPA 或外置运放，而不是默认使用 GPAMP。

## 输入与输出

```c
typedef struct {
    float requested_gain;
    float bias_voltage_v;
} signal_gpamp_config_t;
```

`SignalGPAMP_ValidateConfig()` 要求 `requested_gain > 0` 且 `bias_voltage_v >= 0`。这只验证软件参数，不验证增益是否是当前芯片实际支持的离散档位。

## SysConfig 配置（真实 GPAMP）

1. 打开比赛工程 `.syscfg`，`Add -> GPAMP`。
2. 选择输入、反馈/增益、偏置、输出路由，并确认是否需要 rail-to-rail 或 chopping。
3. 需要连接 ADC 时，确认是内部连接还是 GPAMP 输出 Pin 再进 ADC；两种链路不能混用。
4. 保存 Generate，查看生成的 instance/Pin 宏和资源冲突；不要编辑生成文件。

GPAMP 的 chopping、带宽、稳定性和 ADC 建立时间必须按题目频率与实测确认。详细路线见 [OPA/GPAMP 教程](../../00_docs/SYSCONFIG_BEGINNER_GUIDE.md#opa-gpamp)。

## 最小示例

```c
#include "signal_gpamp.h"

void gpamp_validate_example(void)
{
    const signal_gpamp_config_t config = {
        .requested_gain = 2.0f,
        .bias_voltage_v = 1.65f
    };
    signal_result_t result = SignalGPAMP_ValidateConfig(&config);
    /* result 为 OK 不代表真实 GPAMP 已配置。 */
    (void)result;
}
```

## API

| API | 用途 |
|---|---|
| `SignalGPAMP_ValidateConfig` | 校验 gain 和 bias 参数 |
| `SignalGPAMP_Apply` | 校验后调用应用提供的 callback |
| `SignalGPAMP_GetModuleStatus` | 返回软件模块状态 |

`SignalGPAMP_Apply()` 需要非空的 `gpamp`、`gpamp->apply` 和合法 config。当前仓库没有正式的 MSPM0G3507 callback adapter。

## 模块链

```text
题目目标 gain/bias -> ValidateConfig -> SysConfig 配置真实 GPAMP
    -> 输出量程检查 -> ADC 或后级
```

## 常见错误

- 任意 `requested_gain` 通过 Validate 后，就认为芯片一定支持该增益。
- 不检查 chopping 影响、带宽和 ADC 建立时间。
- 在应用层临时编写 callback 绕开真实硬件配置缺口。
- 将 GPAMP 当作高频、高增益 OPA 使用。

## 资源与验证状态

- 不动态分配内存，参数校验为常数时间。
- 软件源码已构建验证；硬件集成状态为 `API_GAP`，没有当前开发板可直接导入的 GPAMP Profile。
- 文档约定见 [BEGINNER_README_STANDARD.md](../../00_docs/BEGINNER_README_STANDARD.md)。
