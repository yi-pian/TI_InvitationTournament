# PGA113AIDGSR：双通道 SPI 可编程增益放大器

README 类型：`EXACT_DEVICE_GUIDE / COMPILE_VERIFIED_DRIVER`

验证状态：`DOC_VERIFIED`、`CODE_COMPILE_VERIFIED`；尚未上板。

官方资料：[TI 产品页](https://www.ti.com/product/PGA113) · [PGA113 Datasheet](https://www.ti.com/lit/gpn/pga113)

## 1. 它是什么

PGA113 是双通道可编程增益放大器，通过 SPI 选择 CH0/CH1，并在 1、2、5、10、20、50、100、200 倍中选择增益。

## 2. 为什么比赛可能用

题目信号幅度跨度大时，可由 MCU 自动切换增益，充分利用 ADC 量程；也可在两路输入间切换。它是模拟前端，不是数字算法增益。

## 3. 供电

AVDD 和 DVDD 2.2～5.5 V。初次统一 3.3 V，并分别就近去耦。VREF 设置输出基准，单电源测交流时常接合适中点，不应随意悬空。

## 4. MSPM0 能否直接连接

DVDD=3.3 V 时，CS/SCLK/DIO 可直连 MSPM0。模拟输入、VREF、输出必须在 PGA113 的共模、摆幅和带宽限制内。

## 5. 需要接 MCU 的 Pin

DGS-10 引脚：SCLK(7)、DIO(8)、CS(9) 接 MCU；DVDD(10)、AVDD(1)、GND(6) 供电；CH1(2)、CH0/VCAL(3)、VREF(4)、VOUT(5) 属于模拟链。

## 6. 接线表

| PGA113 | MSPM0/模拟电路 | SysConfig |
|---|---|---|
| SCLK | SPI SCLK | SPI Controller |
| DIO | SPI PICO/MOSI | SPI Controller |
| CS | GPIO output，初始高 | GPIO |
| CH0、CH1 | 两路模拟输入 | 无 |
| VOUT | ADC/后级输入 | 无 |
| VREF | 0 V 或中点基准 | 无 |
| AVDD/DVDD/GND | 3.3 V/3.3 V/GND | 无 |

## 7. SysConfig 一步一步配置

添加 SPI Controller，SCLK+PICO、MSB first、8-bit；PGA113 支持时钟空闲低/高的两种对应模式，初学者先选 Mode 0（CPOL=0、CPHA=0），SCLK 1 MHz；另加初始高 GPIO CS。使用生成宏。

## 8. 地址/帧格式

无器件地址。16-bit 帧，MSB first：高字节 `0x2A`，低字节高半字节为 gain code，bit0 选 channel。

## 9. 关键命令

`0x2Axx` 设置增益/通道；`0xE1F1` shutdown；`0xE100` wakeup。驱动枚举把 gain 顺序映射为 1、2、5、10、20、50、100、200。

## 10. Power-Up / Reset 与 Bring-Up 起点

没有单独 RESET pin。上电后主动写一次需要的 gain/channel，不依赖未知外部状态。休眠后使用 `PGA113_Wakeup()`。

## 11. 最小初始化

加入 `pga113.c/.h` 和公共 blocking bus；`SYSCFG_DL_init()` 后 CS 拉高。

## 12. 启动/设置

```c
bool ok = PGA113_SetGainAndChannel(SPI_0_INST,
                                   GPIO_PGA_PORT,
                                   GPIO_PGA_CS_PIN,
                                   PGA113_GAIN_10,
                                   PGA113_CHANNEL_0);
```

## 13. 判断 Ready

无 ready/readback。阻塞写返回后仍要为模拟输出建立留时间；用 ADC 或示波器验证增益。

## 14. 读取/输出值

器件不返回数字测量值。输出是模拟 `VOUT`，后接 MSPM0 内部 ADC 或外置 ADC。

## 15. 输入变成输出

理想理解：`VOUT ≈ VREF + Gain × (VIN - VREF)`；实际还受共模范围、输出摆幅、带宽、偏置和负载影响。高增益时特别容易饱和。

## 16. main 完整例子

### 【比赛现场直接复制】

```c
#include <stdbool.h>
#include "ti_msp_dl_config.h"
#include "pga113.h"

volatile bool g_ok;

int main(void)
{
    SYSCFG_DL_init();
    DL_GPIO_setPins(GPIO_PGA_PORT, GPIO_PGA_CS_PIN);
    g_ok = PGA113_SetGainAndChannel(SPI_0_INST,
                                    GPIO_PGA_PORT,
                                    GPIO_PGA_CS_PIN,
                                    PGA113_GAIN_10,
                                    PGA113_CHANNEL_0);
    while (1) {
    }
}
```

## 17. 比赛最常改参数

gain、channel、VREF、允许输入范围、SPI/CS 宏、切换后的建立等待、ADC 满量程策略。

## 18. 常见错误

把增益枚举值当实际倍率数字；VREF 悬空；200 倍时输出饱和；模拟输入超出共模；AVDD/DVDD 未共地；DIO 错接到 POCI；CS 在两个字节之间抬高。

## 19. Datasheet 关键章节

Pin Configuration、Recommended Operating Conditions、Gain/Channel Selection、Serial Interface Timing、Input/Output Range、Bandwidth/Settling、Shutdown、Typical Application and Layout。

## 20. 官方资料入口

- [TI PGA113 Product Page](https://www.ti.com/product/PGA113)
- [TI PGA113 Datasheet](https://www.ti.com/lit/gpn/pga113)
- [TI PGA113EVM-B](https://www.ti.com/tool/PGA113EVM-B)
- [TI SBOU073A：PGA112EVM/PGA113EVM User Guide](https://www.ti.com/lit/ug/sbou073a/sbou073a.pdf)

EVM 原理图和测试流程可帮助核对 VREF、输入/输出范围与跳线；验证状态仍是 Compile，不是 Board。
