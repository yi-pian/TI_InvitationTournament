# 通用 SPI 数字电位器教程

README 类型：`GENERIC_TUTORIAL`  
状态：`DOCUMENTATION_ONLY / DATASHEET_REQUIRED`；没有统一 SPI 命令集。

## 它是什么

数字电位器用数字命令移动内部电阻梯的滑动端 `W`。它能设置增益、阈值、偏置或衰减，但 A/W/B 是模拟端，不是普通数字 Pin。

```text
目标档位/阻值 → SPI 命令 → 滑动端位置 → 模拟电路增益或偏置改变
```

## 常见功能与危险点

| 项目 | 作用 | 必须确认 |
|---|---|---|
| A、W、B | 电阻梯两端和滑动端 | 端电压范围、滑动端电流、是否允许负压 |
| SCLK、SDI | 写命令/位置 | SPI mode、帧格式 |
| CS | 选中/保存 | 有效边沿、NVM 保存条件 |
| SDO（若有） | 回读/级联 | 是否存在、回读格式 |
| RESET/SHDN | 恢复或断开 | 上电位置和有效电平 |

MCU 使用 3.3 V 不代表 A/W/B 可以接任意正负信号。超过端电压或滑动端电流限制可能损坏器件。

## 拿到实物先确认

完整型号、总阻值、tap 数、供电与数字 I/O、A/W/B 允许电压、wiper 电流、带宽、噪声、易失/非易失、上电位置、写入寿命、SPI mode 和命令格式。

## MSPM0 / SysConfig / 接线

配置低速 `SPI Controller` 和 CS GPIO；按真实器件再添加 RESET/SHDN。A/W/B 接模拟网络，不接 SPI。

| 器件 | MSPM0/电路 |
|---|---|
| SCLK | SPI SCLK |
| SDI | SPI MOSI |
| SDO | SPI MISO（若有） |
| CS | GPIO output |
| A/W/B | 安全的小信号分压或目标模拟电路 |

## 最小 Bring-Up

1. 不接高增益闭环，先用 A/B 构成安全低压分压器。
2. 低速 blocking SPI，写最小、中间、最大档位。
3. 用万用表测 W，确认方向、单调性和端点余量。
4. 若有回读，核对写入值；若有 NVM，先确认写寿命再测试保存。
5. 验证安全后才接入放大器/滤波器。

## Generic main 框架

```c
#include <stdbool.h>
#include <stdint.h>
#include "ti_msp_dl_config.h"

bool TODO_MODEL_SPECIFIC_Digipot_Init(void);
bool TODO_MODEL_SPECIFIC_Digipot_WritePosition(uint16_t position);

int main(void)
{
    SYSCFG_DL_init();
    if (!TODO_MODEL_SPECIFIC_Digipot_Init() ||
        !TODO_MODEL_SPECIFIC_Digipot_WritePosition(
            TODO_MODEL_SPECIFIC_MID_POSITION)) {
        __BKPT(0);
    }
    while (1) { __WFI(); }
}
```

## 比赛最常改参数

总阻值、位置/tap、方向、上电默认值、是否保存 NVM、SPI clock，以及位置到增益/截止频率的校准表。

## 如果换成另一个数字电位器

可复用“目标位置 → 总线写入 → 实测模拟结果”的结构。必须重查端电压、电流、总阻值、tap 数、易失性、命令和上电状态。I2C 具体示例见 [TPL0401A-10](../tpl0401a_10/README.md)，三线脉冲型见 [X9C104](../x9c104/README.md)，总线步骤见 [SPI 寄存器器件 Recipe](../../00_docs/recipes/SPI_REGISTER_DEVICE_RECIPE.md)。

