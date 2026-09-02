# 比赛常用功能代码审计与归档

## 1. 审计结论

本次检查针对信号类邀请赛最常重复编写的应用逻辑。原来这些代码主要集中在
`CONTEST_FUNCTIONAL_CODE_COOKBOOK.md`，模块 README 只给出接口和边界，现场查找不够
直接。现在保留统一手册作为总索引，并把可直接复制的最小代码同步放到对应模块 README。

| 功能 | 归属 | 现在应查看的位置 | 是否需要改模块 `.c/.h` |
|---|---|---|---|
| 动态采样率 | 双路 ADC + 应用层 | `02_acquisition/adc_dual_sync/README.md` 第 20 节；总手册第 2 节 | 否 |
| 动态 Y 轴/自动量程 | 波形算法 + 应用层 | `01_bsp/tft_waveform/README.md` 第 19 节；总手册第 3 节 | 否 |
| TFT 屏幕分页 | 显示驱动 + 应用层 | `12_external_devices/display/st7789/README.md` 第 9 节；总手册第 4 节 | 否 |
| ILI9341 分页 | 显示驱动 + 应用层 | 总手册第 4 节（函数名为 `TFT_ILI9341_*`） | 否 |
| 矩阵键盘扫描/消抖/鬼键 | 矩阵键盘模块 | `01_bsp/matrix_keypad_4x4/README.md` 第 10.1、18.3 节 | 否 |
| 数字键盘预输入 | 矩阵键盘输出 + 应用层 | `01_bsp/matrix_keypad_4x4/README.md` 第 20 节；总手册第 5 节 | 否 |
| 参数确认/取消/溢出保护 | 应用层状态机 | 矩阵键盘 README 第 20 节；总手册第 5 节 | 否 |
| 采集超时与错误页 | ADC 状态机 + 应用层 | 总手册第 6 节 | 否 |
| ISR 与主循环边界 | 所有实时模块 | 总手册第 7 节 | 否 |

## 2. 使用顺序

1. 从目标模块 README 复制 `.c/.h` 和 `README_MINIMAL_EXAMPLE.c` 的正常初始化/调用顺序。
2. 按 README 的 SysConfig 点击路径配置硬件，并让 CCS 重新生成 `ti_msp_dl_config.c/.h`。
3. 只把本审计表中标为“应用层”的代码复制到 `main.c`；把示例变量替换成题目变量。
4. 先完成一帧采集和显示，再加入动态采样率、自动量程、分页或预输入，不要一次把所有
   状态机塞进中断。
5. 每个工程步骤文档都要写清：模块 README 复制了什么、SysConfig 生成了什么、应用层
   自写了什么，以及每个变量/边界判断的作用。

## 3. 归属边界

- `SignalDualADC_SetSampleRate()`、`GetConfiguredRate()`、`SignalTFTWaveform_Draw()`、
  `MapY()`、`SignalMatrixKeypad4x4_ReadNewSymbol()` 和 `TFT_ST7789_*` 图形 API 是模块
 公开能力，应按模块 README 使用。
- “根据频率计算目标 Fs”“扫描数组计算显示量程”“页面枚举与 dirty 标志”“数字缓冲、
  退格、确认、取消”是题目相关组合逻辑，保留在 `main.c`，不塞进冻结驱动。
- ST7789 当前只有图形 API，没有内置文字 API。不能把 ILI9341 的 `DrawString` 或
  `DrawInt32` 直接复制到 ST7789 工程；需要文字时另选字库模块并记录模块差异。
- SysTick/ADC/DMA ISR 只做计数、置标志、发布 buffer 或读取稳定键值；TFT、FFT、浮点和
  大循环放在主循环或低频任务。

## 4. 本次改动范围

已新增/补充的文档：

- `02_acquisition/adc_dual_sync/README.md` 第 20 节；
- `01_bsp/tft_waveform/README.md` 第 19 节；
- `01_bsp/matrix_keypad_4x4/README.md` 第 20 节；
- `12_external_devices/display/st7789/README.md` 第 9、10 节；
- 本审计文件。

没有修改任何正式模块的 `.c/.h`，没有手工修改 SysConfig 生成文件，也没有声称完成
实物验证。工程若使用 ST7789，仍需先做纯色、矩形、窗口和旋转测试；当前统一状态为
`BOARD NOT_RUN`。
