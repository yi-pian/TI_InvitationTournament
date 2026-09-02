# ADS112C04：I2C 16-bit 精密 ADC

README 类型：`EXACT_DEVICE_GUIDE / COMPILE_VERIFIED_DRIVER`

验证状态：`DOC_VERIFIED`、`CODE_COMPILE_VERIFIED`；尚未上板，因此不是 `BOARD_VERIFIED`。

官方资料：[TI 产品页](https://www.ti.com/product/ADS112C04) · [ADS112C04 Datasheet](https://www.ti.com/lit/ds/symlink/ads112c04.pdf)

## 1. 它是什么

ADS112C04 是 16-bit、最高 2 kSPS 的 ΔΣ ADC，内部带 4 路输入 MUX、1～128 倍 PGA、2.048 V 参考、振荡器和激励电流源，通过 I2C 控制。

小白理解：它不是用来抓很高速波形的；它擅长把传感器、桥式电路或小直流信号慢而精细地量出来。

## 2. 为什么比赛可能用

- 需要比 MSPM0 内部 ADC 更高的低速直流分辨率；
- 热电偶、应变桥、RTD、小差分电压；
- 需要可编程增益、内部参考或 50/60 Hz 抑制；
- 需要 4 路单端或 2 路差分输入。

若题目要求几十 kSPS 以上波形/频谱，优先 SAR ADC，不要被“16-bit”吸引而忽略 2 kSPS 上限。

## 3. 供电

`AVDD-AVSS` 和 `DVDD-DGND` 推荐范围均为 2.3～5.5 V。最容易联调的单电源方案是 AVDD=DVDD=3.3 V、AVSS=DGND=GND，每组电源就近放至少 100 nF 去耦。模拟输入绝对电压及 PGA 共模范围仍须遵守数据手册，不能仅按数字满量程判断。

## 4. MSPM0 能否直接连接

可以。在 DVDD=3.3 V 且 I2C 上拉到 3.3 V 时，SDA/SCL、RESET、DRDY 可直接连接 MSPM0G3507。SDA/SCL 必须上拉；DRDY 也按手册用上拉。若模块板把 I2C 上拉到了 5 V，先改到 3.3 V 或加合适的双向电平转换。

## 5. 每个需要接 MCU 的 Pin

- `SDA`：I2C 数据；
- `SCL`：I2C 时钟；
- `RESET`：低有效复位，可接 GPIO，也可拉高仅使用 RESET 命令；
- `DRDY`：低有效数据就绪，可接 GPIO；初次可不接，轮询寄存器 2 的 DRDY 位；
- `A0/A1`：地址选择，通常硬接 GND 或 DVDD，不需要 MCU 动态控制。

封装引脚号不同：TSSOP 的 SDA/SCL 为 15/16，WQFN 的 SDA/SCL 为 13/14。连线时按你手上封装的 Pin Functions 表，不按本 README 猜脚号。

## 6. 接线表

| ADS112C04 | MSPM0G3507 功能 | SysConfig |
|---|---|---|
| SDA | 任一 I2C SDA 复用脚 | I2C Controller / SDA |
| SCL | 任一 I2C SCL 复用脚 | I2C Controller / SCL |
| DRDY | 可选 GPIO 输入 | GPIO Input；后续可设下降沿中断 |
| RESET | 可选 GPIO 输出 | GPIO Output，初始高 |
| A0、A1 | 初次都接 GND | 不需要 |
| DVDD、AVDD | 3.3 V | 不需要 |
| DGND、AVSS | GND | 不需要 |
| REFP、REFN | 内部参考时按手册处理 | 不接 MCU |
| AIN0…AIN3 | 被测模拟信号 | 不属于 SysConfig |

## 7. SysConfig 一步一步配置

1. 打开工程 `.syscfg`，添加 **I2C** 外设，模式选择 Controller。
2. 选择一组可用 SDA/SCL 管脚；给实例命名，例如 `I2C_0`。
3. 速率先选 Standard mode 100 kHz。
4. SDA、SCL 外部各接约 4.7 kΩ 到 3.3 V；模块已有上拉时不要盲目再并很多电阻。
5. 初次使用寄存器轮询，不需要开 I2C interrupt/DMA。
6. 若接 DRDY，增加 GPIO Input；若接 RESET，增加初始高的 GPIO Output。
7. 保存，确认生成头中存在你实际的 `I2C_x_INST`。示例使用 `I2C_0_INST`，若你的宏不同必须替换。

详细 I2C 公共方法见 [I2C Register Device Recipe](../../00_docs/recipes/I2C_REGISTER_DEVICE_RECIPE.md)。

## 8. I2C 地址

A1/A0 可以分别接 GND、DVDD、SDA 或 SCL，形成 16 个地址。最简单的 A1=GND、A0=GND 对应 7-bit 地址 `0x40`，代码直接传 `0x40`，不要左移成 `0x80`。

本驱动常量：

```c
#define ADS112C04_ADDRESS_A1_GND_A0_GND (0x40U)
```

## 9. 关键寄存器

| 地址 | 名称 | 主要内容 | 复位值 |
|---|---|---|---|
| 0 | CONFIG0 | MUX、GAIN、PGA_BYPASS | 0x00 |
| 1 | CONFIG1 | 数据率、normal/turbo、single/continuous、参考、温度模式 | 0x00 |
| 2 | CONFIG2 | DRDY、CRC、burn-out、IDAC 电流 | 0x00 |
| 3 | CONFIG3 | 两个 IDAC 的输出路由 | 0x00 |

例：`CONFIG0=0x81` 表示 AIN0 对 AVSS、gain=1、旁路 PGA；这是最容易理解的单端输入设置。`CONFIG1=0x80` 表示 normal mode、330 SPS、single-shot、内部 2.048 V 参考。

## 10. Power-Up / Reset 与 Bring-Up 起点

上电或 RESET 后 4 个寄存器都回到 0，器件等待 START/SYNC。软件复位：

```c
bool ok = ADS112C04_Reset(I2C_0_INST, 0x40U);
```

本驱动发送 RESET 命令 `0x06`。不用额外改生成的 DriverLib 文件，也不要编辑 `ti_msp_dl_config.c`。

## 11. 最小初始化

工程加入：

- `ads112c04.c/.h`；
- `../../00_common/mspm0_blocking_bus.c/.h`；
- 把上述两个目录加入 include path。

```c
bool ok = ADS112C04_Reset(I2C_0_INST, 0x40U);
ok = ok && ADS112C04_WriteRegister(I2C_0_INST, 0x40U,
                                   ADS112C04_REG_CONFIG0, 0x81U);
ok = ok && ADS112C04_WriteRegister(I2C_0_INST, 0x40U,
                                   ADS112C04_REG_CONFIG1, 0x80U);
```

这只是 AIN0 单端、gain 1、330 SPS 的 bring-up 配置。换差分输入、增益或参考时重新计算寄存器，不要只改电压公式。

## 12. 启动转换

single-shot 模式每次测量都发送：

```c
ADS112C04_StartSingle(I2C_0_INST, 0x40U);
```

continuous 模式需先把 CONFIG1 的 CM(bit3) 设为 1，再发送一次 START/SYNC；停止时发送 POWERDOWN。当前轻量驱动优先 single-shot bring-up，没有建立复杂状态机。

## 13. 判断数据 Ready

方法 1，轮询 CONFIG2 bit7：

```c
bool ready = false;
while (!ready) {
    if (!ADS112C04_IsReady(I2C_0_INST, 0x40U, &ready)) {
        /* I2C 错误处理 */
        break;
    }
}
```

正式应用要加超时。方法 2，把 DRDY 接 GPIO：转换完成时 DRDY 变低；可先轮询 `DL_GPIO_readPins()`，稳定后再用下降沿中断。

## 14. 读取 ADC 值

```c
int16_t raw;
bool ok = ADS112C04_ReadRaw(I2C_0_INST, 0x40U, &raw);
```

结果是 16-bit 二补码，差分输入为负时 `raw` 会是负数。驱动发送 RDATA，再用 repeated START 读取 MSB、LSB 两字节。

## 15. raw code 变成电压

差分电压：

```c
float vin = ADS112C04_RawToVoltage(raw, 2.048f, 1U);
```

公式为 `Vin = raw × Vref / (32768 × gain)`。单端 AINx 对 AVSS 只使用正半量程，接近 0 V 时受偏置影响偶尔可能出现小负码。外部参考必须传实际参考值；gain 必须和 CONFIG0 一致。

## 16. main 完整例子

### 【比赛现场直接复制】

```c
#include <stdbool.h>
#include <stdint.h>
#include "ti_msp_dl_config.h"
#include "ads112c04.h"

#define ADC_ADDR       (ADS112C04_ADDRESS_A1_GND_A0_GND)
#define READY_POLLS    (10000U)

volatile int16_t g_raw;
volatile float g_voltage;
volatile bool g_ok;

int main(void)
{
    bool ready = false;
    uint32_t polls = READY_POLLS;

    SYSCFG_DL_init();

    g_ok = ADS112C04_Reset(I2C_0_INST, ADC_ADDR);
    g_ok = g_ok && ADS112C04_WriteRegister(
        I2C_0_INST, ADC_ADDR, ADS112C04_REG_CONFIG0, 0x81U);
    g_ok = g_ok && ADS112C04_WriteRegister(
        I2C_0_INST, ADC_ADDR, ADS112C04_REG_CONFIG1, 0x80U);
    g_ok = g_ok && ADS112C04_StartSingle(I2C_0_INST, ADC_ADDR);

    while (g_ok && !ready && (polls-- > 0U)) {
        g_ok = ADS112C04_IsReady(I2C_0_INST, ADC_ADDR, &ready);
    }
    g_ok = g_ok && ready;
    g_ok = g_ok && ADS112C04_ReadRaw(I2C_0_INST, ADC_ADDR,
                                     (int16_t *)&g_raw);
    if (g_ok) {
        g_voltage = ADS112C04_RawToVoltage(g_raw, 2.048f, 1U);
    }

    while (1) {
    }
}
```

把 `I2C_0_INST` 换成你工程实际生成宏。观察 `g_ok/g_raw/g_voltage`，或再接 UART/屏幕显示。

## 17. 比赛最常改参数

| 要改什么 | 改哪里 | 同时检查什么 |
|---|---|---|
| I2C 地址 | `ADC_ADDR` | A0/A1 实际接法 |
| 输入通道/差分组合 | CONFIG0 MUX | 模拟接线 |
| gain | CONFIG0 GAIN | RawToVoltage 的 gain、共模范围 |
| 数据率 | CONFIG1 DR | 转换等待时间、噪声 |
| single/continuous | CONFIG1 CM | START/POWERDOWN 流程 |
| 内/外部参考 | CONFIG1 VREF | REFP/REFN 接线、换算电压 |
| Ready 方法 | 轮询或 DRDY | SysConfig GPIO/中断 |

## 18. 常见错误

- NACK：地址错、没有上拉、A0/A1 与代码不一致、没共地；
- 一直不 ready：没发 START、CONFIG1/命令错、轮询超时太短；
- 单端配置满是负数：MUX 正负端顺序或地线错误；
- 高增益饱和：差分输入超过 `VREF/gain` 或共模不满足 PGA；
- 电压差一倍/多倍：配置增益、参考与换算参数不一致；
- 读到旧数据：未等 DRDY，或 continuous 模式消费速度不足；
- 把它用于高速 FFT：2 kSPS 架构不匹配。

## 19. Datasheet 关键章节

- §5 Pin Configuration and Functions；
- §6.3 Recommended Operating Conditions；
- §8.3 Input Multiplexer、PGA、Reference、Digital Filter；
- §8.4 Device Functional Modes；
- §8.5 Programming、I2C、Commands、Data Format；
- §8.6 Register Map；
- §10 Power Supply Recommendations；§11 Layout。

## 20. 官方资料入口

- [TI ADS112C04 Product Page](https://www.ti.com/product/ADS112C04)
- [TI ADS112C04 Datasheet](https://www.ti.com/lit/ds/symlink/ads112c04.pdf)
- [TI ADS112C04EVM](https://www.ti.com/tool/ADS112C04EVM)

Product Page、Datasheet 和 EVM 资料已核对；本仓库仍只有源码编译证据，没有实物板验证。
