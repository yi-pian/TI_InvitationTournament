# 可编程模拟滤波器控制通用教程

README 类型：`GENERIC_TUTORIAL`  
状态：`DOCUMENTATION_ONLY / DATASHEET_REQUIRED`；这是控制架构模板，不是一个实体 Driver。

## 它是什么

比赛中的“可编程滤波器”往往是模拟滤波电路加上继电器、模拟开关、数字电位器、PGA、外置 DAC 或专用滤波芯片。MCU 选择一个档位，硬件才改变截止频率、Q 或增益。

```text
目标 type/fc/Q/gain → 查硬件档位表 → GPIO/SPI/I2C/DAC 控制 → 等待稳定 → 测量验证
```

## 先建立硬件档位表

| 档位 | 目标 type/fc/Q/gain | GPIO/总线控制码 | 安全输入范围 | 稳定时间 | 实测结果 |
|---|---|---|---|---|---|
| 0 | TODO | TODO | TODO | TODO | TODO |

这个表来自真实原理图、元件值和扫频实测，不能由通用 README 填假数。

## MSPM0 / SysConfig / 接线

按控制器件选择最少外设：GPIO 开关用 `GPIO Output`，数字电位器/专用芯片用 I2C 或 SPI，模拟控制电压用内部/外部 DAC。上电默认状态必须是不会短路、不会过增益的安全档。模拟输入输出不直接接普通 GPIO。

## 最小 Bring-Up

1. 输入固定小幅正弦，示波器同时看输入/输出。
2. 只启用一个安全档位，确认通带点和阻带点方向正确。
3. 禁用/静音后切到第二档，等待稳定，再测两点。
4. 对每档扫频，记录幅度和相位，填入实测表。
5. 确认切换毛刺和饱和恢复时间后，才允许自动切档。

## Generic main 框架

```c
#include <stdbool.h>
#include <stdint.h>
#include "ti_msp_dl_config.h"

bool TODO_HARDWARE_TABLE_ApplyFilterSlot(uint8_t slot);
void TODO_MODEL_SPECIFIC_WaitSettling(void);

int main(void)
{
    SYSCFG_DL_init();
    if (!TODO_HARDWARE_TABLE_ApplyFilterSlot(0U)) {
        __BKPT(0);
    }
    TODO_MODEL_SPECIFIC_WaitSettling();
    /* 再启动 ADC 测量，不能在切换瞬间采结果。 */
    while (1) { __WFI(); }
}
```

## 比赛最常改参数

档位表、控制码、目标截止频率/Q/增益、切换顺序、settling time、安全输入范围和校准结果。

## 如果换成另一种可编程滤波器

可复用“逻辑档位 → 硬件控制码 → 等待 → 实测验证”的上层结构；必须重新从原理图建立档位表。按底层接口选择 [GPIO](../../00_docs/recipes/GPIO_CONTROLLED_DEVICE_RECIPE.md)、[SPI](../../00_docs/recipes/SPI_REGISTER_DEVICE_RECIPE.md)、[I2C](../../00_docs/recipes/I2C_REGISTER_DEVICE_RECIPE.md) 或 [模拟电压控制](../../00_docs/recipes/ANALOG_VOLTAGE_CONTROLLED_DEVICE_RECIPE.md) Recipe。

