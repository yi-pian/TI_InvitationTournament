# DAC7811：12-bit SPI 乘法型电流输出 DAC

README 类型：`EXACT_DEVICE_GUIDE / COMPILE_VERIFIED_DRIVER`

验证状态：`DOC_VERIFIED`、`CODE_COMPILE_VERIFIED`；尚未上板。

官方资料：[TI 产品页](https://www.ti.com/product/DAC7811) · [DAC7811 Datasheet](https://www.ti.com/lit/ds/symlink/dac7811.pdf)

## 1. 它是什么

DAC7811 是 12-bit、3-wire 串行输入的乘法型 DAC。它的 `IOUT1` 是电流输出，不是拿一根线就得到 0～3.3 V 的普通电压 DAC。

## 2. 为什么比赛可能用

外接运放和参考后，可做可编程增益、波形幅度、衰减或反相信号输出；SPI 最高速率高，适合快速更新。若只需要简单直流电压，普通电压输出 DAC 更省硬件。

## 3. 供电

数字电源 2.7～5.5 V，MSPM0 联调建议 3.3 V。还必须有参考输入 `VREF`，并按典型应用接 I/V 运放、`RFB`、`IOUT1/IOUT2`。电源和参考都要去耦。

## 4. MSPM0 能否直接连接

数字控制脚 SCLK、SDIN、SYNC 在 3.3 V 供电时可以直连。模拟输出不能直连负载或 ADC 当作电压：必须先完成数据手册推荐的 I/V 转换电路。

## 5. 需要接 MCU 的 Pin

`SCLK`、`SDIN`、`SYNC`。`SDO` 仅菊链时需要；本驱动初始化后禁用 daisy-chain，不连接 MCU。

## 6. 接线表

| DAC7811 | MSPM0/外部电路 | SysConfig |
|---|---|---|
| SCLK | SPI SCLK | SPI Controller |
| SDIN | SPI PICO/MOSI | SPI Controller |
| SYNC | GPIO output，初始高 | GPIO |
| SDO | 单芯片模式可不接 | 无 |
| VDD/GND | 3.3 V/GND | 无 |
| VREF | 精密参考或输入信号 | 无 |
| IOUT1/IOUT2/RFB | I/V 运放网络 | 无 |

## 7. SysConfig 一步一步配置

添加 SPI Controller；选择 SCLK/PICO；MSB first、8-bit；按数据手册时序使用空闲低、第一边沿采样的常用 Mode 0；初次 1 MHz；再添加初始高 GPIO `SYNC`。保存并使用生成宏，例如 `SPI_0_INST`、`GPIO_DAC_PORT`、`GPIO_DAC_SYNC_PIN`。

## 8. 地址/帧格式

无地址。每帧 16 bit，MSB first：高 4 bit 是 command，低 12 bit 是 data。传输期间 SYNC 保持低。

## 9. 关键命令

| 16-bit word | 用途 |
|---|---|
| `0x9000` | 禁用上电默认 daisy-chain，进入单芯片操作 |
| `0x1000 | code` | 写入并更新 DAC，code 取 0..4095 |
| `0xB000` | 清零输出 |

## 10. Power-Up / Reset 与 Bring-Up 起点

上电后先调用 `DAC7811_InitStandalone()`，否则器件上电默认菊链行为可能让第一条普通写命令不符合预期。需要安全输出时再调用 `DAC7811_ClearZero()`。

## 11. 最小初始化

加入 `dac7811.c/.h` 和 `00_common/mspm0_blocking_bus.c/.h`：

```c
SYSCFG_DL_init();
DL_GPIO_setPins(GPIO_DAC_PORT, GPIO_DAC_SYNC_PIN);
bool ok = DAC7811_InitStandalone(SPI_0_INST,
                                 GPIO_DAC_PORT,
                                 GPIO_DAC_SYNC_PIN);
```

## 12. 启动一次输出

```c
DAC7811_WriteCode(SPI_0_INST, GPIO_DAC_PORT,
                  GPIO_DAC_SYNC_PIN, 2048U);
```

写入即更新，不需要另一个 LDAC 脉冲。

## 13. 判断 Ready

没有 ready pin。阻塞 helper 等待 `DL_SPI_isBusy()` 为 false 后才抬高 SYNC。首次用示波器看 SYNC/SCLK/SDIN。

## 14. 写入 DAC 值

API 会把输入限制到低 12 bit。应用仍应先把参数限制在 0..4095，避免负数转换为无意的大码值。

## 15. code 变成电压

按数据手册典型反相 I/V 电路，理想关系常写为：

`VOUT = -VREF × code / 4096`

实际极性和幅度取决于 VREF、运放供电、反馈连接和负载。若要正向 0～VREF，需要再反相/偏置，不能靠改 code 消除硬件极性。

## 16. main 完整例子

### 【比赛现场直接复制】

```c
#include <stdbool.h>
#include "ti_msp_dl_config.h"
#include "dac7811.h"

volatile bool g_ok;

int main(void)
{
    SYSCFG_DL_init();
    DL_GPIO_setPins(GPIO_DAC_PORT, GPIO_DAC_SYNC_PIN);

    g_ok = DAC7811_InitStandalone(SPI_0_INST,
                                  GPIO_DAC_PORT,
                                  GPIO_DAC_SYNC_PIN);
    g_ok = g_ok && DAC7811_WriteCode(SPI_0_INST,
                                     GPIO_DAC_PORT,
                                     GPIO_DAC_SYNC_PIN,
                                     2048U);
    while (1) {
    }
}
```

## 17. 比赛最常改参数

code、VREF、SPI/SYNC 宏、SPI clock、运放与供电、需要的输出极性/幅度、静态更新或定时连续更新。

## 18. 常见错误

- 把 IOUT 当电压输出；
- 忘记 `0x9000` 退出 daisy-chain；
- 16-bit 字节顺序反；
- 运放输入/输出超出共模或摆幅；
- 只测 MCU 数字波形，不测 VREF 和 I/V 电路；
- 期望用 DAC7811 直接输出带直流偏置的正电压。

## 19. Datasheet 关键章节

Pin Configuration、Digital Interface、Serial Input Register、Daisy-Chain Operation、Analog Output、Multiplying Operation、Typical Applications、Power Supply/Reference and Layout。

## 20. 官方资料入口

- [TI DAC7811 Product Page](https://www.ti.com/product/DAC7811)
- [TI DAC7811 Datasheet](https://www.ti.com/lit/ds/symlink/dac7811.pdf)
- [TI DAC7811EVM](https://www.ti.com/tool/DAC7811EVM)
- [TI SLAU163：DAC7811EVM User Guide](https://www.ti.com/lit/ug/slau163/slau163.pdf)

EVM 资料用于理解参考、I/V 运放和板级测量，不代表仓库 MSPM0 Driver 已在 EVM 上验证。
