# 通用外置 DDS 拼装教程

README 类型：`GENERIC_TUTORIAL`  
状态：`DOCUMENTATION_ONLY / DATASHEET_REQUIRED`；“SPI DDS”不是统一协议。

## 它是什么

DDS 用参考时钟和相位累加器合成可编程周期信号。MSPM0 通常只负责写频率字、相位字和控制寄存器；真正的高速波形在 DDS 芯片内部产生，因此不需要 MCU 逐点送完整波表。

```text
frequency/phase/waveform → 计算 tuning word → 写控制口 → update → DDS/外部滤波 → 输出
```

## 常见信号

| 信号 | 作用 | 型号相关点 |
|---|---|---|
| SCLK、SDIO/SDATA | 写控制字 | SPI mode、位序、帧宽 |
| CS/FSYNC | 界定一帧 | 有效边沿 |
| RESET | 清相位/回已知状态 | 脉宽和复位后输出 |
| UPDATE/IO_UPDATE | 让新参数同时生效 | 有些器件没有独立更新脚 |
| MCLK/REFCLK | 合成参考 | 真实频率、倍频器、抖动 |
| OUT | 模拟输出 | 幅度、偏置、重构滤波和负载 |

## 拿到实物先确认

完整料号、模块晶振实际频率、供电与 I/O 电平、串行协议、频率字位数、参考时钟倍频、Reset/Update、相位/波形能力、输出是电压还是电流、模块板是否已有滤波与放大。不要仅凭“AD98xx 模块”猜控制字。

## MSPM0 / SysConfig / 接线

一般配置低速 `SPI Controller`，再按实际器件增加 CS/FSYNC、RESET、UPDATE GPIO。某些器件使用 GPIO bit-bang 或并行口，必须跟具体 README。

| DDS 功能 | MSPM0/外部 |
|---|---|
| SCLK | SPI SCLK 或 GPIO clock |
| SDATA | SPI MOSI 或 GPIO data |
| FSYNC/CS | GPIO output |
| RESET/UPDATE | GPIO output（若存在） |
| MCLK | 模块晶振或外部低抖动时钟，不由普通 GPIO 冒充 |
| OUT | 示波器/滤波/放大级，确认地与幅度 |

## 最小 Bring-Up

1. 只接安全负载，用示波器观察输出。
2. 低速 blocking 写 Reset 和最少初始化字。
3. 设置远低于 MCLK 的固定频率，选择最简单波形。
4. 按官方顺序 Update/退出 Reset。
5. 测输出频率，反推 MCLK 是否与代码一致。
6. 再验证相位、波形和幅度控制；扫频放到最后。

## Generic main 框架

```c
#include <stdbool.h>
#include <stdint.h>
#include "ti_msp_dl_config.h"

bool TODO_MODEL_SPECIFIC_DDS_ResetAndInit(uint32_t reference_clock_hz);
bool TODO_MODEL_SPECIFIC_DDS_SetFrequency(uint32_t frequency_hz);
bool TODO_MODEL_SPECIFIC_DDS_ApplyUpdate(void);

int main(void)
{
    SYSCFG_DL_init();
    if (!TODO_MODEL_SPECIFIC_DDS_ResetAndInit(
            TODO_MODEL_SPECIFIC_REFERENCE_CLOCK_HZ) ||
        !TODO_MODEL_SPECIFIC_DDS_SetFrequency(1000U) ||
        !TODO_MODEL_SPECIFIC_DDS_ApplyUpdate()) {
        __BKPT(0);
    }
    while (1) { __WFI(); }
}
```

这是流程模板，不是可链接 API。

## 比赛最常改参数

`reference_clock_hz`、输出频率、相位、波形类型、倍频设置、幅度/外部增益、Update 时机和扫频步进/驻留时间。

## 如果换成另一个 DDS

可复用“参数 → tuning word → 写入 → update”的上层结构；必须重查字宽、寄存器、参考时钟、Reset/Update、输出结构和滤波。已有具体教程：[AD9833](../ad9833/README.md)、[AD9850](../ad9850/README.md)；通用总线步骤见 [SPI 寄存器器件 Recipe](../../00_docs/recipes/SPI_REGISTER_DEVICE_RECIPE.md)。
