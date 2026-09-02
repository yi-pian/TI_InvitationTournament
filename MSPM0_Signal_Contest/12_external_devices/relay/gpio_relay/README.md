# GPIO 控制继电器模块教程

README 类型：`GENERIC_TUTORIAL`  
状态：`DOCUMENTATION_ONLY / DATASHEET_REQUIRED`；不同模块的驱动和有效电平不同。

## 它是什么

继电器用电磁线圈控制隔离触点。MSPM0 只给驱动级一个逻辑信号，不能用 GPIO 直接给裸线圈供电。模块若带晶体管/MOS、续流二极管或光耦，接法仍要看真实原理图。

```text
GPIO → 驱动晶体管/光耦 → 线圈吸合 → COM 与 NO/NC 改变连接
```

## 拿到实物先确认

线圈/模块供电、电流、3.3 V 输入是否可靠、高/低有效、是否带驱动管和续流二极管、光耦是否真正隔离、上电默认状态、COM/NO/NC 触点额定电压电流，以及触点要切换的负载类型。

## MSPM0 / SysConfig / 接线

配置一个 `GPIO Output`，初始值设为“释放”状态。GPIO 接模块 IN 或外部驱动级输入；模块电源按规格供电；非隔离模块通常要共地。裸线圈必须有合适驱动与续流路径。

| 继电器端 | 作用 |
|---|---|
| IN | 来自 MSPM0 GPIO/驱动级 |
| VCC/GND | 模块线圈与逻辑供电，按实物确认 |
| COM | 触点公共端 |
| NO | 默认断开，吸合后接 COM |
| NC | 默认接 COM，吸合后断开 |

## 最小 Bring-Up

1. 触点先不接 DUT，模块使用限流电源。
2. MCU 上电时保持释放，确认没有误吸合。
3. 单次吸合，用万用表测 COM/NO/NC；再释放并复测。
4. 连续低频切换，观察 MCU 是否因线圈干扰复位。
5. 先切低压已知负载，最后才接正式负载。
6. 精密模拟信号要考虑触点电阻、弹跳、热电势和地回路。

## Generic main 框架

```c
#include <stdbool.h>
#include "ti_msp_dl_config.h"

void TODO_MODEL_SPECIFIC_RelaySet(bool energized);

int main(void)
{
    SYSCFG_DL_init();
    TODO_MODEL_SPECIFIC_RelaySet(false); /* 上电安全状态 */
    /* 人工确认接线后，才允许短暂吸合测试。 */
    while (1) { __WFI(); }
}
```

## 比赛最常改参数

GPIO 映射、有效电平、上电安全状态、吸合/释放等待时间、切换顺序和最大切换频率。

## 如果换成另一个继电器模块

只复用“安全默认 → 控制 → 等待触点稳定”的流程；重新确认供电、驱动、有效电平、触点额定值和隔离。参考 [GPIO Controlled Device Recipe](../../00_docs/recipes/GPIO_CONTROLLED_DEVICE_RECIPE.md)。
