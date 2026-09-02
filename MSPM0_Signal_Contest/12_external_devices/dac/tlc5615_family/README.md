# TLC5615 类串行 DAC 通用教程

README 类型：`GENERIC_TUTORIAL`  
状态：`DOCUMENTATION_ONLY / DATASHEET_REQUIRED`；没有选定完整后缀和模块板，不能给出真实帧常量。

## 先知道它在做什么

TLC5615 类目录用于“串行写入一个 DAC code，得到对应模拟输出”的学习入口。它不是现成 Driver。完整料号、兼容替代品或第三方模块板可能在供电、参考、数字边沿、帧宽和输出缓冲上不同。

```text
目标电压 → 根据真实 VREF/增益算 code → DIN/SCLK/CS 发送一帧 → DAC 输出稳定
```

## 常见功能线与接线

| 功能 | MSPM0/外部连接 | 先查什么 |
|---|---|---|
| DIN | SPI MOSI | 位顺序和有效位位置 |
| SCLK | SPI SCLK | 空闲电平、采样边沿、速率 |
| CS | GPIO output | 有效电平和锁存边沿 |
| LDAC（若有） | GPIO output | 是否独立更新 |
| VREF | 参考源 | 允许范围和去耦 |
| VOUT | 万用表/后级 | 输出范围、负载、建立时间 |

SysConfig 新建低速 `SPI Controller` 和 CS GPIO。仅当你的具体器件确有 LDAC/CLR 时才添加相应 GPIO。不要照抄别的 DAC 的 SPI mode。

## 最小 Bring-Up

1. 记录芯片顶标、完整后缀和模块原理图。
2. 查供电、数字 I/O、VREF、帧格式和上电输出。
3. 输出先接高阻万用表，不接功率负载。
4. blocking SPI 分别写 0、中点、近满量程 code。
5. 测量单调变化，核对实际增益与偏置。
6. 静态正确后才循环写波表；需要稳定 Fs 时加入 Timer/DMA。

## Generic main 框架

```c
#include <stdbool.h>
#include <stdint.h>
#include "ti_msp_dl_config.h"

bool TODO_MODEL_SPECIFIC_TLC5615_WriteCode(uint16_t code);

int main(void)
{
    SYSCFG_DL_init();
    if (!TODO_MODEL_SPECIFIC_TLC5615_WriteCode(
            TODO_MODEL_SPECIFIC_MIDSCALE_CODE)) {
        __BKPT(0);
    }
    while (1) { __WFI(); }
}
```

模板不会自行编译；它刻意把未知帧格式标成 `TODO_MODEL_SPECIFIC`。

## 比赛最常改参数

完整料号、VREF、code 位数、SPI mode/clock、CS 时序、目标电压、更新 Fs 和外部输出增益。

## 如果换成另一个同类 DAC

保留“算 code → 发帧 → 测静态三点”的流程，重查供电、VREF、帧位数、边沿、输出公式和建立时间。详细拼装步骤见 [通用 SPI DAC](../generic_spi_dac/README.md) 和 [SPI 寄存器器件 Recipe](../../00_docs/recipes/SPI_REGISTER_DEVICE_RECIPE.md)。

