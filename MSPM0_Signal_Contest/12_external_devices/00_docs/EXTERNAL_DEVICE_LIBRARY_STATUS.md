# External Device Library Status

更新时间：2026-08-17。器件证据状态只使用：`DOCUMENTATION_ONLY`、`DOC_VERIFIED`、`CODE_COMPILE_VERIFIED`、`BOARD_VERIFIED`；`COPY_READY` 另表示隔离应用已 generate/compile/full link。

## 1. 状态定义

| 状态 | 证据要求 |
|---|---|
| `DOCUMENTATION_ONLY` | 已整理资料和迁移边界，但精确型号、电气、协议或正式目标驱动仍有缺口 |
| `DOC_VERIFIED` | 厂商官方 product page/datasheet 已核对供电、Pin、接口、时序、寄存器/命令，并写入 README |
| `CODE_COMPILE_VERIFIED` | 正式目标 `.c` 使用 TI Arm Clang、MSPM0G3507 define、MSPM0 SDK 与 CMSIS include 编译通过 |
| `BOARD_VERIFIED` | 真实器件/模块已接线，上电和功能结果由示波器、逻辑分析仪、万用表或可复查运行记录确认 |

`CODE_COMPILE_VERIFIED` 仅表示本轮目标源码编译，不等于完整 Application link，更不等于上板。

## 2. 编译环境与结果

| 项目 | 本轮证据 |
|---|---|
| Compiler | TI Arm Clang `5.1.1.LTS` |
| Target | Cortex-M0+，`__MSPM0G3507__` |
| SDK | `C:\TI\mspm0_sdk_2_11_00_07` |
| CMSIS | SDK `source/third_party/CMSIS/Core/Include` |
| Flags | C11、`-Wall -Wextra -Werror` |
| Result | 16/16 正式目标 `.c` source compile PASS，0 warning；SSD1306 与 ST7789 另完成隔离 COPY TEST |
| Board | NOT RUN |

详细清单见 [compile report](../90_validation/EXTERNAL_DEVICE_COMPILE_REPORT.md)。

## 3. 具体器件状态

| Device | DOC | CODE COMPILE | BOARD | 备注 |
|---|---|---|---|---|
| PGA113 | PASS | PASS | NOT RUN | SPI blocking |
| TPL0401A-10 | PASS | PASS | NOT RUN | I2C blocking |
| X9C104S | PASS | N/A | NOT RUN | README 直接 GPIO，无 `.c/.h` |
| CD4052B/CD4053B | PASS | N/A | NOT RUN | README 直接 GPIO |
| CD4066B | PASS | N/A | NOT RUN | README 直接 GPIO |
| MAX14752 | PASS | N/A | NOT RUN | 高压器件；必须硬件安全复核 |
| TCA6408A | PASS | PASS | NOT RUN | I2C blocking |
| DAC7811 | PASS | PASS | NOT RUN | SPI；必须外部 I/V 运放 |
| ADS112C04 | PASS | PASS | NOT RUN | I2C precision ADC |
| ADS7887 | PASS | PASS | NOT RUN | SPI 10-bit SAR |
| ADS7866 | PASS | PASS | NOT RUN | SPI 12-bit SAR |
| AD9833 | PASS | PASS | NOT RUN | SPI DDS |
| AD9850 | PASS | PASS | NOT RUN | GPIO serial DDS；既有 PC test 未在本轮重写 |
| WH-X096-2864KSWEG01-A4 / SSD1306 | PASS | PASS | NOT RUN | 4-pin I2C OLED；另通过 COPY TEST |
| GMT024-01 / ST7789V2 | 待实物确认 | PASS | NOT RUN | 240×320 SPI TFT；核心/平台 `.c/.h` 已编译并通过隔离 COPY TEST，精确电气、偏移和上板待确认 |

## 4. 正式源码编译清单

- `00_common/mspm0_blocking_bus.c`
- `adc/ads112c04/ads112c04.c`
- `adc/ads7887/ads7887.c`
- `adc/ads7866/ads7866.c`
- `dac/dac7811/dac7811.c`
- `dds/ad9833/ad9833.c`
- `dds/ad9850/ad9850.c`
- `dds/ad9850/ad9850_mspm0_platform.c`
- `digital_pot/tpl0401a_10/tpl0401a_10.c`
- `gpio_expander/tca6408a/tca6408a.c`
- `programmable_gain/pga113/pga113.c`
- `display/ssd1306/ssd1306.c`
- `display/ssd1306/ssd1306_mspm0_i2c.c`
- `display/ssd1306/ssd1306_mspm0g3507.c`
- `display/st7789/signal_tft_st7789.c`
- `display/st7789/signal_tft_st7789_mspm0g3507.c`
- `display/st7789/signal_tft_st7789_font.c`

## 5. 升级到 BOARD_VERIFIED 的统一条件

逐器件保存：实物型号/模块正反面、供电实测、接线表、SysConfig 生成宏、逻辑分析仪或模拟波形、固定参数实测值、重复上电结果。只升级实际测过的器件，不批量升级一个类别。
