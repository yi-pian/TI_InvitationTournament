# AD9850：40-bit 串行 / 5-byte 并行 DDS

README 类型：`EXACT_DEVICE_GUIDE / COMPILE_VERIFIED_DRIVER`

验证状态：`DOC_VERIFIED`、`CODE_COMPILE_VERIFIED`；核心有 PC test，尚未上板。

官方资料：[ADI 产品页](https://www.analog.com/en/products/ad9850.html) · [AD9850 Rev.H Datasheet](https://www.analog.com/media/en/technical-documentation/data-sheets/ad9850.pdf)

## 1. 它是什么

带 32-bit frequency tuning word、5-bit phase、10-bit DAC 和比较器的 DDS。可用 40-bit 单线串行或 5 次 8-bit 并行加载，外部参考时钟最高可到 125 MHz（工作条件按数据手册）。本仓库使用最容易接线的 4-GPIO 串行方式。

## 2. 为什么比赛可能用

MCU 只在改频率时发送 40 bit，DDS 自己持续输出，适合信号源、扫频激励和时钟。相比 AD9833，频率字更宽、参考时钟可更高，但常见模块板供电、晶振、输出滤波和逻辑电平差异更大。

## 3. 供电

裸芯片支持 3.3 V 或 5 V。常见模块可能带 100/125 MHz 有源晶振，模块供电必须按其原理图/丝印确认。5 V 裸芯片条件下数字 VIH 不能保证由 MSPM0 3.3 V 高电平满足，需电平转换；不要用 GPIO 给模块供电。

## 4. MSPM0 能否直接连接

AD9850 和模块控制侧都工作在兼容的 3.3 V 逻辑时可直连。若模块 5 V 供电或输出方波接回 MCU，分别检查输入 VIH 和返回电平；模拟正弦输出不能直接当 GPIO。

## 5. 需要接 MCU 的 Pin

常见 4-wire 模块：`W_CLK`、`FQ_UD`、`DATA`、`RESET`。裸芯片串行 DATA 是 D7(pin25)，W_CLK pin7，FQ_UD pin8，RESET pin22；pin12 是 RSET，不是 RESET。

## 6. 接线表

| AD9850 模块 | MSPM0 | SysConfig |
|---|---|---|
| W_CLK | GPIO output | GPIO |
| FQ_UD | GPIO output | GPIO |
| DATA/D7 | GPIO output | GPIO |
| RESET | GPIO output | GPIO |
| VCC/GND | 合规电源/GND | 无 |
| SIN/IOUT | 示波器/DUT（按需滤波） | 无 |
| SQ_OUT | 可选 capture 输入，先确认电平 | GPIO/Timer Capture |

模块排针顺序没有统一标准，必须看实物丝印和模块原理图。

## 7. SysConfig 一步一步配置

1. 添加 4 个 GPIO Output，命名 DDS_W_CLK/FQ_UD/DATA/RESET；2. 初始低；3. 选择未与 ADC、TFT、UART 冲突的管脚；4. 保存并查看生成宏；5. 不需要 SPI，因为 AD9850 串行格式是 40-bit LSB-first GPIO bit-bang；6. `system_clock_hz` 填当前 Clock Tree 真实值。

## 8. 地址/帧格式

没有地址。串行模式每次发送 40 bit：先 32-bit FTW 的 bit0..31，再两个保留 0、power-down bit、5-bit phase；每位在 W_CLK 上升沿移入，最后 FQ_UD 上升沿整体更新。

## 9. 关键控制字

`FTW = round(fout × 2^32 / fref)`；phase code 0..31，每步 11.25°。保留控制位必须按手册保持合法值。

## 10. Power-Up / Reset 与 Bring-Up 起点

模块供电和参考时钟稳定后调用 `AD9850_Init()`；驱动执行硬件 reset、建立串行加载状态并写一帧。不要在模块未供电时先用 GPIO 强驱控制脚，防止反向供电/latch-up。

## 11. 最小初始化

链接唯一正式源码 `ad9850.c`、`ad9850_mspm0_platform.c`，include 本目录：

```c
static ad9850_t g_dds;
static ad9850_mspm0_platform_t g_io = {
    .w_clk_port = DDS_GPIO_PORT, .w_clk_pin = DDS_GPIO_W_CLK_PIN,
    .fq_ud_port = DDS_GPIO_PORT, .fq_ud_pin = DDS_GPIO_FQ_UD_PIN,
    .data_port = DDS_GPIO_PORT, .data_pin = DDS_GPIO_DATA_PIN,
    .reset_port = DDS_GPIO_PORT, .reset_pin = DDS_GPIO_RESET_PIN,
    .system_clock_hz = 32000000U,
};
static const ad9850_config_t g_cfg = {
    .io_context = &g_io,
    .write_line = AD9850_MSPM0_WriteLine,
    .delay_us = AD9850_MSPM0_DelayUs,
    .reference_clock_hz = 125000000U,
    .edge_delay_us = 1U,
};
```

系统时钟和 DDS 晶振必须按实物修改。

## 12. 启动一次输出

```c
AD9850_Init(&g_dds, &g_cfg);
AD9850_SetFrequencyHz(&g_dds, 1000U);
```

## 12a. 双 AD9850 输出与相位差

两颗 AD9850 各自保留一份 `ad9850_t` 和平台 GPIO 配置。两颗器件初始化成功后，可用双路 API 同时更新频率、相位和掉电状态：

```c
/* g_dds_a and g_dds_b must both have returned AD9850_STATUS_OK from Init. */
ad9850_status_t status = AD9850_SetDualOutput(
    &g_dds_a, &g_dds_b,
    1000U, 1000U,
    0U, 8U,       /* B = A + 8 * 11.25 degrees = 90 degrees */
    false, false);
```

`phase_difference_code` 的范围为 0..31，每码 11.25°，第二路相位采用 `(phase_a_code + phase_difference_code) mod 32`。两颗器件应共用同一个参考时钟；`FQ_UD` 更新仍是顺序 GPIO 操作，不能替代严格同步启动所需的公共硬件复位/更新触发。

## 13. 判断 Ready

无 ready/readback。GPIO bit-bang 完成后等待 DDS pipeline；用示波器或频率计确认。

## 14. 读取输出值

不能从控制口读回频率。`ad9850_t` 保存当前 tuning word/phase/power-down；真实输出仍需测量。

## 15. 参数换算

驱动用 64-bit 整数计算 FTW，避免浮点误差/溢出。参考时钟填错会使所有输出按固定比例偏差。软件限制 `fout <= fref/2`，但靠近 Nyquist 时镜像和滤波会让实际可用上限更低。

## 16. main 完整例子

### 【比赛现场直接复制】

```c
#include "ti_msp_dl_config.h"
#include "ad9850.h"
#include "ad9850_mspm0_platform.h"

/* 先放入第 11 节的 g_dds、g_io、g_cfg 定义。 */
volatile ad9850_status_t g_status;

int main(void)
{
    SYSCFG_DL_init();
    g_status = AD9850_Init(&g_dds, &g_cfg);
    if (g_status == AD9850_STATUS_OK) {
        g_status = AD9850_SetFrequencyHz(&g_dds, 1000U);
    }
    while (1) { }
}
```

## 17. 比赛最常改参数

`reference_clock_hz`、`system_clock_hz`、output frequency、phase code、power-down、4 个 GPIO 宏、输出滤波。幅度和 offset 由模块/外部模拟电路决定。

## 18. 常见错误

- 100 MHz 模块却填 125 MHz，所有频率固定比例偏差；
- 5 V 模块直接吃 3.3 V 控制高电平；
- DATA 顺序误做 MSB-first；
- 发送 40 bit 后忘记 FQ_UD；
- 把裸片 pin12 RSET 当 RESET；
- 模块未进入串行模式；
- 高频输出不滤波、负载不匹配导致杂散/幅度差。

## 19. Datasheet 关键章节

Pin Functions、Specifications/Logic Levels、Timing Characteristics、Programming the AD9850、Serial Load、Master Reset、Frequency/Phase Word、DAC/RSET、Output Spectrum/Reconstruction Filter、PCB Layout。

## 20. 官方资料入口

- [Analog Devices AD9850 Product Page](https://www.analog.com/en/products/AD9850.html)
- [Analog Devices AD9850 Datasheet](https://www.analog.com/media/en/technical-documentation/data-sheets/ad9850.pdf)
- [AN-543：High Quality All-Digital RF Frequency Modulation Generation with the AD9850](https://www.analog.com/media/en/technical-documentation/application-notes/an-543.pdf)

官方 Datasheet 还给出评估板信息；本仓库已有 core PC test 和源码编译证据，但没有真实 DDS 模块上板证据。
