# External Device Catalog

本页列出当前外部器件库的真实入口。它不是全世界芯片清单；目录里只有分类 README 的条目仍属于参考资料，不能冒充具体驱动。

## 1. 通用控制类型

| 类型 | 典型识别特征 | MSPM0 初次实现 | Recipe |
|---|---|---|---|
| SPI Register | command/register/data | SPI blocking + GPIO CS | [打开](recipes/SPI_REGISTER_DEVICE_RECIPE.md) |
| SPI Streaming ADC | CS 后固定 clocks 返回 raw | SPI blocking + GPIO CS | [打开](recipes/SPI_STREAMING_ADC_RECIPE.md) |
| I2C Register | 7-bit address + register | I2C blocking | [打开](recipes/I2C_REGISTER_DEVICE_RECIPE.md) |
| Parallel ADC | D bus + CONVST/BUSY/RD | GPIO blocking | [打开](recipes/PARALLEL_ADC_RECIPE.md) |
| GPIO Controlled | select/enable/control | 直接 DriverLib GPIO | [打开](recipes/GPIO_CONTROLLED_DEVICE_RECIPE.md) |
| 3-Wire GPIO | 非标准 CS/U-D/INC | GPIO bit-bang | [打开](recipes/THREE_WIRE_GPIO_DEVICE_RECIPE.md) |
| Analog Voltage | VCTRL | DAC + 模拟缓冲 | [打开](recipes/ANALOG_VOLTAGE_CONTROLLED_DEVICE_RECIPE.md) |
| PWM Controlled | duty/frequency | Timer PWM | [打开](recipes/PWM_CONTROLLED_DEVICE_RECIPE.md) |

## 2. 具体器件

| 类别 | 器件 | 关键能力 | 实现形态 | 状态 | 路径 |
|---|---|---|---|---|---|
| ADC | ADS112C04 | 16-bit、4ch、2 kSPS、PGA、I2C | `.c/.h` + common bus | DOC + CODE_COMPILE | [README](../adc/ads112c04/README.md) |
| ADC | ADS7887 | 10-bit、1.25 MSPS、SPI SAR | `.c/.h` + common bus | DOC + CODE_COMPILE | [README](../adc/ads7887/README.md) |
| ADC | ADS7866 | 12-bit、200 kSPS、SPI SAR | `.c/.h` + common bus | DOC + CODE_COMPILE | [README](../adc/ads7866/README.md) |
| DAC | DAC7811 | 12-bit multiplying/current output | `.c/.h` + common bus | DOC + CODE_COMPILE | [README](../dac/dac7811/README.md) |
| DDS | AD9833 | 28-bit FTW、正弦/三角/方波 | `.c/.h` + common bus | DOC + CODE_COMPILE | [README](../dds/ad9833/README.md) |
| DDS | AD9850 | 32-bit FTW、5-bit phase | core + MSPM0 GPIO platform | DOC + CODE_COMPILE | [README](../dds/ad9850/README.md) |
| PGA | PGA113 | 2ch、1～200 倍、SPI | `.c/.h` + common bus | DOC + CODE_COMPILE | [README](../programmable_gain/pga113/README.md) |
| Digital Pot | TPL0401A-10 | 10 kΩ、128 position、I2C | `.c/.h` + common bus | DOC + CODE_COMPILE | [README](../digital_pot/tpl0401a_10/README.md) |
| Digital Pot | X9C104S | 100 kΩ、100 tap、NVM | README 直接 GPIO | DOC | [README](../digital_pot/x9c104/README.md) |
| Analog MUX | CD4052B/CD4053B | dual 4:1 / triple 2:1 | README 直接 GPIO | DOC | [README](../analog_switch/cd4052_cd4053/README.md) |
| Analog Switch | CD4066B | 4×SPST bilateral | README 直接 GPIO | DOC | [README](../analog_switch/cd4066b/README.md) |
| High-Voltage MUX | MAX14752 | 8:1、最高 72 V 供电域 | README 直接 GPIO | DOC | [README](../analog_switch/max14752/README.md) |
| GPIO Expander | TCA6408A | I2C 8-bit GPIO、INT/RESET | `.c/.h` + common bus | DOC + CODE_COMPILE | [README](../gpio_expander/tca6408a/README.md) |
| Display | WH-X096-2864KSWEG01-A4 / SSD1306 | 128×64 单色、4-pin I2C | core + MSPM0 I2C adapter + MSPM0G3507 binding + common bus | DOC + CODE_COMPILE + COPY_READY | [README](../display/ssd1306/README.md) |
| Display | GMT024-01 / ST7789V2 | 240×320、16-bit RGB565、7PIN SPI、ASCII 字库 | `.c/.h` 核心 + 字库 + MSPM0G3507 SPI 平台适配 | CODE_COMPILE + COPY_READY；精确电气待确认 | [README](../display/st7789/README.md) |

表中 `DOC`/`CODE_COMPILE` 分别代表完整状态名 `DOC_VERIFIED`/`CODE_COMPILE_VERIFIED`；没有任何一行标成 `BOARD_VERIFIED`。

## 3. 现有参考入口（不是本轮具体驱动）

- ADC：`ad7606_family`、`generic_spi_adc`；
- DAC：`generic_spi_dac`、`tlc5615_family`；
- DDS：`generic_spi_dds`；
- 数字电位器：`generic_spi_digital_pot`、`x9c_family`；
- 模拟开关：`cd4051_74hc4051`；
- PGA/VGA：`ad603_control_adapter`；
- 其他分类：输入、继电器、存储、电平转换、隔离、传感器、电源控制等目录 README。

这些入口只能帮助分类或核对 Datasheet，不能因目录名存在就写成 `CODE_COMPILE_VERIFIED`。

## 4. 公共代码边界

`00_common/mspm0_blocking_bus.c/.h` 是首次 bring-up 的阻塞 SPI/I2C helper，不是独立器件。它集中真实 DriverLib 收发，避免每个串行驱动复制总线循环。简单 GPIO 器件不使用它，也不创建空壳 driver。

陌生 SAR ADC 使用 [SPI SAR ADC Migration Guide](SPI_SAR_ADC_MIGRATION_GUIDE.md)。
