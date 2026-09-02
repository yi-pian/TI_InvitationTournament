# CD4051 / 74HC4051 类 8:1 模拟多路开关教程

README 类型：`GENERIC_TUTORIAL`  
状态：`DOCUMENTATION_ONLY / DATASHEET_REQUIRED`；“4051”不是完整料号。

## 它是什么

4051 类器件用三根地址线从 8 路模拟端中选择一路连接到公共端。它可以做量程切换、输入复用、反馈网络选择。模拟开关不是 ADC：它只改变模拟通路，电压仍需送进 ADC 或后级电路。

```text
S2:S1:S0 选择通道 → EN 允许 → Xn 与 COM 接通 → 等待稳定 → ADC/后级使用
```

CD4051B、74HC4051 和不同厂商版本的供电、逻辑阈值、模拟范围、导通电阻、带宽和保护能力可能不同，不能混用一个 datasheet。

## 常见信号和接线

| 功能 | MSPM0/模拟电路 | 必须确认 |
|---|---|---|
| S0/S1/S2 | 3 个 GPIO output | 真值表和逻辑电平 |
| EN/INH | GPIO output | 高/低有效 |
| COM/Z | 公共模拟节点 | 信号方向和允许范围 |
| X0...X7 | 8 路模拟节点 | 不得超出电源轨/器件规定 |
| VCC/VEE/GND | 电源 | 单/双电源接法和去耦 |

SysConfig 配置 4 个 GPIO output，初始化时先置于禁用状态。Pin 号由实际板卡决定。

## 拿到实物先确认

完整厂商料号、封装 Pinout、供电范围、3.3 V 逻辑兼容性、模拟信号允许范围、EN 真值、break-before-make、Ron/flatness、带宽、漏电、charge injection 和外部保护。

## 最小 Bring-Up

1. COM 和各 Xn 只接安全低压已知 DC，不先接负压或高压。
2. 上电保持 EN 禁用；用万用表确认无意外通路。
3. 使能后从通道 0 到 7 逐个切换。
4. 用万用表/示波器确认只有目标通道接通。
5. 再输入低幅正弦，检查带宽、串扰和切换毛刺。
6. 正式采样时在切换后留足 settling time，再启动 ADC。

## Generic main 框架

```c
#include <stdbool.h>
#include <stdint.h>
#include "ti_msp_dl_config.h"

void TODO_MODEL_SPECIFIC_4051_Enable(bool enable);
bool TODO_MODEL_SPECIFIC_4051_Select(uint8_t channel);

int main(void)
{
    SYSCFG_DL_init();
    TODO_MODEL_SPECIFIC_4051_Enable(false);
    if (!TODO_MODEL_SPECIFIC_4051_Select(0U)) {
        __BKPT(0);
    }
    TODO_MODEL_SPECIFIC_4051_Enable(true);
    /* 等待具体电路所需稳定时间后再采 ADC。 */
    while (1) { __WFI(); }
}
```

## 比赛最常改参数

通道号、EN 极性、切换后稳定时间、GPIO 映射，以及“通道号 → 量程/输入”的表。

## 如果换成另一个同类开关

可复用“禁用 → 写地址 → 使能 → 等待稳定”的结构；必须重查真值表、供电、模拟范围、Ron、带宽和 Pinout。明确器件可看 [CD4052/CD4053](../cd4052_cd4053/README.md)、[CD4066B](../cd4066b/README.md)、[MAX14752](../max14752/README.md)；GPIO 控制流程见 [GPIO Controlled Device Recipe](../../00_docs/recipes/GPIO_CONTROLLED_DEVICE_RECIPE.md)。
