# MAX14752：8:1 高压模拟多路复用器

README 类型：`EXACT_DEVICE_GUIDE`

验证状态：`DOC_VERIFIED`；只需直接 GPIO 代码，无独立 `.c/.h`，尚未上板。

官方资料：[ADI 产品页](https://www.analog.com/en/products/max14752.html) · [MAX14752/MAX14753 Datasheet](https://www.analog.com/media/en/technical-documentation/data-sheets/max14752-max14753.pdf)

> 安全警告：这是 ±10～±36 V 双电源或 +20～+72 V 单电源的高压模拟开关，不是普通 3.3/5 V 小信号 MUX。没有合格高压电源、限流、绝缘和上电检查时不要接线实验。

## 1. 它是什么

8 路模拟输入选 1 路到 OUT 的高压双向 MUX，典型 RON 约 60 Ω；S0/S1/S2 选择通道，EN 既控制使能也定义控制逻辑电平范围。

## 2. 为什么比赛可能用

题目涉及较高双极性模拟电压、多量程高压测量或工业信号切换时可能用。普通 0～3.3 V ADC 前端一般优先低压 MUX。

## 3. 供电

支持约 ±10～±36 V 双电源，或 +20～+72 V 单电源。VDD、VSS、GND 的供电结构必须完全按数据手册；先限流上电并测每条轨。

## 4. MSPM0 能否直接连接

选择输入的逻辑水平由 EN 决定。将 EN 由同一 3.3 V MSPM0 逻辑域可靠驱动时，可让 S0..S2 使用 3.3 V GPIO 逻辑；但高压模拟电源与 MCU 地的关系、绝对最大额定和故障路径必须由硬件负责人复核。

## 5. 需要接 MCU 的 Pin

TSSOP-16：S0(pin1)、EN(pin2)、S2(pin15)、S1(pin16) 接 GPIO。VSS(pin3)、IN0..IN3(pin4..7)、OUT(pin8)、IN7..IN4(pin9..12)、VDD(pin13)、GND(pin14) 属于电源/模拟链。

## 6. 接线表

| MAX14752 | MSPM0/高压电路 | SysConfig |
|---|---|---|
| S0/S1/S2 | GPIO output | GPIO |
| EN | GPIO output，初始低 | GPIO；低=禁用 |
| IN0..IN7/OUT | 高压模拟路径 | 无 |
| VDD/VSS/GND | 合规高压电源/系统地 | 无 |

## 7. SysConfig 一步一步配置

添加 S0、S1、S2、EN 四个 GPIO Output；EN 初始低，地址初始 000；保存并使用生成宏。无需 SPI/I2C。

## 8. 地址/真值表

EN 高时，`S2:S1:S0` 的二进制 000..111 选择 IN0..IN7；EN 低时关闭通道。

## 9. 关键寄存器

无。

## 10. Power-Up / Reset 与 Bring-Up 起点

先让 EN 低，再检查 VDD/VSS/GND 与所有模拟输入，设置地址，最后 EN 高。芯片无软件 reset。

## 11. 最小初始化

```c
SYSCFG_DL_init();
DL_GPIO_clearPins(GPIO_HVMUX_PORT,
                  GPIO_HVMUX_EN_PIN | GPIO_HVMUX_S0_PIN |
                  GPIO_HVMUX_S1_PIN | GPIO_HVMUX_S2_PIN);
```

## 12. 选择通道

### 【比赛现场直接复制】

```c
static void max14752_select(uint8_t ch)
{
    DL_GPIO_clearPins(GPIO_HVMUX_PORT, GPIO_HVMUX_EN_PIN);
    if (ch & 1U) DL_GPIO_setPins(GPIO_HVMUX_PORT, GPIO_HVMUX_S0_PIN);
    else DL_GPIO_clearPins(GPIO_HVMUX_PORT, GPIO_HVMUX_S0_PIN);
    if (ch & 2U) DL_GPIO_setPins(GPIO_HVMUX_PORT, GPIO_HVMUX_S1_PIN);
    else DL_GPIO_clearPins(GPIO_HVMUX_PORT, GPIO_HVMUX_S1_PIN);
    if (ch & 4U) DL_GPIO_setPins(GPIO_HVMUX_PORT, GPIO_HVMUX_S2_PIN);
    else DL_GPIO_clearPins(GPIO_HVMUX_PORT, GPIO_HVMUX_S2_PIN);
    DL_GPIO_setPins(GPIO_HVMUX_PORT, GPIO_HVMUX_EN_PIN);
}
```

## 13. 判断 Ready

无 ready。按 switching time 和后级 RC 留建立时间。

## 14. 读取状态

无数字回读；软件保存 channel，真实输出由安全的分压/隔离测量链验证。

## 15. 模拟传输

导通仍有约几十欧姆阻抗及漏电。MAX14752 不是降压器：高压输入被选择后，OUT 仍可能是高压，绝不能直接接 MSPM0 ADC。

## 16. main 完整例子

```c
#include <stdint.h>
#include "ti_msp_dl_config.h"
int main(void)
{
    SYSCFG_DL_init();
    max14752_select(3U);
    while (1) { }
}
```

## 17. 比赛最常改参数

channel、EN 安全策略、GPIO 宏、高压供电、输入范围、后级分压/保护、建立时间。

## 18. 常见错误

当成低压 MUX；OUT 直接进 MCU；EN 初始高；忽略高压爬电/限流；VDD/VSS 供电不满足最小值；模拟信号超电源轨；没有先测供电便连接 MCU。

## 19. Datasheet 关键章节

Pin Configuration、Recommended Operating Conditions、Logic Inputs/EN、Truth Table、Analog Signal Range、On Resistance/Leakage、Switching Characteristics、Power-Supply Sequencing and Layout。
