# SPI 寄存器器件接入 Recipe

适用于 PGA、数字电位器、带寄存器的 DAC、DDS 等“拉低 CS，发送命令/地址/数据，再拉高 CS”的器件。比赛首次联调优先使用阻塞轮询，确认协议正确后才考虑中断或 DMA。

## 1. 先从数据手册抄下 6 项

| 项目 | 从哪里看 | 例子 |
|---|---|---|
| SPI 模式 | Timing / Serial Interface | Mode 0：CPOL=0、CPHA=0 |
| 位序 | Timing | MSB first |
| 每帧位数 | Serial Interface | 16 bit |
| 最大 SCLK | Electrical Characteristics | 10 MHz |
| CS 时序 | Timing Diagram | 传输期间保持低 |
| 写帧格式 | Register / Command Table | 命令 4 bit + 数据 12 bit |

不要凭“多数 SPI 都是 Mode 0”猜模式。模式错时，逻辑分析仪上仍会有波形，但器件可能完全不响应。

## 2. MSPM0G3507 连接

| 器件 | MSPM0 | 说明 |
|---|---|---|
| SCLK | 任一可复用为 SPI SCLK 的管脚 | 时钟 |
| DIN/SDI | SPI PICO/MOSI | MCU 发给器件 |
| DOUT/SDO | SPI POCI/MISO | 只写器件可不接 |
| CS/SYNC/FSYNC | 普通 GPIO 输出 | 推荐软件控制，一帧边界最清楚 |
| GND | GND | 必须共地 |

先核对器件数字电源和逻辑阈值。不能因为引脚名字相同就把 5 V 逻辑直接接到 3.3 V MCU。

## 3. SysConfig 手动配置

1. 打开工程的 `.syscfg`，添加一个 **SPI** 外设并选择 **Controller**。
2. 选择 SCLK、PICO（MOSI）；器件需要回读时再选择 POCI（MISO）。
3. 按数据手册设置 CPOL/CPHA、MSB first 和 8-bit data size。
4. 先用不超过器件上限的低速，例如 500 kHz 或 1 MHz。
5. 再添加一个 GPIO 输出作为 CS，初始输出设为高。
6. 保存并查看生成的 `ti_msp_dl_config.h`。记住实际宏名，例如 `SPI_0_INST`、`GPIO_EXT_PORT`、`GPIO_EXT_CS_PIN`；下面示例中的名字必须替换为你的生成宏。

这里统一使用 8-bit SPI，即 16-bit 帧拆成两个字节发送。这样不依赖特定 SPI FIFO 的 16-bit 配置，最适合首次 bring-up。

## 4. 直接 DriverLib 写一个 16-bit 帧

### 【比赛现场直接复制】

```c
static uint8_t spi_txrx8(SPI_Regs *spi, uint8_t value)
{
    DL_SPI_transmitDataBlocking8(spi, value);
    return DL_SPI_receiveDataBlocking8(spi);
}

static void spi_write16_msb(SPI_Regs *spi,
                            GPIO_Regs *cs_port,
                            uint32_t cs_pin,
                            uint16_t word)
{
    DL_GPIO_clearPins(cs_port, cs_pin);
    (void)spi_txrx8(spi, (uint8_t)(word >> 8));
    (void)spi_txrx8(spi, (uint8_t)word);
    while (DL_SPI_isBusy(spi)) {
    }
    DL_GPIO_setPins(cs_port, cs_pin);
}
```

调用：

```c
int main(void)
{
    SYSCFG_DL_init();
    DL_GPIO_setPins(GPIO_EXT_PORT, GPIO_EXT_CS_PIN);
    spi_write16_msb(SPI_0_INST, GPIO_EXT_PORT, GPIO_EXT_CS_PIN, 0x1234U);
    while (1) {
    }
}
```

仓库已有同一写法的公共实现：`00_common/mspm0_blocking_bus.c/.h`，正式器件驱动优先复用 `MSPM0_EXT_SPI_Write16MSB()`，不要复制第二份总线代码。

## 5. 读寄存器

读操作通常先发“读命令 + 地址”，然后继续发 dummy 字节以产生时钟：

```c
DL_GPIO_clearPins(GPIO_EXT_PORT, GPIO_EXT_CS_PIN);
(void)spi_txrx8(SPI_0_INST, read_command);
uint8_t value = spi_txrx8(SPI_0_INST, 0x00U);
while (DL_SPI_isBusy(SPI_0_INST)) {
}
DL_GPIO_setPins(GPIO_EXT_PORT, GPIO_EXT_CS_PIN);
```

dummy 字节数量、读命令格式以及 CS 是否要保持低，必须以具体器件数据手册为准。

## 6. 首次验证顺序

1. 上电前量电源与共地；2. CS 空闲为高；3. 只发一条固定命令；4. 逻辑分析仪检查模式、位序、字节数和 CS；5. 能读 ID/寄存器的器件先做回读；6. 最后再把 SCLK 提高。

常见错误：把 PICO/POCI 接反、CS 每个字节都抬高、SPI 模式错、16-bit 字节顺序反、生成宏名没有替换、未等待最后一位真正发完便抬高 CS。

