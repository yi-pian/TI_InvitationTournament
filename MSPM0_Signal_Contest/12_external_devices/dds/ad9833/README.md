# AD9833：SPI 可编程波形发生器

README 类型：`EXACT_DEVICE_GUIDE / COMPILE_VERIFIED_DRIVER`

验证状态：`DOC_VERIFIED`、`CODE_COMPILE_VERIFIED`；尚未上板。

官方资料：[ADI 产品页](https://www.analog.com/en/products/ad9833.html) · [AD9833 Datasheet](https://www.analog.com/media/en/technical-documentation/data-sheets/ad9833.pdf) · [AN-1070 Programming Guide](https://www.analog.com/en/resources/app-notes/an-1070.html)

## 1. 它是什么

低功耗 DDS，外部 MCLK 驱动，内部 28-bit 频率累加器和 10-bit DAC，可输出正弦、三角或方波；SPI 写控制字，不需要 MCU 持续搬运波形表。

## 2. 为什么比赛可能用

只用 3 根控制线即可产生稳定可调频率，适合作信号源、扫频激励或时钟。它不支持数字设置任意幅度和直流偏置，幅度/offset 需外部模拟电路。

## 3. 供电

2.3～5.5 V，初次 3.3 V。MCLK、数字地/模拟地、去耦、输出负载和重构滤波按手册设计。常见模块板可能带 25 MHz 晶振，必须读实物丝印，不要硬编码猜值。

## 4. MSPM0 能否直接连接

VDD=3.3 V 时 FSYNC/SCLK/SDATA 可直连。输出 VOUT 是模拟信号，不能当 GPIO；高频时需要滤波和合适终端。

## 5. 需要接 MCU 的 Pin

`SCLK`、`SDATA`、`FSYNC`。`MCLK` 来自板载/外部时钟，不由这个 SPI 驱动自动产生。

## 6. 接线表

| AD9833 | MSPM0/外部 | SysConfig |
|---|---|---|
| SCLK | SPI SCLK | SPI Controller |
| SDATA | SPI PICO/MOSI | SPI Controller |
| FSYNC | GPIO output，初始高 | GPIO |
| MCLK | 有源晶振/时钟 | 通常不接 MCU |
| VOUT | 示波器/DUT（按需滤波） | 无 |
| VDD/AGND/DGND | 3.3 V/地 | 无 |

## 7. SysConfig 一步一步配置

添加 SPI Controller；SCLK+PICO；MSB first、8-bit；按官方接口设置 SCLK idle high，数据在 falling edge 有效（标准 SPI 表述为 CPOL=1、CPHA=0）；初次 1 MHz；另加初始高 GPIO FSYNC。每个 16-bit word 的两个字节期间 FSYNC 必须持续低，两个频率 half-word 之间应分别形成帧。

## 8. 地址/帧格式

无地址。每个写帧固定 16 bit、MSB first。D15:D14 选择 control/frequency/phase 目标；B28=1 时 28-bit 频率字分成两个 14-bit 帧写入。

## 9. 关键寄存器

Control、FREQ0/FREQ1（各 28 bit）、PHASE0/PHASE1（各 12 bit）。当前驱动使用 FREQ0 和 PHASE0，初始化写 `0x2100`（B28=1、RESET=1）。

## 10. Power-Up / Reset 与 Bring-Up 起点

初始化期间保持 control RESET bit=1，先写完整频率和相位，最后清 RESET，避免未配置完成时输出杂散。`AD9833_Init()` 完成第一步；`AD9833_SetOutput()` 完成整套写入。

## 11. 最小初始化

加入 `ad9833.c/.h` 与公共 blocking bus：

```c
SYSCFG_DL_init();
DL_GPIO_setPins(GPIO_DDS_PORT, GPIO_DDS_FSYNC_PIN);
bool ok = AD9833_Init(SPI_0_INST, GPIO_DDS_PORT,
                      GPIO_DDS_FSYNC_PIN);
```

## 12. 启动一次输出

```c
AD9833_SetOutput(SPI_0_INST, GPIO_DDS_PORT,
                 GPIO_DDS_FSYNC_PIN,
                 25000000U, 1000U, 0U,
                 AD9833_WAVE_SINE);
```

前者 25 MHz 是真实 MCLK，后者 1 kHz 是目标输出。

## 12a. 双 AD9833 输出与相位差

两颗器件可以共用一个 SPI 控制器，分别使用两个 FSYNC GPIO。先建立两个通道配置，再调用一次双路接口：

```c
static const ad9833_channel_config_t ch_a = {
    SPI_0_INST, GPIO_DDS_PORT, GPIO_DDS_A_FSYNC_PIN,
    25000000U, 1000U, AD9833_WAVE_SINE
};
static const ad9833_channel_config_t ch_b = {
    SPI_0_INST, GPIO_DDS_PORT, GPIO_DDS_B_FSYNC_PIN,
    25000000U, 1000U, AD9833_WAVE_SINE
};

/* B phase = A phase + 90 degrees (1024 / 4096). */
bool ok = AD9833_SetDualOutput(&ch_a, &ch_b, 0U, 1024U);
```

`phase_difference_code_12bit` 的范围为 0..4095，每码约 0.0879°。两颗 AD9833 应使用同一 MCLK；该 API 通过 SPI 顺序写入两颗器件，不能替代需要严格同时起振的硬件同步/公共复位方案。

## 13. 判断 Ready

无 ready/readback。阻塞 SPI 写完后，输出经过 DDS pipeline 出现；用示波器验证。

## 14. 读取输出值

没有数字回读 API。频率配置状态由应用保存；模拟输出用频率计/ADC/示波器测量。

## 15. 参数换算

`FTW = round(fout × 2^28 / MCLK)`；相位 code 为 12-bit，对应 `phase = code × 360°/4096`。驱动用 64-bit 中间值避免乘法溢出，并限制 `fout <= MCLK/2`；工程实用上限通常更低，由镜像和滤波决定。

## 16. main 完整例子

### 【比赛现场直接复制】

```c
#include <stdbool.h>
#include "ti_msp_dl_config.h"
#include "ad9833.h"

#define AD9833_MCLK_HZ (25000000U) /* 按模块晶振修改 */
volatile bool g_ok;

int main(void)
{
    SYSCFG_DL_init();
    DL_GPIO_setPins(GPIO_DDS_PORT, GPIO_DDS_FSYNC_PIN);
    g_ok = AD9833_SetOutput(SPI_0_INST,
                            GPIO_DDS_PORT,
                            GPIO_DDS_FSYNC_PIN,
                            AD9833_MCLK_HZ,
                            1000U,
                            0U,
                            AD9833_WAVE_SINE);
    while (1) { }
}
```

## 17. 比赛最常改参数

真实 MCLK、output_hz、phase code、wave、SPI/FSYNC 宏、输出滤波。幅度与 offset 不在这个数字 API 内。

## 18. 常见错误

把模块晶振猜成 25 MHz；SPI mode 错；FSYNC 在每个字节间抬高；两个 14-bit 频率字放在同一 32-bit frame；未先置 B28/RESET；期望软件改正弦幅度；输出接重负载导致失真。

## 19. Datasheet 关键章节

Pin Configuration、Serial Interface、Control Register、Frequency/Phase Registers、Programming Flow、Interfacing to Microprocessors、Output Spectrum/Reconstruction、Power Supply and Grounding。

## 20. 官方资料入口

- [Analog Devices AD9833 Product Page](https://www.analog.com/en/products/ad9833.html)
- [Analog Devices AD9833 Datasheet](https://www.analog.com/media/en/technical-documentation/data-sheets/ad9833.pdf)
- [EVAL-AD9833 Evaluation Board](https://www.analog.com/en/resources/evaluation-hardware-and-software/evaluation-boards-kits/eval-ad9833.html)
- [AN-1070：Programming the AD9833/AD9834](https://www.analog.com/en/resources/app-notes/an-1070.html)

EVAL 与应用笔记用于核对编程顺序和输出链；本仓库没有上板验证。
