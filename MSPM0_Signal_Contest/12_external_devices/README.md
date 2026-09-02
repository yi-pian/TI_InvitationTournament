# External Device Library：外部器件库

README 类型：`LIBRARY_INDEX`

本目录只负责 MSPM0G3507 片外器件：接线、SysConfig、DriverLib 调用、最小 bring-up 和验证。MCU 内部 ADC/DAC/Timer、正式算法、ILI9341 等原有模块仍使用原目录的唯一源码，不在这里复制。

## 先判断器件属于哪种控制方式

| Datasheet 特征 | 先看哪个 Recipe |
|---|---|
| SPI 命令/寄存器，CS 下发送若干字节 | [SPI Register Device](00_docs/recipes/SPI_REGISTER_DEVICE_RECIPE.md) |
| SPI ADC，CS 后连续移出采样结果 | [SPI Streaming ADC](00_docs/recipes/SPI_STREAMING_ADC_RECIPE.md) |
| I2C 地址 + 寄存器/命令 | [I2C Register Device](00_docs/recipes/I2C_REGISTER_DEVICE_RECIPE.md) |
| SPI 显示控制器，带 DC/CS/RST/BL | [ST7789 可复用模块](display/st7789/README.md)；陌生显示模块先看 [小白教程](00_docs/ST7789_UNKNOWN_MODULE_TUTORIAL.md) |
| D0..Dn + CONVST/BUSY/RD | [Parallel ADC](00_docs/recipes/PARALLEL_ADC_RECIPE.md) |
| 地址/使能脚直接选通 | [GPIO Controlled Device](00_docs/recipes/GPIO_CONTROLLED_DEVICE_RECIPE.md) |
| CS/U-D/INC 等非标准三线时序 | [3-Wire GPIO Device](00_docs/recipes/THREE_WIRE_GPIO_DEVICE_RECIPE.md) |
| 用控制电压改变增益/频率/衰减 | [Analog Voltage Controlled Device](00_docs/recipes/ANALOG_VOLTAGE_CONTROLLED_DEVICE_RECIPE.md) |
| 接受占空比，或 PWM+RC 变电压 | [PWM Controlled Device](00_docs/recipes/PWM_CONTROLLED_DEVICE_RECIPE.md) |

陌生 SPI SAR ADC 直接看 [迁移指南](00_docs/SPI_SAR_ADC_MIGRATION_GUIDE.md)：通常保留 SPI Tx/Rx 与 CS，只重新确认 mode、frame、bit、timing 和 command。

## 已整理的具体器件

| 类别 | 器件 | 控制方式 | 使用入口 |
|---|---|---|---|
| 精密 ADC | ADS112C04 | I2C | [README](adc/ads112c04/README.md) |
| SAR ADC | ADS7887 | SPI，10-bit/1.25 MSPS | [README](adc/ads7887/README.md) |
| SAR ADC | ADS7866 | SPI，12-bit/200 kSPS | [README](adc/ads7866/README.md) |
| DAC | DAC7811 | SPI，电流输出 | [README](dac/dac7811/README.md) |
| DDS | AD9833 | SPI 16-bit | [README](dds/ad9833/README.md) |
| DDS | AD9850 | 4-GPIO / 40-bit 串行 | [README](dds/ad9850/README.md) |
| PGA | PGA113 | SPI | [README](programmable_gain/pga113/README.md) |
| 数字电位器 | TPL0401A-10 | I2C | [README](digital_pot/tpl0401a_10/README.md) |
| 数字电位器 | X9C104S | CS/U-D/INC GPIO | [README](digital_pot/x9c104/README.md) |
| 模拟 MUX | CD4052B/CD4053B | GPIO | [README](analog_switch/cd4052_cd4053/README.md) |
| 模拟开关 | CD4066B | GPIO | [README](analog_switch/cd4066b/README.md) |
| 高压 MUX | MAX14752 | GPIO | [README](analog_switch/max14752/README.md) |
| GPIO 扩展 | TCA6408A | I2C | [README](gpio_expander/tca6408a/README.md) |
| 显示 | WH-X096-2864KSWEG01-A4 / SSD1306 | 128×64、4-pin I2C | [README](display/ssd1306/README.md) |
| 显示 | GMT024-01 / ST7789 | 240×320、RGB565、7PIN SPI（代码已编译，上板待确认） | [README](display/st7789/README.md) |

## 把串行驱动加入工程

1. 链接具体器件目录中的唯一 `.c`，不要复制进 Application。
2. 除 AD9850 自带 platform 外，新串行驱动还链接 `00_common/mspm0_blocking_bus.c`。
3. Include path 加具体器件目录和 `12_external_devices/00_common`。
4. 按器件 README 配 `.syscfg`；不编辑生成的 `ti_msp_dl_config.c/.h`。
5. 调用 `SYSCFG_DL_init()`，再调用 README 的最小 API。
6. 首次用 blocking；协议和电气正确后才升级 DMA/interrupt。

GPIO 简单器件不需要 `.c/.h`。直接复制 README 的几行 DriverLib GPIO 代码，并替换为本工程 SysConfig 生成宏。

## 初学者使用顺序

```text
确认完整芯片型号/模块原理图
→ 打开对应 README 或通用 Recipe
→ 断电接线并核对电压
→ 按 README 配 SysConfig
→ 加入唯一源码（若有）
→ 复制最小 main
→ 低速/固定参数 bring-up
→ 逻辑分析仪、示波器或万用表验证
→ 再接入比赛 Application
```

完整目录看 [EXTERNAL_DEVICE_CATALOG](00_docs/EXTERNAL_DEVICE_CATALOG.md)，验证证据看 [EXTERNAL_DEVICE_LIBRARY_STATUS](00_docs/EXTERNAL_DEVICE_LIBRARY_STATUS.md)，README 教程完整性看 [EXTERNAL_README_COMPLETENESS_AUDIT](00_docs/EXTERNAL_README_COMPLETENESS_AUDIT.md)。

## 状态规则

- `DOC_VERIFIED`：引脚、电压、协议、时序和关键寄存器已按厂商官方资料复核。
- `CODE_COMPILE_VERIFIED`：正式 `.c` 已用当前 TI Arm Clang + MSPM0 SDK 做目标源码编译。
- `BOARD_VERIFIED`：真实器件接线并有仪器/运行结果。当前本轮没有任何新器件达到该状态。
