# example06 总步骤复现说明

## 1. 从恢复母版开始

本轮以恢复后的 `signal_contest_template_final` 为唯一基线，没有沿用其他对话的生成文件或
文档。先读三个正式模块 README，再复制模块，最后配置 SysConfig 和 main。

## 2. 模块清单与复制结果

| 模块 | 复制文件 | 是否修改 |
|---|---|---|
| 单 ADC/DMA | `signal_adc_dma.c/.h` | 否 |
| 公共状态 | `signal_status.h` | 否 |
| 上升过零 | `signal_zero_cross.c/.h`、`signal_algorithm_status.h` | 否 |
| 过零插值 | `signal_zero_cross_interpolation.c/.h` | 否 |
| ST7789 | `signal_tft_st7789.c/.h`、`signal_tft_st7789_mspm0g3507.c/.h` | 否 |
| 8×16 字库 | `signal_tft_st7789_font.c/.h`、`signal_tft_st7789_font_data.inc` | 否 |
| 矩阵键盘 | `signal_matrix_keypad_4x4.c/.h` | 否 |

FFT 使用 SDK 自带 CMSIS-DSP，不复制 FFT 模块。

复制核对 SHA-256（源文件与 `modules/` 文件相同）：

```text
signal_adc_dma.c 3FC9D81DECD80146B66F2144608288346392C4140B0901B724017F1489BEA289
signal_adc_dma.h 765F1E299A11EC5FAF383C22F9EB1C15B66A0A5D49E1DCEB2B2AF63B07A1E96F
signal_status.h 21BFC92D4B3BD05E858D5C301D7E83B6F41CCCAB7FCA9B04159666AE66BBC55E
signal_zero_cross.c 3651814862070FE890F23E8D8C14258FB585D15E98B4FA4E1D53E074A35D2F77
signal_zero_cross.h 055E89AFB46E26B94E549414B6D72D7A59651C5F9BAC81DF1674D13FE6124DA8
signal_zero_cross_interpolation.c 062EF905011EFAB4768F52CEA962B561FD8C2834B4807A8461DD33C29706B715
signal_zero_cross_interpolation.h 727AFB43B80F15B03FA228185B9E763131FB4E6B48C65FF0A0AB28B27768C844
signal_algorithm_status.h DC2144BEA3C950E414D2719F477DD6B676D47689C0614BC5B2E46230D914329C
signal_tft_st7789.c 23B5928B38A1C12F02861216587F9DBFB6C86741D5C718A651F328E7E54808D9
signal_tft_st7789.h B6D00C92B2DBA6345A653A4896474DE491B30C6DA6EE3DF76F1E672FB2FFF479
signal_tft_st7789_mspm0g3507.c 51880C9584739FC60D8024BE82B762918609CDA9C971432E12B47EFD6DFD8BBD
signal_tft_st7789_mspm0g3507.h ED8E60306EB555AB86A10038110D2DD93DFE21C8427CAF5EAAC6D3535B67A1E0
signal_tft_st7789_font.c 006D5D6B2A135F75E47EEAFDD2885BAAEA509C58B971EFF7A3CBE4FD02AF81DD
signal_tft_st7789_font.h ADD7D4B2A4D47323348FF7309A24E8698AB3EC263194C02C7F0645275F62C0CD
signal_tft_st7789_font_data.inc D94197BF8720D15319D352C922C89BAAE21A3EF0701443AA086204461456CA9A
signal_matrix_keypad_4x4.c 561B76E2EEA585D6739B25D5F869B96919BA3948B8F95F97795BF5B865691918
signal_matrix_keypad_4x4.h F08E8B6B89F8047AA8CC6F6561D89A0BB0605B5771C60E73F9AF469DD9BC8203
```

## 3. SysConfig 是否按 README

是。单 ADC、DMA、Timer、ST7789 SPI/GPIO、键盘 GPIO 均按正式 README 的字段配置；由于
母版 README 没有“多模块组合”章节，已在 `modules/README.md` 补充组合顺序、资源表和
本题边界，再按该补充说明执行。生成配置文件没有手工编辑。

SysConfig CLI 结果：使用工程声明的 1.28.0 生成成功（returncode=0）。有 ADC 唤醒时间、
SPI 休眠保持和 DMA full-channel 提示，均不是生成错误；生成头文件已确认单 ADC、DMA、
TIMG0 和 Event 1 宏。

## 4. main 复制/自写对照

- 复制：ADC 模块 Init/SetSampleRate/Start/IsFinished 调用顺序；ST7789 平台初始化和
  字库 API 形状；键盘 `ReadNewSymbol()` API。
- 自写：`signal_config.h` 的 Fs/N/频带；`App_Measure`、`App_PrepareWave`、
  `App_DrawWave`、`App_DrawMeasurement`、`App_KeypadTask` 和错误处理。
- 没有自写或改动任何模块 `.c/.h/.inc`；没有在 ISR 中做浮点、FFT 或 SPI。
- 原双 ADC 文件仅作为未参与构建的历史备份保存在 `modules_legacy_dual/`，当前 `modules/`
  不含其 `.c`，因此 CCS 不会把双 ADC 链路编入工程。

`main.c` 逐行/逐段索引：1-8 为总流程注释；9-19 为标准库、模块和 CMSIS 头文件；21-30 为
ADC/FFT/绘图区常量；32-46 为静态 DMA、FFT、TFT 和键盘状态；48-54 为测量结果结构；
56-60 为失败后停机；62-127 为去直流、Hann、RFFT 频带搜索、I/Q 累加、幅相；129-158 为
周期窗口采样和自动量程；160-178 为 80% 高度波形连线；180-194 为键盘字符到 1-5 周期数；
196-225 为 8×16 字符和测量值绘制；227-268 为初始化；270-305 为“采集→等待→测量→显示→键盘”
主循环。主函数中的每个比赛步骤还在 229-236、252-266、271-304 逐段写明。

## 5. 逐步执行顺序

1. 单 ADC/DMA 采一帧 `g_raw[]`。
2. ST7789 初始化并用 8×16 字库显示 F/VPP/N。
3. 采用与 `22_X` 一致的 SysTick 调度：每 1 ms 计时、每 5 ms 读取矩阵键盘，数字 1～5 设置显示周期数。
4. Hann + CMSIS RFFT 搜索 10～100 kHz，用对数功率抛物线细化频率；I/Q 计算峰峰值并重构目标波形。
5. 自动量程与波形映射，波形高度目标约为绘图区 80%。

## 6. 验证记录

- 复制文件 SHA-256 已在复制阶段逐文件比对；`COPIED_MODULES.md` 记录来源。
- TI Arm Clang `-Wall -Wextra -Werror -fsyntax-only`：main、SysConfig 生成文件和全部
  复制模块均 PASS。
- 单 ADC 源码、SysConfig CLI 和 TI Arm Clang 语法检查已通过；旧 Debug 目录曾缓存双 ADC
  对象清单，已保留备份并移走，需在 CCS Refresh 后重新生成 Debug 再做完整链接。
- 当前尚未进行实板采样/屏幕/键盘验证；因此不能把结果升级为
  `BOARD_VERIFIED` 或 `CONTEST_VERIFIED`。
- CCS 测试顺序、当前 `tmp/syscfg` 重复链接错误的清理方法、接线和信号源验收项目见
  `example06_05_测试步骤与故障排查.md`。
