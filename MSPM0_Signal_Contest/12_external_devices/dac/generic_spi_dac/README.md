# 通用 SPI DAC 拼装教程

README 类型：`GENERIC_TUTORIAL`  
状态：`DOCUMENTATION_ONLY / DATASHEET_REQUIRED`；SPI DAC 没有统一帧格式。

## 它是什么

SPI DAC 接收数字 code，并在模拟输出端产生相应电压或电流。它适合设定直流偏置、控制增益，也可在更新速度足够时输出查表波形。

```text
目标电压/波形点 → code 换算 → SPI 帧 → 输入寄存器 → LDAC/自动更新 → 模拟输出
```

## 常见信号

| 信号 | 作用 | 型号相关点 |
|---|---|---|
| SCLK、SDI | 传输命令与 code | Mode、位顺序、word bits |
| CS/SYNC | 限定一帧 | 有效边沿和帧间隔 |
| LDAC | 把输入寄存器同步更新到输出 | 是否存在、可否固定电平 |
| CLR/RESET | 输出回到安全码 | 默认码和有效电平 |
| VREF | 决定输出比例 | 内置/外置、允许范围、驱动要求 |
| VOUT 或 IOUT | 模拟输出 | 电压型、乘法型电流型、负载能力 |

## 拿到实物先确认

完整料号、通道数、分辨率、供电与 I/O 电平、参考源、输出是电压还是电流、单/双极性电路、帧位数、SPI mode、LDAC/CLR、上电默认码、建立时间、输出负载与运放需求。特别注意：电流输出/乘法 DAC 通常不能把 IOUT 当普通 VOUT 直接使用。

## MSPM0 与接线

SysConfig 通常添加 `SPI Controller`，再按器件添加 CS、LDAC、CLR GPIO。`SYSCFG_DL_init()` 后先用 blocking 写寄存器。

| DAC 功能 | MSPM0/外部电路 |
|---|---|
| SCLK | SPI SCLK |
| SDI | SPI MOSI |
| CS/SYNC | GPIO output |
| LDAC/CLR | GPIO output（若存在） |
| VREF | 精密参考或器件规定来源，不接 GPIO |
| IOUT | 按 datasheet 接跨阻/输出运放 |
| GND | 正确处理模拟地与数字地回流 |

## 最小 Bring-Up

1. 先确认输出级和 VREF 接法，负载保持安全。
2. SPI 低速 blocking，控制 CLR/LDAC 到确定状态。
3. 依次写零点、中点、近满量程三个安全 code。
4. 用万用表测量，检查方向、单调性和增益。
5. 静态输出正确后才连续更新；波形输出再加 Timer/DMA 和重构滤波。

## Generic main 框架

```c
#include <stdbool.h>
#include <stdint.h>
#include "ti_msp_dl_config.h"

bool TODO_MODEL_SPECIFIC_ResetAndConfigure(void);
bool TODO_MODEL_SPECIFIC_WriteCode(uint32_t code);
void TODO_MODEL_SPECIFIC_LatchOutput(void);

int main(void)
{
    SYSCFG_DL_init();
    if (!TODO_MODEL_SPECIFIC_ResetAndConfigure() ||
        !TODO_MODEL_SPECIFIC_WriteCode(TODO_MODEL_SPECIFIC_MIDSCALE_CODE)) {
        __BKPT(0);
    }
    TODO_MODEL_SPECIFIC_LatchOutput();
    while (1) { __WFI(); }
}
```

这是 `GENERIC TEMPLATE`；所有 `TODO_MODEL_SPECIFIC_*` 都必须由完整型号实现替换。

## 连续波形怎样升级

总线有效位率至少覆盖 `Fs × 每样点帧位数`，并给 CS、LDAC、CPU/DMA 留裕量。顺序是静态三点 → blocking 波表 → Timer 节拍 → SPI TX DMA → 双 buffer。别把 DAC 标称更新率直接当成可达到的系统波形采样率。

## 比赛最常改参数

输出范围、VREF、分辨率/code 上限、SPI 时钟、LDAC 策略、波形 Fs、波表 N、幅度、偏置和输出运放增益。

## 如果换成另一个 SPI DAC

可复用“电压到 code、写帧、锁存输出”的结构。必须重新核对帧格式、参考与输出类型、上电码、LDAC/CLR、建立时间和负载。参考 [SPI 寄存器器件 Recipe](../../00_docs/recipes/SPI_REGISTER_DEVICE_RECIPE.md)；乘法 DAC 示例见 [DAC7811](../dac7811/README.md)。

