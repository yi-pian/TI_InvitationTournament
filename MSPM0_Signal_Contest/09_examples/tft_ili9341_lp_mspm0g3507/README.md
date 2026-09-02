# LP-MSPM0G3507 + ILI9341 彩条与文字示例

这个示例来自放入工作区的真实 TFT 工程，现已改为通过 projectspec **链接正式唯一驱动**：

- 模块说明：[TFT ILI9341 README](../../01_bsp/tft_ili9341/README.md)
- Platform 说明：[MSPM0G3507 TFT Platform](../../08_applications/common/mspm0g3507/README.md)
- `main.c` 是 `【COMPILE-VERIFIED EXAMPLE】`，由 Platform Minimal Example 回归执行 final link。

```text
main.c
  ↓ callbacks
01_bsp/tft_ili9341/signal_tft_ili9341.c
  + signal_tft_ili9341_font_data.inc
  ↓
SPI1 + GPIO → ILI9341
```

## 1. 你需要什么

- LP-MSPM0G3507 LaunchPad；
- 带 SPI 排针的 240×320 ILI9341 模块；
- 杜邦线；
- CCS、MSPM0 SDK 2.11.00.07、SysConfig；
- 最好有万用表，白屏排查时最好再有逻辑分析仪。

## 2. 接线

必须断电接线，并按你手上屏幕的丝印确认引脚顺序。

| 屏幕丝印 | LP-MSPM0G3507 | 40-pin 位置 | 说明 |
|---|---|---:|---|
| VCC | 3V3 | 电源排针 | 默认不用 5 V |
| GND | GND | 任一 GND | 必须共地 |
| SCK/CLK | PB9 | 7 | SPI1 SCLK |
| SDI/MOSI | PB8 | 15 | SPI1 PICO |
| CS | PB6 | 13 | SPI1 CS0，硬件片选 |
| DC/RS | PB15 | 17 | GPIO |
| BL/LED | PB12 | 非 40-pin 标准位置 | 只有 BL 是逻辑使能时才这样接 |
| SDO/MISO | 不接 | — | 当前只写屏 |
| RESET | 未接 MCU | — | 示例使用软件复位；新接线建议增加复位 GPIO |

如果你的 BL 是背光电源脚，禁止直接让 PB12 供电；应按屏板说明使用限流/三极管。完整电气说明见 [`../../01_bsp/tft_ili9341/README.md`](../../01_bsp/tft_ili9341/README.md)。

## 3. SysConfig 配置事实

[`tft_ili9341.syscfg`](tft_ili9341.syscfg) 配置了：

- `SPI1` Controller；
- mode 0（CPOL=0、CPHA=0）；
- 8 bit、MSB first；
- 8 MHz；
- PB9 SCLK、PB8 PICO、PB6 CS0；
- PB15 DC、PB12 BL；
- POCI 分配 PA16，但屏幕线不接。

SysConfig 是源文件；不要手改构建目录中的 `ti_msp_dl_config.c/h`。

## 4. 如何 Build

1. CCS 选择 `File → Import Projects...`；
2. 选择 `CCS Projects from ProjectSpec`；
3. 选择 [`ticlang/tft_ili9341_LP_MSPM0G3507_nortos_ticlang.projectspec`](ticlang/tft_ili9341_LP_MSPM0G3507_nortos_ticlang.projectspec)；
4. 确认 SDK 指向 `C:\TI\mspm0_sdk_2_11_00_07`；
5. Clean Project，再 Build Project；
6. 下载运行。

projectspec 对正式驱动使用 `action="link"`，因此 Project Explorer 里看到的模块文件是链接，不是复制品。

## 5. 正常现象

初始化后屏幕为黑底，显示红、绿、蓝三块竖向区域，边缘有白框；红色区域上还会以 8×16 字体显示 `MSPM0 SIGNAL`、`VPP=`、`1.25`，并显示 16×16 的“电子”示例字模。若需要先检查引脚电平，把 `main.c` 中 `g_pin_test_mode` 在调试器里改为 1 后复位；此模式不会初始化屏幕，只把五根信号线拉高供万用表检查。

## 6. 白屏排查顺序

1. 量 VCC 是否约 3.3 V，GND 是否共地；
2. 确认背光亮不等于控制器已经工作；
3. 确认屏幕 `SDI` 接 PB8，不是接到 MISO；
4. 检查 CS/DC 是否接反；
5. 把 SPI 时钟从 8 MHz 降到 1 MHz；
6. 逻辑分析仪检查 mode 0 和初始化命令；
7. 若模块必须硬复位，再分配一个 GPIO 接 RESET 并填写 `set_reset` 回调。

## 7. 验证状态与旧证据

- 原始工程曾由 TI Arm Clang 5.1.1.LTS 完整链接，旧 map 显示 Flash 4,728 B、SRAM used 597 B（含 512 B stack）。
- 加入四套 ASCII 字库、“电子”示例字模和文字调用后，正式模块已重新执行 SysConfig generate、TI Arm Clang compile 和 full link：Flash 21,016 B、SRAM used 597 B（含 512 B stack），map 位于 `10_tests/tft_ili9341/build/`。
- 原 CCS 生成目录和 map 被原样保存在 `legacy_build_evidence/`，只用于追溯，不要把其中生成的 `ti_msp_dl_config.*` 当源文件。
- 本轮没有观察实屏结果，仍是 `BUILD_VERIFIED`，不是 `BOARD_VERIFIED`。
