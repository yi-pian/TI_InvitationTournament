# EC11 类机械旋转编码器教程

README 类型：`GENERIC_TUTORIAL`  
状态：`DOCUMENTATION_ONLY / DATASHEET_REQUIRED`；本目录没有正式 Driver。

## 它是什么

EC11 类旋钮转动时输出 A/B 两路相位错开的机械触点信号，按下旋钮还可能有独立 SW 按键。通过 A/B 状态变化顺序判断方向，通过有效步数调整菜单参数。

```text
旋转 → A/B Gray 状态序列 → 去抖/解码 → +1 或 -1
按下 → SW 去抖 → 确认/切换功能
```

## 常见引脚与接线

| 功能 | MSPM0 | 说明 |
|---|---|---|
| A | GPIO input | 上拉或下拉按真实接法 |
| B | GPIO input | 与 A 同一逻辑电平体系 |
| C/COM | GND 或规定公共端 | 先用万用表确认 |
| SW | GPIO input | 独立按键，需要去抖 |

模块板可能自带上拉/RC；裸编码器通常没有。不要未确认就同时打开内部上下拉。

## MSPM0 / SysConfig

第一次配置 A、B、SW 为 GPIO input，先轮询。需要响应更快时再给 A/B 配置 GPIO 边沿中断；ISR 只记录状态/时间，不在中断里做菜单和屏幕刷新。

## 最小 Bring-Up

1. 万用表找公共端、A、B、SW。
2. 每约 1 ms 读取并打印 A/B 原始状态。
3. 慢慢转一格，记录完整状态序列和每格产生的边沿数。
4. 加状态表解码，非法跳变丢弃或计错。
5. 加时间/状态去抖，再处理 SW。
6. 快速正反转测试丢步，最后才进入 GPIO interrupt。

## Generic main 框架

```c
#include <stdint.h>
#include "ti_msp_dl_config.h"

int8_t TODO_MODEL_SPECIFIC_EC11_PollStep(void); /* -1 / 0 / +1 */
bool TODO_MODEL_SPECIFIC_EC11_ButtonPressed(void);

int main(void)
{
    int32_t value = 0;
    SYSCFG_DL_init();
    while (1) {
        value += TODO_MODEL_SPECIFIC_EC11_PollStep();
        if (TODO_MODEL_SPECIFIC_EC11_ButtonPressed()) {
            /* 确认当前 value；不要在中断里刷新整屏。 */
        }
        DL_Common_delayCycles(TODO_MODEL_SPECIFIC_POLL_DELAY_CYCLES);
    }
}
```

## 比赛最常改参数

上拉方向、每格计数、轮询周期、去抖时间、加速规则、数值上下限和 SW 功能。

## 如果换成另一个机械编码器

可复用 A/B 状态机；必须重新测每格脉冲、公共端、是否有 SW、模块 RC 和触点时序。GPIO 配置流程见 [GPIO Controlled Device Recipe](../../00_docs/recipes/GPIO_CONTROLLED_DEVICE_RECIPE.md)。

