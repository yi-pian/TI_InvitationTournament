# TCA6408A：I2C 8-bit GPIO 扩展器

README 类型：`EXACT_DEVICE_GUIDE / COMPILE_VERIFIED_DRIVER`

验证状态：`DOC_VERIFIED`、`CODE_COMPILE_VERIFIED`；尚未上板。

官方资料：[TI 产品页](https://www.ti.com/product/TCA6408A) · [TCA6408A Datasheet](https://www.ti.com/lit/ds/symlink/tca6408a.pdf)

## 1. 它是什么

通过 I2C 扩展 8 个 GPIO 的器件，带独立接口电源 VCCI、I/O 电源 VCCP、INT 和 RESET。

## 2. 为什么比赛可能用

MSPM0 管脚不够时，可控制多路模拟开关、LED、继电器使能或读取慢速按键。它不适合精确定时、PWM 或高速并行采样。

## 3. 供电

VCCI 和 VCCP 均支持约 1.65～5.5 V。最简单两者均接 3.3 V；需要和另一电压域接口时才分开，并按手册核对 I/O 电平和电流。

## 4. MSPM0 能否直接连接

VCCI=3.3 V、I2C 上拉至 3.3 V 时可直连。P0..P7 的电平由 VCCP 决定；若 VCCP=5 V，不代表这些口可无条件接回 MSPM0。

## 5. 需要接 MCU 的 Pin

SDA、SCL；可选 `/INT` 输入、`/RESET` 输出。ADDR 通常硬接低或高。P0..P7 接被控制器件。

## 6. 接线表

| TCA6408A | MSPM0/电路 | SysConfig |
|---|---|---|
| SDA/SCL | I2C SDA/SCL | I2C Controller |
| /INT | 可选 GPIO Input | 可加下降沿中断 |
| /RESET | GPIO Output，初始高 | GPIO |
| ADDR | GND 或 VCCI | 无 |
| VCCI/VCCP/GND | 3.3 V/3.3 V/GND | 无 |
| P0..P7 | 外部数字信号 | 无 |

## 7. SysConfig 一步一步配置

添加 I2C Controller、SDA/SCL、100 kHz、外部 4.7 kΩ 上拉；若使用 RESET 加初始高 GPIO；若使用 INT 加上拉和 GPIO input，第一次先轮询。使用生成的真实宏。

## 8. I2C 地址

ADDR 低：7-bit `0x20`；ADDR 高：`0x21`。驱动提供 `TCA6408A_ADDRESS_ADDR_LOW/HIGH`。

## 9. 关键寄存器

| 地址 | 寄存器 | 复位值 | 说明 |
|---|---|---|---|
| 0x00 | Input | 实际输入 | 只读 |
| 0x01 | Output | 0xFF | 输出锁存值 |
| 0x02 | Polarity | 0x00 | 1 表示输入反相 |
| 0x03 | Configuration | 0xFF | 1=input，0=output |

方向位最容易写反：`0` 才是输出。

## 10. Power-Up / Reset 与 Bring-Up 起点

上电默认全输入。把 `/RESET` 拉低会回默认；正常保持高。不要在设置方向前假设端口已经是输出。

## 11. 最小初始化

加入 `tca6408a.c/.h` 和公共 blocking bus。例：低 4 位输出、高 4 位输入：

```c
bool ok = TCA6408A_SetDirection(I2C_0_INST, 0x20U, 0xF0U);
```

建议先写 Output 锁存值，再把相应位改为输出，以避免短暂错误电平。

## 12. 启动一次输出

```c
TCA6408A_WritePins(I2C_0_INST, 0x20U, 0x05U);
```

这个函数写整个 8-bit output register。若只改一位，应在应用中维护 shadow byte，修改后整字节写回，避免破坏其他位。

## 13. 判断输入变化

最简单轮询 `TCA6408A_ReadPins()`。需要事件通知时接 `/INT`；读取输入寄存器后按手册处理清除条件。

## 14. 读取 GPIO

```c
uint8_t pins;
bool ok = TCA6408A_ReadPins(I2C_0_INST, 0x20U, &pins);
```

## 15. raw 变成逻辑状态

```c
bool p6_high = (pins & (1U << 6)) != 0U;
```

若 Polarity register 对应位为 1，读取逻辑已经被硬件反相。

## 16. main 完整例子

### 【比赛现场直接复制】

```c
#include <stdbool.h>
#include <stdint.h>
#include "ti_msp_dl_config.h"
#include "tca6408a.h"

#define IOX_ADDR  (TCA6408A_ADDRESS_ADDR_LOW)
volatile bool g_ok;
volatile uint8_t g_inputs;

int main(void)
{
    SYSCFG_DL_init();
    g_ok = TCA6408A_WritePins(I2C_0_INST, IOX_ADDR, 0x00U);
    g_ok = g_ok && TCA6408A_SetDirection(I2C_0_INST,
                                         IOX_ADDR, 0xF0U);
    g_ok = g_ok && TCA6408A_WritePins(I2C_0_INST,
                                      IOX_ADDR, 0x05U);
    g_ok = g_ok && TCA6408A_ReadPins(I2C_0_INST,
                                     IOX_ADDR, (uint8_t *)&g_inputs);
    while (1) {
    }
}
```

## 17. 比赛最常改参数

ADDR、direction mask、output shadow、polarity、VCCP、I/O 上拉、INT/RESET 是否使用。

## 18. 常见错误

把 1 当输出；整字节写破坏其他输出；ADDR 接法和代码不一致；I2C 无上拉；VCCI/VCCP 混淆；直接驱动超出每口/总电流的负载；拿 I2C GPIO 做精确 PWM。

## 19. Datasheet 关键章节

Pin Configuration、Power Supplies VCCI/VCCP、I2C Interface/Slave Address、Register Map、Interrupt Output、Reset、I/O Port、Electrical Characteristics。

## 20. 官方资料入口

- [TI TCA6408A Product Page](https://www.ti.com/product/TCA6408A)
- [TI TCA6408A Datasheet](https://www.ti.com/lit/ds/symlink/tca6408a.pdf)

Product Page 还提供 GPIO Expander EVM 入口；本仓库未在该 EVM 或自制板上进行硬件验证。
