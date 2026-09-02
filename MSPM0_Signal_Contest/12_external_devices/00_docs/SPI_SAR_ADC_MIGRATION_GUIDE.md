# 陌生 SPI SAR ADC 迁移指南

当库里没有题目指定的 XYZ ADC 时，不要从零开始。先把它归类为“SPI 流式 SAR ADC”，打开 [SPI Streaming ADC Recipe](recipes/SPI_STREAMING_ADC_RECIPE.md)，再只替换本页列出的器件差异。

## 1. 通常可以保留的部分

- MSPM0 SysConfig 中 SPI Controller 的基本建立方式；
- SCLK、POCI/MISO、GPIO CS 的连接结构；
- `SYSCFG_DL_init()`；
- 拉低 CS、用 dummy byte 产生时钟、等待 SPI 空闲、拉高 CS；
- 把两个接收字节组合成 `uint16_t frame`；
- 固定直流三点验证和逻辑分析仪检查方法。

## 2. 必须重新查 Datasheet 的部分

| 必查项 | 为什么不能沿用 ADS7887/ADS7866 |
|---|---|
| 供电和输入范围 | 可能是外部参考、差分或双极性 |
| SPI mode | CPOL/CPHA 错会整体移位或无数据 |
| frame length | 可能 12、16、18、24、32 clocks |
| data bits | 分辨率与传输帧长度不是一回事 |
| leading/trailing bits | 决定 mask 和 shift |
| CS timing | 可能由 CS 边沿启动转换，或需要独立 CONVST |
| SCLK 上限/下限 | 某些器件要求在规定时间内读完 |
| quiet/acquisition time | 决定两次采样之间能否立即开始 |
| busy/ready | 可能需要等待 BUSY/DRDY |
| command bits | 有些 SAR ADC 需要通道/功耗命令 |
| output coding | unsigned、two's complement、offset binary |

## 3. 用表格完成迁移

| 参数 | ADS7887 参考 | ADS7866 参考 | XYZ（从手册填写） |
|---|---|---|---|
| resolution | 10 bit | 12 bit | |
| SPI mode | 按 ADS7887 timing 配置并实测确认 | Mode 3 | |
| clocks/frame | 16 | 16 | |
| data extraction | `(frame >> 2) & 0x03FF` | `frame & 0x0FFF` | |
| start conversion | CS falling | CS falling | |
| analog full-scale | 0..VDD | 0..VDD | |
| max sample rate | 1.25 MSPS | 200 kSPS | |

## 4. 最小迁移代码

### 【比赛现场直接复制】

```c
uint16_t frame = adc_read_frame16();

/* 这两项是迁移到 XYZ 时最常改的地方。 */
#define XYZ_DATA_SHIFT  (2U)
#define XYZ_DATA_MASK   (0x03FFU)

uint16_t raw = (frame >> XYZ_DATA_SHIFT) & XYZ_DATA_MASK;
```

若 XYZ 不是 16-clock 帧，就修改 `adc_read_frame16()` 的收发字节数；若带命令，就把某些 dummy byte 替换为命令字节；若需要 CONVST/BUSY，就按 GPIO Recipe 增加控制和等待。不要为此修改正式 ADS7887 驱动。

## 5. 迁移验收

1. 暂时把 SPI 降到器件上限的 1/4 以下；2. 接 GND、约半满量程、接近满量程；3. 记录原始完整 frame，不要先截位；4. 在逻辑分析仪中手算一个 frame；5. 确认 raw 公式；6. 再提高采样率；7. 最后才迁移到 DMA。

只要这 7 项通过，说明你已经掌握了陌生 SAR ADC 的核心接口，不需要库里预先存在 XYZ 型号。

