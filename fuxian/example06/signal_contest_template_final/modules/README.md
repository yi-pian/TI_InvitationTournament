# modules

把模块 README 列出的 `.c/.h/.inc` 和少量公共头文件复制到本目录。本题采集输入只有一路，使用 `02_acquisition/adc_dma` 单 ADC 模块，不使用双 ADC 模块。

本母版 Include Path 已包含 `${PROJECT_ROOT}/modules`。复制后：

1. 在 CCS Project Explorer 右键工程并 Refresh；
2. 确认新 `.c` 文件没有 Exclude from Build；
3. 在 `main.c` include 模块主头文件；
4. 立即 Build 一次。

不要把正式仓库的 Platform/Adapter 整目录搬进来；只复制当前模块 README 明列的文件。
# example06 应用组合记录（在正式模块 README 之后补充）

本工程严格按比赛顺序组合模块，模块源文件只从 `MSPM0_Signal_Contest` 原样复制，
不修改 `.c/.h/.inc`。

## 1. 选择和复制

1. `02_acquisition/adc_dma`：复制 `signal_adc_dma.c/.h`，以及公共
   `01_bsp/common/signal_status.h`。`g_raw[]` 保存题目的一路混合输入波形。
2. `12_external_devices/display/st7789`：复制核心、MSPM0G3507 平台层和
   `signal_tft_st7789_font.c/.h/font_data.inc`，字库调用固定为 `TFT_ST7789_FONT_8X16`。
3. `01_bsp/matrix_keypad_4x4`：复制 `signal_matrix_keypad_4x4.c/.h`，使用 README 的
   `SignalMatrixKeypad4x4_ReadNewSymbol()` 固定引脚便利接口。
4. `03_measurement/frequency_zero_cross`：复制 `signal_zero_cross.c/.h` 和公共
   `03_measurement/common/signal_algorithm_status.h`；它不需要 SysConfig。
5. `05_precision/zero_cross_interpolation`：复制 `signal_zero_cross_interpolation.c/.h`；
   README 要求它与上一步的 ZeroCross 同时链接，不需要 SysConfig。
6. FFT 不复制模块，直接使用母版已启用的 CMSIS-DSP `arm_rfft_fast_f32()`。

## 2. 本题 SysConfig 补充步骤

正式模块 README 已分别给出单模块配置；母版没有“多模块组合”配置，因此在这里补充
组合顺序。打开 `signal_contest_template.syscfg` 后按 GUI 添加并命名：

| 资源 | 实例/组名 | 关键配置 |
|---|---|---|
| ADC12 | `SIGNAL_ADC` | ADC0，PA25/通道2，Event、Repeat、Memory0 DMA done |
| DMA | `SIGNAL_ADC_DMA` | DMA_CH0，Half Word，Peripheral-to-Block，Single |
| TIMER | `SIGNAL_SAMPLE_TIMER` | TIMG0，BUSCLK/1/1，周期 2 us，ZERO_EVENT publisher，Event 1 |
| SPI | `SPI_TFT` | SPI1，SCLK PB9，MOSI PB8，硬件 CS PB6 |
| GPIO | `GPIO_TFT_CTRL` | `TFT_DC` PB15、`TFT_BLK` PB12 输出 |
| GPIO | `GPIO_KEYPAD` | 行 PB16/PB0/PB7/PB17 输出；列 PB18/PB13/PB20/PB4 上拉输入 |

ADC subscriber channel 1 使用 Timer 的 ZERO_EVENT publisher 1。这里的引脚
和资源沿用正式 profile 与 ST7789 README 的已验证示例；若实际接线更改，只在 SysConfig
中换成合法且不冲突的 ADC/GPIO 映射，并保持生成宏名称不变。不要手改生成的
`ti_msp_dl_config.c/.h`。

## 3. main 粘贴顺序

按比赛小题依次粘贴：

1. 单 ADC：复制 `signal_adc_dma.h` 的配置结构和 README 的
   `SignalADC_Init -> SetSampleRate -> Start -> IsFinished` 调用顺序。
2. ST7789：复制平台 README 的 `SignalTFTST7789_MSPM0_Init()` 调用和字库的
   `TFT_ST7789_DrawString/DrawFloat/DrawInt32` 形状。
3. 矩阵键盘：复制 README 规定的 5 ms 调用 `ReadNewSymbol`，应用逻辑仅接受 `'1'` 到 `'5'`
   作为显示周期数。
4. 过零链：复制 README 的 `SignalZeroCross_Process -> SignalZeroCrossInterpolation_Process`
   调用顺序；应用层仅从相邻上升过零点中选择最接近 FFT 锁定目标的一对，作为显示起点与周期。
5. 测量逻辑：少量自写代码完成去直流、Hann、CMSIS RFFT、目标频带峰值搜索、I/Q 幅相、
   自动量程和波形坐标映射；FFT 算子本身不是自写模块。

## 4. 题目边界

10 倍干扰若在目标搜索带外，10--100 kHz 限频搜索可排除；若干扰也在同一搜索带且没有
参考通道/先验频率，仅凭一个混合 ADC 输入无法从信息上唯一判定“目标”是哪一个峰，文档不
虚假宣称能在该条件下必然分离。
