# TPL0401A-10：I2C 10-kΩ 数字电位器

README 类型：`EXACT_DEVICE_GUIDE / COMPILE_VERIFIED_DRIVER`

验证状态：`DOC_VERIFIED`、`CODE_COMPILE_VERIFIED`；尚未上板。

官方资料：[TI 产品页](https://www.ti.com/product/TPL0401A-10) · [TPL0401A-10 Datasheet](https://www.ti.com/lit/ds/symlink/tpl0401a-10.pdf)

## 1. 它是什么

单通道 10-kΩ、128 个位置的易失性数字电位器，通过 I2C 改变 H 与 W 之间的有效阻值；L 端在芯片内部接 GND。

## 2. 为什么比赛可能用

可用 MCU 调整放大器增益、比较阈值、偏置或低速模拟参数。它不能替代功率电位器，也不能处理超出电源轨的模拟电压。

## 3. 供电

VDD 2.7～5.5 V，初次 3.3 V。上电后等待至少约 120 µs 再通信。H/W 模拟端电压必须遵守数据手册范围。

## 4. MSPM0 能否直接连接

VDD=3.3 V、SDA/SCL 上拉到 3.3 V 时可直连。裸芯片 SDA/SCL 需要外部上拉。

## 5. 需要接 MCU 的 Pin

SCL(3)、SDA(4)；SC70-6 其余为 VDD(1)、GND(2)、W(5)、H(6)。L 已内部接地。

## 6. 接线表

| TPL0401A | MSPM0/电路 | SysConfig |
|---|---|---|
| SCL | I2C SCL | I2C Controller |
| SDA | I2C SDA | I2C Controller |
| VDD/GND | 3.3 V/GND | 无 |
| H | 电阻串高端 | 无 |
| W | 滑动端/可变输出 | 无 |

## 7. SysConfig 一步一步配置

添加 I2C Controller，选 SDA/SCL，100 kHz，外接约 4.7 kΩ 上拉到 3.3 V；初次不启用 DMA/interrupt；保存并使用真实 `I2C_x_INST`。

## 8. I2C 地址

TPL0401A-10 的 7-bit 地址 `0x2E`；TPL0401B-10 为 `0x3E`。传给 DriverLib 时不要左移。

## 9. 关键寄存器

唯一 wiper 寄存器地址 `0x00`，低 7 bit 是 0..127。上电默认约 `0x40`；器件是易失性的，掉电后不会保留上次值。

## 10. Power-Up / Reset 与 Bring-Up 起点

没有 MCU reset 命令。等待供电稳定和上电时间，然后主动写需要的位置。

## 11. 最小初始化

加入 `tpl0401a_10.c/.h` 与公共 blocking bus。`SYSCFG_DL_init()` 后直接写中点：

```c
bool ok = TPL0401_SetWiper(I2C_0_INST,
                           TPL0401A_10_I2C_ADDRESS, 64U);
```

## 12. 启动一次设置

调用 `TPL0401_SetWiper()`。输入会与 `0x7F` 相与，应用最好显式限制 0..127。

## 13. 判断 Ready

无 DRDY。I2C ACK 表示写入被接收；模拟端还需留建立时间。需要确认时读回 wiper。

## 14. 读取位置

```c
uint8_t position;
bool ok = TPL0401_GetWiper(I2C_0_INST,
                           TPL0401A_10_I2C_ADDRESS, &position);
```

## 15. code 与阻值

code 越大，W 越接近 H；端到滑动端的实际阻值还包含 wiper resistance 和器件容差。精密增益不要只靠理想 `10k × code/127`，应在最终电路中标定。

## 16. main 完整例子

### 【比赛现场直接复制】

```c
#include <stdbool.h>
#include <stdint.h>
#include "ti_msp_dl_config.h"
#include "tpl0401a_10.h"

volatile bool g_ok;
volatile uint8_t g_position;

int main(void)
{
    SYSCFG_DL_init();
    for (volatile uint32_t i = 0; i < 10000U; ++i) { }

    g_ok = TPL0401_SetWiper(I2C_0_INST,
                            TPL0401A_10_I2C_ADDRESS, 64U);
    g_ok = g_ok && TPL0401_GetWiper(I2C_0_INST,
                                    TPL0401A_10_I2C_ADDRESS,
                                    (uint8_t *)&g_position);
    while (1) {
    }
}
```

正式工程用已验证的微秒延时替换示例空循环。

## 17. 比赛最常改参数

器件 A/B 地址、position、SDA/SCL 管脚、I2C clock、模拟 H/W 接法、标定表。

## 18. 常见错误

把它当三端任意浮动电位器（L 已内部接 GND）；代码传 8-bit 地址；无上拉；期望掉电保存；H/W 超过电源轨；忽略 wiper 电阻、10-kΩ 容差和允许电流。

## 19. Datasheet 关键章节

Pin Configuration、Recommended Operating Conditions、I2C Interface、Slave Address、Register Map/Wiper Register、Power-Up、Digital Potentiometer Operation、Typical Applications。

## 20. 官方资料入口

- [TI TPL0401A-10 Product Page](https://www.ti.com/product/TPL0401A-10)
- [TI TPL0401A-10 Datasheet](https://www.ti.com/lit/ds/symlink/tpl0401a-10.pdf)

产品页列出的评估软件/资料用于核对器件行为；仓库 Driver 仍未经过实物电位器验证。
