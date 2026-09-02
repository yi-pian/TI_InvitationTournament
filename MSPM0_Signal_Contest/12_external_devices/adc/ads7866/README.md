# ADS7866：12-bit / 200-kSPS SPI SAR ADC

README 类型：`EXACT_DEVICE_GUIDE / COMPILE_VERIFIED_DRIVER`

验证状态：`DOC_VERIFIED`、`CODE_COMPILE_VERIFIED`；尚未上板。

官方资料：[TI 产品页](https://www.ti.com/product/ADS7866) · [ADS7866 Datasheet](https://www.ti.com/lit/ds/symlink/ads7866.pdf)

## 1. 它是什么

单通道 12-bit SAR ADC，最高约 200 kSPS，使用 CS、SCLK、SDO 的 16-clock 串行帧；REF/VDD 同时是供电和参考。

## 2. 为什么比赛可能用

比 ADS112C04 快，适合中低速波形；比 ADS7887 多 2 bit 分辨率，但最高采样率更低。接口简单且无流水线延迟。

## 3. 供电

器件可在约 1.2～3.6 V 范围工作；要达到 200 kSPS，按数据手册使用满足吞吐要求的电压。MSPM0 最简单是 3.3 V。输入范围 0～REF/VDD。

## 4. MSPM0 能否直接连接

3.3 V 供电时可以。REF/VDD 必须稳定、就近去耦，输入不能超过其范围。

## 5. 需要接 MCU 的 Pin

SOT-23-6：CS(6)、SDO(5)、SCLK(4) 接 MCU；VIN(3)、GND(2)、REF/VDD(1) 接模拟信号和供电。

## 6. 接线表

| ADS7866 | MSPM0 功能 | SysConfig |
|---|---|---|
| CS | GPIO output，初始高 | GPIO |
| SCLK | SPI SCLK | SPI Controller |
| SDO | SPI POCI/MISO | SPI Controller |
| VIN | 0..REF/VDD | 不需要 |
| REF/VDD、GND | 3.3 V、GND | 不需要 |

## 7. SysConfig 一步一步配置

添加 SPI Controller；MSB first、8-bit；SCLK idle high，MCU 在上升沿取样（标准 SPI 命名通常为 Mode 3）；初次 SCLK 1 MHz；再添加初始高 GPIO CS。使用逻辑分析仪确认 SDO 在下降沿更新、MCU 在相反边沿采样。

## 8. 地址/帧格式

无地址。CS 下降开始采样，16 个时钟返回 4 个 leading zero + 12-bit result，随后 SDO 三态。

## 9. 关键寄存器

无配置寄存器。

## 10. Power-Up / Reset 与 Bring-Up 起点

无软件复位。CS 高时自动进入采集/低功耗状态；首次采样前确保供电、参考和输入已经稳定。

## 11. 最小初始化

加入 `ads7866.c/.h` 及 `00_common/mspm0_blocking_bus.c/.h`；`SYSCFG_DL_init()` 后 CS 置高。

## 12. 启动转换

调用读取函数时拉低 CS 即启动；SCLK 同时是转换时钟。

## 13. 判断数据 Ready

没有 DRDY。阻塞读取完整 16-clock 后完成；高速采集由 Timer/DMA 调度帧。

## 14. 读取 ADC 值

```c
uint16_t raw;
bool ok = ADS7866_ReadRaw(SPI_0_INST, GPIO_ADC_PORT,
                          GPIO_ADC_CS_PIN, &raw);
```

驱动保留 `frame & 0x0FFF`。

## 15. raw code 变成电压

```c
float v = ADS7866_RawToVoltage(raw, 3.3f);
```

公式 `raw × VDD / 4096`；VDD 也是参考，建议传实测值。

## 16. main 完整例子

### 【比赛现场直接复制】

```c
#include <stdbool.h>
#include <stdint.h>
#include "ti_msp_dl_config.h"
#include "ads7866.h"

volatile uint16_t g_raw;
volatile float g_voltage;

int main(void)
{
    SYSCFG_DL_init();
    DL_GPIO_setPins(GPIO_ADC_PORT, GPIO_ADC_CS_PIN);
    if (ADS7866_ReadRaw(SPI_0_INST, GPIO_ADC_PORT,
                        GPIO_ADC_CS_PIN, (uint16_t *)&g_raw)) {
        g_voltage = ADS7866_RawToVoltage(g_raw, 3.3f);
    }
    while (1) {
    }
}
```

## 17. 比赛最常改参数

SCLK、精确 Fs、SPI/CS 宏、参考实测值、输入驱动/RC、blocking 或 DMA。

## 18. 常见错误

SPI 边沿错导致整体移位；少发时钟只读到 leading zero；把数据再右移 4 位；供电太低却要求 200 kSPS；输入源阻抗太高导致采样电容未稳定。

## 19. Datasheet 关键章节

Pin Configuration、Recommended Operating Conditions、Timing Characteristics、Serial Interface/Data Format、Typical Characteristics、Analog Input/Reference and Layout。

陌生器件迁移见 [SPI SAR ADC Migration Guide](../../00_docs/SPI_SAR_ADC_MIGRATION_GUIDE.md)。

## 20. 官方资料入口

- [TI ADS7866 Product Page](https://www.ti.com/product/ADS7866)
- [TI ADS7866 Datasheet](https://www.ti.com/lit/ds/symlink/ads7866.pdf)
- [TI SLAA308：ADS786x 与 MSP430F2013 接口应用报告](https://www.ti.com/lit/an/slaa308/slaa308.pdf)

应用报告可用于理解三线串行时序和 Bring-Up 思路，但其中 MCU、Pin 和工程配置不能原样当成 MSPM0G3507 的 SysConfig。
