# 通用 SPI ADC 拼装教程

README 类型：`GENERIC_TUTORIAL`  
状态：`DOCUMENTATION_ONLY / DATASHEET_REQUIRED`；没有一个叫“Generic SPI ADC”的统一协议或可链接 Driver。

## 它是什么

这类 ADC 用 SPI 把模拟采样结果送给 MSPM0。器件可能是“CS 一拉低就开始转换”的 SAR ADC，也可能要先发命令、等待 DRDY，或者连续输出带通道标签的数据帧。因此，SPI 只是传输方式，不代表帧格式相同。

```text
模拟输入 → ADC 转换 → Ready/等待 → SPI 读取帧 → raw → 按码型换成电压
```

## 常见信号

| 信号 | 通常作用 | 必须查的型号差异 |
|---|---|---|
| `SCLK` | 串行时钟 | Mode、最高/最低频率、空闲电平 |
| `SDO/MISO` | ADC 向 MCU 送数据 | 高阻条件、位顺序、数据线数量 |
| `SDI/MOSI` | MCU 发命令/配置 | 有些纯读 ADC 不需要 |
| `CS` | 选中器件，也可能控制采样周期 | 有效电平、建立保持时间 |
| `CONVST` | 独立启动转换 | 是否存在、有效边沿 |
| `DRDY/BUSY` | 数据就绪/转换忙 | 极性和清除条件 |
| `RESET` | 回到已知状态 | 是否存在及脉宽 |

## 拿到实物先确认

完整料号、模块原理图、供电/数字 I/O 电平、单端/差分/伪差分输入、输入范围与共模、参考源、分辨率、最大采样率、SPI mode、每帧位数、码型、通道标签、流水线延迟、DRDY/BUSY 规则。任何一个未确认，都不要猜常量。

## MSPM0、SysConfig 与接线

第一次只需要：一个低速 `SPI Controller`、CS GPIO，以及器件真实存在的 DRDY/CONVST/RESET GPIO。保留 SysConfig 生成的外设名和 Pin 名；代码调用 `SYSCFG_DL_init()`，不要手改生成文件。

| 器件功能 | MSPM0 |
|---|---|
| SCLK | SPI SCLK |
| SDO | SPI MISO |
| SDI（若有） | SPI MOSI |
| CS | GPIO output 或受控片选 |
| DRDY/BUSY | GPIO input，先轮询 |
| CONVST/RESET | GPIO output |
| GND | 与 MCU 建立正确数字地参考 |

具体 MSPM0 Pin 由你的板卡和 SysConfig 决定，README 不写死。

## 最小 Bring-Up

1. 输入接 GND、中点或安全的已知直流，不要悬空。
2. SPI 先用低速 blocking 模式，逻辑分析仪同时看 SCLK/CS/SDO。
3. 按 datasheet 完成 Reset、上电等待和必要配置。
4. 只触发/读取一次，打印十六进制 raw。
5. 用三个已知电压确认零点、方向、单调性和码型。
6. 确认单次读取后再循环；达到带宽瓶颈后才改 Timer + DMA。

## Generic main 框架

这是流程模板，`TODO_MODEL_SPECIFIC_*` 不是仓库 API，必须由具体型号 Driver 替换。

```c
#include <stdbool.h>
#include <stdint.h>
#include "ti_msp_dl_config.h"

bool TODO_MODEL_SPECIFIC_ResetAndConfigure(void);
bool TODO_MODEL_SPECIFIC_StartConversion(void);
bool TODO_MODEL_SPECIFIC_WaitReady(uint32_t timeout);
bool TODO_MODEL_SPECIFIC_ReadRaw(int32_t *raw);

int main(void)
{
    int32_t raw = 0;
    SYSCFG_DL_init();

    if (!TODO_MODEL_SPECIFIC_ResetAndConfigure() ||
        !TODO_MODEL_SPECIFIC_StartConversion() ||
        !TODO_MODEL_SPECIFIC_WaitReady(100000U) ||
        !TODO_MODEL_SPECIFIC_ReadRaw(&raw)) {
        __BKPT(0);
    }

    /* 在调试器/UART 中检查 raw；确认码型后再换算电压。 */
    while (1) { __WFI(); }
}
```

## 高速时怎样升级

先估算 `Fs × 每帧位数 × 通道数`，再加协议和软件裕量。升级顺序是 blocking 单次 → Timer 固定节拍 → DRDY 中断 → SPI RX DMA → Ping-Pong buffer。DMA 只解决搬运，不会修正错误 SPI mode、丢帧或模拟前端未稳定。

## 比赛最常改参数

`sample_rate_hz`、通道/输入选择、PGA 或量程、参考电压、SPI 时钟、帧长度、DRDY 超时、raw 码型与电压换算。

## 如果换成另一个 SPI ADC

可复用的是“先 blocking 验证一帧，再升级 DMA”的流程。必须重做的是供电、输入结构、帧协议、SPI mode、Ready 条件、码型、流水线延迟和电压换算。为完整料号建独立目录，不要在应用中堆条件宏模拟万能 Driver。

参考：[SPI 连续 ADC Recipe](../../00_docs/recipes/SPI_STREAMING_ADC_RECIPE.md)；已有明确型号可查看 [ADS7866](../ads7866/README.md)、[ADS7887](../ads7887/README.md) 和 [ADS112C04](../ads112c04/README.md)。
