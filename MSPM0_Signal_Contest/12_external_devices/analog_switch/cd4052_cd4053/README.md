# CD4052B / CD4053B：GPIO 控制模拟多路开关

README 类型：`EXACT_DEVICE_GUIDE`

验证状态：`DOC_VERIFIED`；只需直接 GPIO 代码，无独立 `.c/.h`，尚未上板。

官方资料：[TI CD4052B 产品页](https://www.ti.com/product/CD4052B) · [TI CD4053B 产品页](https://www.ti.com/product/CD4053B) · [共同 Datasheet](https://www.ti.com/lit/ds/symlink/cd4053b.pdf)

## 1. 它是什么

CD4052B 是双路 4:1 双向模拟 MUX；CD4053B 是三组 2:1 双向模拟开关。GPIO 地址脚选择模拟通路，INH 禁用全部通道。

## 2. 为什么比赛可能用

用少量 GPIO 切换多路输入、量程电阻、滤波支路或信号路径；便宜通用，但导通电阻、带宽、串扰和切换毛刺不如专用高速开关。

## 3. 供电

可单电源或双电源。单电源时 VSS=VEE=GND、VDD 接正电源，模拟信号必须位于 VEE..VDD；双电源用于通过负信号。任何信号都不能在芯片掉电时随意灌入。

## 4. MSPM0 能否直接连接

当 CD405x 以 3.3 V 逻辑电源工作时通常可由 3.3 V GPIO 直接控制。若 VDD=5 V，经典 CMOS VIH 可能高于 MSPM0 的保证高电平，不能只因“实测偶尔能用”就视为可靠，应改用 3.3 V 供电、合适 HCT/电平转换或重新选型。

## 5. 需要接 MCU 的 Pin

CD4052：A(pin10)、B(pin9)、INH(pin6)。CD4053：A(pin11)、B(pin10)、C(pin9)、INH(pin6)。其余 X/Y/common 是双向模拟端。

## 6. 接线表

| 器件控制脚 | MSPM0 | SysConfig |
|---|---|---|
| A/B（4052） | GPIO output | GPIO |
| A/B/C（4053） | GPIO output | GPIO |
| INH | GPIO output，初始高 | GPIO；高=全断开 |
| VDD/VSS/VEE | 按模拟信号范围供电 | 无 |
| COM/通道脚 | 模拟链 | 无 |

CD4052 引脚：Y0/Y2/YCOM/Y3/Y1=1/2/3/4/5，X3/X0/XCOM/X1/X2=11/12/13/14/15，VEE/VSS/VDD=7/8/16。CD4053 详细模拟端以封装 pin table 为准。

## 7. SysConfig 一步一步配置

建立 3～4 个 GPIO Output，起名 MUX_A/B/C/INH。INH 初始高，其余初始低。保存后使用生成宏；不添加 SPI/I2C。

## 8. 地址/真值表

CD4052：`B:A=00/01/10/11` 选择每组的 0/1/2/3。CD4053：A、B、C 分别选择各自那一组的 X/Y。INH=1 时全部断开。

## 9. 关键寄存器

无寄存器。

## 10. Power-Up / Reset 与 Bring-Up 起点

上电优先让 INH 有上拉或 GPIO 初始高，避免 MCU 复位期间随机通道导通。设置地址后再拉低 INH。

## 11. 最小初始化

```c
SYSCFG_DL_init();
DL_GPIO_setPins(GPIO_MUX_PORT, GPIO_MUX_INH_PIN);
DL_GPIO_clearPins(GPIO_MUX_PORT, GPIO_MUX_A_PIN | GPIO_MUX_B_PIN);
```

## 12. 选择通道

### 【比赛现场直接复制】

```c
static void cd4052_select(uint8_t channel)
{
    DL_GPIO_setPins(GPIO_MUX_PORT, GPIO_MUX_INH_PIN);
    if (channel & 1U) DL_GPIO_setPins(GPIO_MUX_PORT, GPIO_MUX_A_PIN);
    else DL_GPIO_clearPins(GPIO_MUX_PORT, GPIO_MUX_A_PIN);
    if (channel & 2U) DL_GPIO_setPins(GPIO_MUX_PORT, GPIO_MUX_B_PIN);
    else DL_GPIO_clearPins(GPIO_MUX_PORT, GPIO_MUX_B_PIN);
    DL_GPIO_clearPins(GPIO_MUX_PORT, GPIO_MUX_INH_PIN);
}
```

## 13. 判断 Ready

无 ready。切换后按信号源阻抗、开关电阻和后级电容留模拟建立时间，再启动 ADC。

## 14. 读取状态

控制器件无状态回读。软件保存当前 channel；真实导通用 ADC/示波器验证。

## 15. 模拟输入到输出

开关近似一个随供电和信号电平变化的导通电阻，并有电容、漏电和串扰，不是理想导线。高源阻抗/高速信号误差更明显。

## 16. main 完整例子

```c
#include <stdint.h>
#include "ti_msp_dl_config.h"
int main(void)
{
    SYSCFG_DL_init();
    cd4052_select(2U);
    while (1) { }
}
```

## 17. 比赛最常改参数

channel、INH 极性流程、GPIO 宏、VDD/VEE、切换后 delay、源阻抗和允许信号范围。

## 18. 常见错误

5 V 供电却直接用 3.3 V GPIO；负模拟信号但 VEE=0；掉电仍给模拟输入；A/B 顺序反；切换后立即 ADC；忽略导通电阻造成增益误差。

## 19. Datasheet 关键章节

Pin Configuration、Recommended Operating Conditions、Control Truth Table、On Resistance、Analog Signal Range、Switching Characteristics、Crosstalk/Feedthrough and Application Information。
