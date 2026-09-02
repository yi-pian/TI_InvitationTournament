# SPI 流式 ADC 接入 Recipe

适用于 ADS7887、ADS7866 一类“CS 启动一次转换，同时在固定数量 SCLK 中移出结果”的 SAR ADC。它们通常没有复杂寄存器，关键是采样时刻、帧长和数据位位置。

## 1. 必须确认的时序参数

- CS 哪个边沿开始采样/转换；
- 一次转换需要多少个 SCLK；
- SPI CPOL/CPHA；
- 数据在上升沿还是下降沿有效；
- 结果前有几个 leading zero，后有几个 trailing zero；
- 最大 SCLK、最大采样率、两帧之间 quiet time；
- 模拟输入范围及参考电压来源。

“12-bit ADC”不等于“读 12 个时钟”。很多器件仍要求完整 16-bit 帧。

## 2. 接线

| ADC | MSPM0G3507 | 注意 |
|---|---|---|
| SCLK | SPI SCLK | 先低速验证 |
| SDO | SPI POCI/MISO | ADC 输出，MCU 输入 |
| CS | GPIO 输出 | 软件控制每个采样帧 |
| VIN | 被测模拟信号 | 不得超过 ADC 允许范围 |
| VDD/VREF | 稳定电源/参考 | 就近去耦 |
| GND | GND | 模拟与数字回流按硬件设计处理 |

只有输出数据的 ADC 通常不需要连接 PICO/MOSI，但 SPI controller 仍可发送 `0x00` 产生时钟。

## 3. SysConfig

1. 添加 SPI Controller；选择 SCLK 和 POCI，PICO 可保留用于产生时钟。
2. 设置 MSB first、8-bit data size，并严格按 ADC 数据手册选 CPOL/CPHA。
3. SCLK 先设 500 kHz～1 MHz，验证后再按吞吐率计算提高。
4. 添加一个初始为高的 GPIO 输出作 CS。
5. 保存并使用生成的 `SPI_x_INST`、端口宏和 CS pin 宏。

## 4. 阻塞读取一个 16-bit 帧

### 【比赛现场直接复制】

```c
static uint16_t adc_read_frame16(void)
{
    uint16_t frame;

    DL_GPIO_clearPins(GPIO_ADC_PORT, GPIO_ADC_CS_PIN);
    DL_SPI_transmitDataBlocking8(SPI_0_INST, 0x00U);
    frame = (uint16_t)DL_SPI_receiveDataBlocking8(SPI_0_INST) << 8;
    DL_SPI_transmitDataBlocking8(SPI_0_INST, 0x00U);
    frame |= DL_SPI_receiveDataBlocking8(SPI_0_INST);
    while (DL_SPI_isBusy(SPI_0_INST)) {
    }
    DL_GPIO_setPins(GPIO_ADC_PORT, GPIO_ADC_CS_PIN);
    return frame;
}
```

然后按手册截位，例如：

```c
uint16_t raw_10bit = (adc_read_frame16() >> 2) & 0x03FFU;
uint16_t raw_12bit = adc_read_frame16() & 0x0FFFU;
```

这两个公式不能混用。ADS7887 是 10-bit 数据位于帧中间；ADS7866 是 12-bit 数据位于低 12 位。

## 5. 转电压

单极性、满量程等于 `VREF` 时常用：

```c
float voltage = ((float)raw * vref_volts) / (float)(1UL << bits);
```

若器件规定输入范围为 `0..VDD`，就传实际 VDD；若使用外部参考，就传参考实测值。差分、双极性或带前端增益的 ADC 必须按其传输函数计算。

## 6. 从阻塞升级到连续采样

阻塞版先证明电气与帧格式正确。需要稳定采样率时，再使用 Timer/Event 触发 SPI 或 DMA，并保留同一套“16-bit 帧如何截位”的纯函数。升级前先确认：DMA 能否精确控制 CS、每次传输字节数、帧间 quiet time、Buffer 半满/满的消费速度。

## 7. 失败定位

- 全 0：SDO 未接、CS/时钟未到、模拟输入为 0、SPI 边沿错；
- 全 1：POCI 悬空或电平不兼容；
- 数值约为正确值的 4 倍/四分之一：数据右移位数错；
- 偶发跳码：SCLK 太快、参考/输入不稳定、CS quiet time 不够；
- 波形顺序对但数字错：先把固定直流电压接入，逐位对照逻辑分析仪。

