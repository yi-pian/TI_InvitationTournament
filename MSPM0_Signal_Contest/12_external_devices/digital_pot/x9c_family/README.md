# X9C 类三线数字电位器通用教程

README 类型：`GENERIC_TUTORIAL`  
状态：`DOCUMENTATION_ONLY / DATASHEET_REQUIRED`；具体 X9C104 教程在独立目录。

## 它是什么

X9C 类器件不是 SPI。MCU 用 `CS` 选中器件，用 `U/D` 选上调或下调，再给 `INC` 脉冲让滑动端逐步移动。有些成员可把位置保存到非易失存储。

```text
选择方向 U/D → CS 选中 → INC 有效边沿 × N → 停止/按规则保存 → W 改变
```

## 常见信号

| 信号 | 通常作用 | 必须查的差异 |
|---|---|---|
| CS | 选中器件，也可能参与保存 | 有效电平和保存时序 |
| U/D | 增/减方向 | 建立保持时间 |
| INC | 每个有效边沿走一步 | 有效边沿、脉宽和最高频率 |
| VH/VL/VW | 电阻梯高端、低端、滑动端 | 允许电压和电流 |

## MSPM0 / SysConfig / 接线

只需三个 `GPIO Output`；不需要把它假装成 SPI。上电时先给 CS/INC 一个不会误步进或误保存的安全电平，然后调用 `SYSCFG_DL_init()`。

## 最小 Bring-Up

1. 查完整料号的供电、端电压、总阻值、tap 数和保存条件。
2. A/W/B 先组成安全低压分压，不接高增益链。
3. 设置 U/D，按官方有效边沿给一个 INC 脉冲。
4. 测 W 是否朝预期方向变化，再连续走 5 步。
5. 找到端点/已知位置后，才把“走 N 步”与目标位置关联。
6. NVM 保存只在必要时执行，避免无意义反复写入。

## Generic main 框架

```c
#include <stdbool.h>
#include <stdint.h>
#include "ti_msp_dl_config.h"

bool TODO_MODEL_SPECIFIC_X9C_Step(bool upward, uint16_t steps,
                                  bool store_after_move);

int main(void)
{
    SYSCFG_DL_init();
    if (!TODO_MODEL_SPECIFIC_X9C_Step(true, 1U, false)) {
        __BKPT(0);
    }
    while (1) { __WFI(); }
}
```

“走 N 步”不是绝对位置，除非你已经可靠归零、记录状态或有外部测量反馈。

## 比赛最常改参数

方向、步数、GPIO 脉冲延时、是否保存、位置到阻值/增益的实测表。

## 如果换成另一个 X9C 型号

可复用 CS/U-D/INC 的控制思想；必须重查总阻值、tap 数、供电、端电压、NVM 规则、有效边沿和时序。明确型号请直接看 [X9C104 Exact Device Guide](../x9c104/README.md) 与 [三线 GPIO 器件 Recipe](../../00_docs/recipes/THREE_WIRE_GPIO_DEVICE_RECIPE.md)。

