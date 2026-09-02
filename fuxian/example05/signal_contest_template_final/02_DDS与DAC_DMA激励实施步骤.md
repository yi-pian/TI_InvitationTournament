# example05 步骤四：AD9833 外部 DDS 扫频激励

> 文件名沿用旧步骤编号；本版不使用片内 DAC、DAC DMA 或软件 DDS 波表。

## 1. 选择和复制

按 `12_external_devices/dds/ad9833/README.md` 复制：

- `ad9833.c/.h`
- `12_external_devices/00_common/mspm0_blocking_bus.c/.h`

四个文件原样复制到 `modules/`，不得改动。`AD9833_Init()` 与 `AD9833_SetOutput()` 是 `main.c` 中调用代码的来源。

## 2. SysConfig

先按 AD9833 README 配置独立 SPI/FSYNC；屏幕 SPI1 必须保留，AD9833 不与其共用引脚：

1. 保留 `SPI_TFT`：SPI1，PB9=SCLK、PB8=MOSI，8 bit、MSB first、初始 Mode 0；PB6=CS0 只属于 ST7789。
2. 新增 `SPI_AD9833`：硬件实例选 SPI0；SCLK 选 PA12，MOSI/PICO 选 PA9；Controller、8 bit、MSB first、MOTO3、Mode 2（CPOL=1、CPHA=0）、方向 PICO、4 MHz。AD9833 不返回数据，故不配置 MISO 或硬件 CS。
3. 新增 GPIO 组 `GPIO_AD9833`，新增输出 `AD9833_FSYNC`，初始高，使用 PA8（BoosterPack 排针 4）。
4. 删除/不添加 `DAC12`、`SIGNAL_DAC_DMA`、`SIGNAL_DAC_TIMER`，也不配置 PA15 DAC 输出。
5. 保存后 Generate，核对 `SPI_TFT_INST`、`SPI_AD9833_INST`、`GPIO_AD9833_PORT` 和 `GPIO_AD9833_AD9833_FSYNC_PIN`。

AD9833 使用 Mode 2（CPOL=1、CPHA=0），ST7789 使用 Mode 0；两者现在分别使用 SPI0 和 SPI1，因此不需要临时切 SPI 模式，也不需要关闭 TFT 的 CS0。该改动只在 SysConfig 和 `main.c` 选择 AD9833 的 SPI 实例，不修改任意模块驱动。

## 3. 从 README 复制到 main

初始化调用来自 README：

```c
DL_GPIO_setPins(GPIO_AD9833_PORT, GPIO_AD9833_AD9833_FSYNC_PIN);
ok = AD9833_Init(SPI_AD9833_INST, GPIO_AD9833_PORT,
                 GPIO_AD9833_AD9833_FSYNC_PIN);
```

每一个扫频点的输出调用来自 README：

```c
ok = AD9833_SetOutput(SPI_AD9833_INST, GPIO_AD9833_PORT,
    GPIO_AD9833_AD9833_FSYNC_PIN, SIGNAL_AD9833_MCLK_HZ,
    output_hz, 0U, AD9833_WAVE_SINE);
```

逐行说明：第一行使 FSYNC 空闲为高；`Init` 写入 B28/RESET；`SetOutput` 使用实际模块 MCLK、当前整数 Hz、0 相位码和正弦输出，内部顺序写入两个 14-bit 频率字与 PHASE0，最后释放 RESET。返回 `false` 时主程序显示 `AD9833 ERROR` 并跳过该点。

`App_AD9833_Init()`、`App_AD9833_SetSine()` 只做 README API 的调用；Mode 2 已由独立 `SPI_AD9833` 的 SysConfig 固定。AD9833 不能回读频率，故相位和显示使用四舍五入后的整数请求频率；28-bit FTW 的量化误差相对本题步进很小。

## 4. 接线与本步验收

| AD9833 模块 | 接到哪里 |
|---|---|
| VCC | 3.3 V，先确认模块允许 3.3 V |
| GND/AGND/DGND | MSPM0 与模拟前端公共地 |
| SCLK | MSPM0 PA12（SPI0 SCLK） |
| SDATA | MSPM0 PA9（SPI0 MOSI/PICO） |
| FSYNC | MSPM0 PA8（BoosterPack 排针 4） |
| MCLK | 模块板载晶振/外部时钟，不接 MCU GPIO |
| VOUT | 先接示波器；经滤波、缓冲、衰减/偏置适配后再接激励前端/DUT |

本工程使用的 LP-MSPM0G3507 BoosterPack 接口中，PA8 是排针 4、PA12 是排针 32；PA9 位于排针 3，
需要将 J14 选择到 `PA9`，不能停在 `PB23`。本工程未配置 PA8 的 UART 功能，因此 PA8 可作为 FSYNC。

先只接供电和 SPI，示波器依次观察 1 kHz、100 kHz；检查频率、波形和 FSYNC 每个 16-bit 字的低脉冲。不得将 AD9833 `VOUT` 直接当作旧 DAC 电压：它没有软件可调幅度或直流偏置，若前端要求 1.65 V 中点，必须由外部电路加入。
