# CD4066B：四路独立双向模拟开关

README 类型：`EXACT_DEVICE_GUIDE`

验证状态：`DOC_VERIFIED`；只需直接 GPIO 代码，无独立 `.c/.h`，尚未上板。

官方资料：[TI 产品页](https://www.ti.com/product/CD4066B) · [CD4066B Datasheet](https://www.ti.com/lit/ds/symlink/cd4066b.pdf)

## 1. 它是什么

一颗芯片内有 4 个独立、双向的 SPST 模拟开关；对应 CONTROL 高时接通，低时断开。

## 2. 为什么比赛可能用

可切换电阻、反馈、滤波支路、信号输入或校准路径。每路独立，不需要二进制地址。

## 3. 供电

VDD(pin14) 和 VSS(pin7) 决定逻辑与模拟信号范围。单电源 3.3 V 时，模拟端必须留在约 0..3.3 V；需要负信号时必须按数据手册设计双电源/电平，不是简单把 GND 换成负电源就结束。

## 4. MSPM0 能否直接连接

3.3 V 供电时 GPIO 可直接控制。若 CD4066B 供电 5 V，MSPM0 3.3 V 高电平未必满足经典 CMOS VIH，需按 Electrical Characteristics 处理电平。

## 5. 需要接 MCU 的 Pin

CONTROL A/B/C/D 分别是 pin13/5/6/12。模拟端：A=1/2，B=3/4，C=8/9，D=10/11。

## 6. 接线表

| CD4066B | MSPM0/电路 | SysConfig |
|---|---|---|
| CONTROL A..D | 1～4 个 GPIO output，初始低 | GPIO |
| 每组 SIG IN/OUT | 被切换模拟节点 | 无 |
| VDD/VSS | 3.3 V/GND（初次） | 无 |

## 7. SysConfig 一步一步配置

需要几路就添加几个 GPIO Output，分别命名 SW_A..SW_D，初始低，保存后使用生成宏。无需通信外设。

## 8. 地址/真值表

没有地址。每个 CONTROL：0=该路断开，1=该路接通。

## 9. 关键寄存器

无。

## 10. Power-Up / Reset 与 Bring-Up 起点

控制脚加确定下拉或设 GPIO 初始低，避免 MCU 复位期间误接通信号。芯片无 reset。

## 11. 最小初始化

```c
SYSCFG_DL_init();
DL_GPIO_clearPins(GPIO_SW_PORT,
                  GPIO_SW_A_PIN | GPIO_SW_B_PIN |
                  GPIO_SW_C_PIN | GPIO_SW_D_PIN);
```

## 12. 控制开关

### 【比赛现场直接复制】

```c
DL_GPIO_setPins(GPIO_SW_PORT, GPIO_SW_A_PIN);   /* A 接通 */
DL_GPIO_clearPins(GPIO_SW_PORT, GPIO_SW_A_PIN);/* A 断开 */
```

## 13. 判断 Ready

无 ready。切换速度由数据手册 switching characteristics 决定；ADC 前要考虑 RC 建立。

## 14. 读取状态

无数字回读。软件保存控制位，模拟路径通过测量验证。

## 15. 模拟传输

导通时仍有 RON，且 RON 随供电和信号电平变化；断开时仍有漏电、电容和 feedthrough。不能当理想继电器。

## 16. main 完整例子

```c
#include "ti_msp_dl_config.h"
int main(void)
{
    SYSCFG_DL_init();
    DL_GPIO_setPins(GPIO_SW_PORT, GPIO_SW_A_PIN);
    while (1) { }
}
```

## 17. 比赛最常改参数

启用哪些路、初始安全状态、GPIO 宏、供电、允许模拟幅度、建立时间。

## 18. 常见错误

5 V CMOS 控制阈值不兼容；模拟信号越过电源轨；控制脚悬空；忽略导通电阻；多路并联时误同时接通；芯片掉电但信号仍存在。

## 19. Datasheet 关键章节

Pin Configuration、Recommended Operating Conditions、Control Input Requirements、On-State Resistance、Switching Characteristics、Crosstalk/Feedthrough、Power Supply and Applications。
