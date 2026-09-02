# X9C104S：100-kΩ 三线非易失数字电位器

README 类型：`EXACT_DEVICE_GUIDE`

验证状态：`DOC_VERIFIED`；本器件使用 README 中的直接 DriverLib GPIO 代码，没有独立 `.c/.h`，尚未上板。

官方资料：[Renesas 产品页](https://www.renesas.com/en/products/x9c104?partno=X9C104SIZ) · [X9C102/103/104/503 Datasheet](https://www.renesas.com/en/document/dst/x9c102-x9c103-x9c104-x9c503-datasheet?r=502676)

## 1. 它是什么

100-kΩ、100 个 tap 的非易失数字电位器，用 `CS + U/D + INC` 三根数字线逐步移动滑动端，可在掉电时保存位置。

## 2. 为什么比赛可能用

可以替代需要 MCU 调节的机械电位器，控制增益、阈值或偏置；接口不用 SPI/I2C。但它只能一步步走，不能读回绝对位置，不适合高速连续调制。

## 3. 供电

X9C104S 使用 5 V 电源（按数据手册允许范围设计）。VH/VL/VW 的电压、电流和功耗都要满足手册；滑动端电流不可超限。

## 4. MSPM0 能否直接连接

该系列 5 V 供电时数字高门限最低值允许 MSPM0 3.3 V 高电平驱动，三个输入脚可由 MSPM0 GPIO 控制；仍需共地。若使用的模块板另加了上拉/反相电路，要以模块原理图为准。

## 5. 需要接 MCU 的 Pin

DIP/SOIC-8 功能：INC(1)、U/D(2)、CS(7) 接 GPIO；VH(3)、VSS(4)、VW(5)、VL(6)、VCC(8) 接模拟网络与电源。

## 6. 接线表

| X9C104 | MSPM0/电路 | SysConfig |
|---|---|---|
| INC | GPIO output，初始高 | GPIO |
| U/D | GPIO output | GPIO |
| CS | GPIO output，初始高 | GPIO |
| VCC/VSS | 5 V/GND | 无 |
| VH/VL | 电阻串两端 | 无 |
| VW | 滑动端 | 无 |

## 7. SysConfig 一步一步配置

添加三个 GPIO Output，分别命名 X9C_CS、X9C_INC、X9C_UD；CS 和 INC 初始高；保存后使用生成 `GPIO_X9C_PORT` 与 pin 宏。无需 SPI/I2C。

## 8. 地址/三线时序

没有数字地址。CS 低选中；U/D 高向一个端点移动、低向另一端；INC 的下降沿移动一步。U/D 方向与 VH/VL 端定义要在实物上用万用表确认。

## 9. 关键寄存器

没有可读写寄存器。内部计数器保存当前位置，但 MCU 不能读回。

## 10. Power-Up / Reset 与 Bring-Up 起点

上电恢复上次保存的 tap。没有 reset 命令。需要已知绝对位置时，可向端点方向走至少 99 步使其饱和，再反向走到目标；先确认端点电流安全。

## 11. 最小初始化

```c
#define CPUCLK_HZ (32000000UL)
static void delay_us(uint32_t us)
{
    DL_Common_delayCycles((CPUCLK_HZ / 1000000UL) * us);
}
```

`CPUCLK_HZ` 必须和你的 SysConfig 系统时钟一致。

## 12. 移动滑动端

### 【比赛现场直接复制】

```c
static void x9c_move(uint8_t steps, bool up, bool store)
{
    if (up) DL_GPIO_setPins(GPIO_X9C_PORT, GPIO_X9C_UD_PIN);
    else    DL_GPIO_clearPins(GPIO_X9C_PORT, GPIO_X9C_UD_PIN);

    delay_us(3U); /* U/D setup，保守大于手册要求 */
    DL_GPIO_clearPins(GPIO_X9C_PORT, GPIO_X9C_CS_PIN);

    for (uint8_t i = 0U; i < steps; ++i) {
        DL_GPIO_clearPins(GPIO_X9C_PORT, GPIO_X9C_INC_PIN);
        delay_us(2U);

        if (!store && (i + 1U == steps)) {
            /* INC 低时抬高 CS：退出但不写 NVM。 */
            DL_GPIO_setPins(GPIO_X9C_PORT, GPIO_X9C_CS_PIN);
            DL_GPIO_setPins(GPIO_X9C_PORT, GPIO_X9C_INC_PIN);
            return;
        }

        DL_GPIO_setPins(GPIO_X9C_PORT, GPIO_X9C_INC_PIN);
        delay_us(100U); /* 等待 wiper change */
    }

    /* INC 高时抬高 CS：保存当前位置。 */
    DL_GPIO_setPins(GPIO_X9C_PORT, GPIO_X9C_CS_PIN);
    if (store) delay_us(20000U);
}
```

连续调节时用 `store=false`，最终确需掉电保留才 `store=true`，避免每步都写非易失存储。

## 13. 判断 Ready

无 ready pin。普通移动保守等待约 100 µs；存储时等待数据手册规定的写周期（示例用 20 ms）。

## 14. 读取位置

不能读回。应用只能维护软件 shadow position；若 MCU 重启且不知道上次保存值，先回端点建立参考。

## 15. step 与阻值

X9C104 标称端到端 100 kΩ、100 tap，但包含端点容差和 wiper resistance。不要把 `position × 100k/99` 当精密标定；最终用电路输出反标。

## 16. main 完整例子

```c
#include <stdbool.h>
#include <stdint.h>
#include "ti_msp_dl_config.h"

int main(void)
{
    SYSCFG_DL_init();
    DL_GPIO_setPins(GPIO_X9C_PORT,
                    GPIO_X9C_CS_PIN | GPIO_X9C_INC_PIN);
    x9c_move(10U, true, false);
    while (1) { }
}
```

## 17. 比赛最常改参数

steps、方向、是否保存、CPU clock/delay、VH/VL/VW 接法、端点归零策略和标定表。

## 18. 常见错误

把它当 SPI；INC 上升沿计数；每走一步都保存；不给存储时间；期望读回位置；VH/VL/VW 超过允许电压或电流；软件位置与实际位置失步。

## 19. Datasheet 关键章节

Pin Descriptions、Recommended Operating Conditions、AC Timing、Principles of Operation、Wiper Movement、Store/Recall、Equivalent Circuit、Endurance and Wiper Current。

## 20. 官方资料入口

- [Renesas X9C104 Product Page](https://www.renesas.com/en/products/x9c104?partno=X9C104SIZ)
- [Renesas X9C102/X9C103/X9C104/X9C503 Datasheet](https://www.renesas.com/en/document/dst/x9c102-x9c103-x9c104-x9c503-datasheet?r=502676)

本目录给的是直接 DriverLib GPIO 示例，没有独立 `.c/.h`，所以不标 `COMPILE_VERIFIED_DRIVER`，也没有 Board 验证。
