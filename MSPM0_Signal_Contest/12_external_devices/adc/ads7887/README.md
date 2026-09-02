# ADS7887：10-bit / 1.25-MSPS SPI SAR ADC

README 类型：`EXACT_DEVICE_GUIDE / COMPILE_VERIFIED_DRIVER`

验证状态：`DOC_VERIFIED`、`CODE_COMPILE_VERIFIED`；尚未上板。

官方资料：[TI 产品页](https://www.ti.com/product/ADS7887) · [ADS7887 Datasheet](https://www.ti.com/lit/ds/symlink/ads7887.pdf)

## 1. 它是什么

单通道、10-bit、最高 1.25 MSPS 的 SAR ADC。CS 下降沿采样，随后 16 个 SCLK 同步完成转换和串行输出。

## 2. 为什么比赛可能用

它比低速 ΔΣ ADC 更适合采波形、测频和 FFT；接口只有 CS、SCLK、SDO，首次联调简单。代价是 10-bit 分辨率不高，输入驱动和参考/电源质量会直接影响高速性能。

## 3. 供电

VDD 2.35～5.25 V；最简单是 3.3 V。输入范围为 0～VDD，VDD 同时参与满量程，因此要稳定并就近去耦。

## 4. MSPM0 能否直接连接

VDD=3.3 V 时可以直接连接。若 VDD=5 V，先检查 SDO 电平是否对 MSPM0 安全，不建议直接冒险连接。

## 5. 需要接 MCU 的 Pin

`CS`、`SCLK`、`SDO`。没有配置寄存器，也不需要 MOSI；MCU 发送 dummy byte 只是为了产生时钟。

## 6. 接线表

| ADS7887 | MSPM0 功能 | SysConfig |
|---|---|---|
| CS | GPIO output，初始高 | GPIO |
| SCLK | SPI SCLK | SPI Controller |
| SDO | SPI POCI/MISO | SPI Controller |
| VIN | 被测信号 0..VDD | 不需要 |
| VDD/GND | 3.3 V/GND | 不需要 |

## 7. SysConfig 一步一步配置

1. 添加 SPI Controller；2. 选择 SCLK 和 POCI；3. MSB first、8-bit；4. 按时序图配置空闲高、在上升沿采样（标准命名通常为 Mode 3），并用逻辑分析仪确认；5. SCLK 先 1 MHz，稳定后再提高，但不超过 25 MHz；6. 添加初始高的 GPIO CS；7. 使用生成的真实宏。

## 8. 地址/帧格式

没有地址和寄存器。每次 CS 拉低产生 16-clock 帧：4 个 leading zero、10-bit data、2 个 trailing zero。

## 9. 关键寄存器

无。真正要改的是 SPI mode、SCLK 和 16-bit 数据截位。

## 10. Power-Up / Reset 与 Bring-Up 起点

无复位命令。CS 高时器件进入采集/省电阶段；保证供电稳定、CS 初始高。

## 11. 最小初始化

加入 `ads7887.c/.h` 和 `00_common/mspm0_blocking_bus.c/.h`，调用 `SYSCFG_DL_init()`，再把 CS 拉高。

## 12. 启动转换

CS 下降沿采样并开始一次转换。`ADS7887_ReadRaw()` 内部自动控制 CS 和两个 SPI 字节。

## 13. 判断数据 Ready

没有 DRDY。完整发送 16 个 SCLK 后数据帧结束；阻塞 DriverLib 调用返回即完成。连续高速版要由 Timer/DMA 的帧节奏保证。

## 14. 读取 ADC 值

```c
uint16_t raw;
bool ok = ADS7887_ReadRaw(SPI_0_INST, GPIO_ADC_PORT,
                          GPIO_ADC_CS_PIN, &raw);
```

驱动执行 `(frame >> 2) & 0x03FF`。注意 ADS7887 是 10-bit，不是 12-bit。

## 15. raw code 变成电压

```c
float v = ADS7887_RawToVoltage(raw, 3.3f);
```

公式是 `raw × VDD / 1024`。用万用表实测 VDD 可降低比例误差。

## 16. main 完整例子

### 【比赛现场直接复制】

```c
#include <stdbool.h>
#include <stdint.h>
#include "ti_msp_dl_config.h"
#include "ads7887.h"

volatile uint16_t g_raw;
volatile float g_voltage;
volatile bool g_ok;

int main(void)
{
    SYSCFG_DL_init();
    DL_GPIO_setPins(GPIO_ADC_PORT, GPIO_ADC_CS_PIN);

    g_ok = ADS7887_ReadRaw(SPI_0_INST, GPIO_ADC_PORT,
                           GPIO_ADC_CS_PIN, (uint16_t *)&g_raw);
    if (g_ok) {
        g_voltage = ADS7887_RawToVoltage(g_raw, 3.3f);
    }
    while (1) {
    }
}
```

## 17. 比赛最常改参数

SCLK、CS/SPI 生成宏、实测 VDD、采样触发频率、阻塞或 DMA backend、输入 RC/驱动放大器。

## 18. 常见错误

- 读数全零/错位：SPI mode 或 POCI 接错；
- 数值四倍：忘记右移 2 bit；
- 当成 12-bit：满量程错误；
- 高速时抖动：输入驱动建立时间不足、VDD/去耦差、CS quiet time 不满足；
- 只改变 while 循环速度就称为精确 Fs：错误，应使用 Timer/Event/DMA。

## 19. Datasheet 关键章节

Pin Configuration、Recommended Operating Conditions、Timing Requirements、Serial Interface、Data Format、Power-Down、Typical Application/Input Driver/Layout。

陌生 SAR ADC 迁移见 [SPI SAR ADC Migration Guide](../../00_docs/SPI_SAR_ADC_MIGRATION_GUIDE.md)。

## 20. 官方资料入口

- [TI ADS7887 Product Page](https://www.ti.com/product/ADS7887)
- [TI ADS7887 Datasheet](https://www.ti.com/lit/ds/symlink/ads7887.pdf)

产品页和 Datasheet 已核对；本目录未宣称存在专用 EVM 或上板证据。
