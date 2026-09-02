# 第四题：可编程信号综合测量与波形复现仪

本工程按 22_X 的比赛顺序组织：先选模块、复制模块文件、配置 SysConfig，再把模块 README 的最小示例 API 复制到 `main.c`，最后只补少量题目逻辑。功能包括 OUT 四种波形发生、CH1/CH2 双路同步 ADC、基本参数、三路测频、FFT 频谱和失真、抗毛刺、Sine Fit、Lock-In、单次捕获/复现以及两类校准。

## 分阶段实施顺序

1. **波形发生**：4×4 键盘选择 Sine/Square/Triangle/Sawtooth，输入频率、幅度、偏置；`Wave Table -> DDS -> DAC DMA -> OUT`。
2. **基本测量**：`CH1/CH2 -> ADC DMA -> ADC To Voltage -> Mean/MinMax/VPP/RMS/AC RMS/Statistics`，显示 DC、Vpp、RMS、AC RMS、Min、Max、Std、CLIPPING。
3. **三路测频**：Comparator + Timer Capture；ADC 去直流 + Zero Cross + Interpolation + Multi Cycle；FFT + Peak + Parabolic Interpolation。
4. **频谱失真**：Rect/Hann/Hamming/Blackman，基波、H2、H3、THD、SNR、SFDR。
5. **抗毛刺**：RAW/Median/Hampel，MAD、Robust Vpp、Robust RMS。
6. **精度增强**：FFT 粗频率 -> Sine Fit 3P/4P；已知 DDS 参考 -> Lock-In。
7. **单次捕获复现**：CAPTURE 页调用集成库 `single_capture_replay` 组合模块。按 `D` 武装连续 ADC；未知时刻到来的任意波第一次越过门限后，固定保存 416 点（1 MSPS 下为 416 us）窗口，模块自动裁剪首尾基线、保存到三个槽位并安全绘图，不以“下降回到门限”判断结束。`1/2/3` 选择三个保存槽位，`#` 循环回放当前槽位。
8. **系统校准**：DAC 0.5 V/2.5 V 回环得到 ADC gain/offset；CH1/CH2 同频相位得到固定延迟。

对应的比赛步骤文档按阶段拆分在上级目录 `fuxian/example04/`：

1. [01_模块选择与SysConfig.md](../01_模块选择与SysConfig.md)
2. [02_可编程波形发生.md](../02_可编程波形发生.md)
3. [03_CH1基本测量.md](../03_CH1基本测量.md)
4. [04_三种频率测量.md](../04_三种频率测量.md)
5. [05_FFT频谱与失真分析.md](../05_FFT频谱与失真分析.md)
6. [06_抗毛刺鲁棒测量.md](../06_抗毛刺鲁棒测量.md)
7. [07_SineFit与LockIn.md](../07_SineFit与LockIn.md)
8. [08_单次捕获与波形复现.md](../08_单次捕获与波形复现.md)
9. [09_系统校准与验证.md](../09_系统校准与验证.md)
10. [10_main自写组合逻辑逐行说明.md](../10_main自写组合逻辑逐行说明.md)

## SysConfig 配置

以 `MSPM0_Signal_Contest/09_examples/integration_profiles/PROFILE_06_FULL_SIGNAL/profile.syscfg` 为硬件底座，然后在 CCS SysConfig 图形界面追加：

| 资源 | 配置 |
|---|---|
| CH1 ADC | ADC0 / PA25 / DMA_CH0 |
| CH2 ADC | ADC1 / PA17 / DMA_CH2 |
| ADC 触发 | TIMG0，10 us 周期，双 ADC 同一事件 |
| OUT DAC | DAC12 + DMA_CH1，TIMG6，10 us 更新事件 |
| Comparator | COMP0，PA27，输出事件通道 4 |
| Timer Capture | TIMG7，2 ms 装载周期，订阅 FSUB0 通道 4；中断勾选 CC0_DN、CC1_DN、ZERO（CC1_DN 为匹配冻结 ISR 的补充） |
| ST7789 SPI | SPI1：SCLK PB9，MOSI PB8，CS PB6 |
| ST7789 控制 | DC PB15，背光 PB12 |
| 矩阵键盘 | 行 PB16/PB0/PB7/PB17；列 PB18/PB13/PB20/PB4，列上拉输入 |
| UART | UART0，PA10/PA11，115200（调试保留） |

这些设置与正式模块 README 的硬件要求一致；新增 `signal_tft_st7789_font` 字库模块只负责字模和调用封装，不改变 ST7789 原驱动，也不新增 SysConfig 资源。它复用了 22_X ILI9341 工程的四种 ASCII 点阵及两个示例汉字。生成后必须检查实例宏、DMA 通道、Timer LOAD 和 IRQ 名称；本目录没有代替 CCS Generate 生成文件。

## 键盘和页面

`1/2/3/4` 选择四种波形；`5/6/7` 选择 RAW/Median/Hampel；`8/9/0/#` 选择 Rect/Hann/Hamming/Blackman；`A` 翻页；普通页面中 `B/C/*` 分别输入频率、幅度、偏置；输入数字后按 `#` 提交、`D` 取消。CAPTURE 页中 `D` 武装并保存到下一个槽位，`1/2/3` 选择并回放 S1/S2/S3，`#` 再次回放当前槽位；校准页按 `D` 执行校准。

页面顺序为 `GEN -> MEASURE -> FREQ -> SPECTRUM -> ROBUST -> CAPTURE -> CAL`。动态区域刷新，静态蓝色边框和标题只在翻页时刷新。

## ST7789 字库

主界面已使用 `signal_tft_st7789_font.c/.h`，不再使用原来的 5×7 字符辅助。字模数据原样来自 22_X 的 ILI9341 字库，提供 6×12、8×16、12×24、16×32 四种 ASCII 字号以及“电”“子”两个 16×16 示例汉字；接口与调用示例见 [signal_tft_st7789_font_README.md](modules/signal_tft_st7789_font_README.md)。完整中文显示仍需要增加目标汉字点阵，再调用 `TFT_ST7789_DrawMonoBitmap()`。

## main 中的复制与自写边界

- 复制内容：各模块 README 中的 include、配置结构体初始化、`Init/Start/Process/GetResult` 调用及错误返回检查；同时补齐 RMS/AC RMS/Statistics 共同依赖的 `signal_math_backend.h` 与 `signal_math_backend_config.h`。
- 自写内容：按键状态机、数字预输入、页面状态、波形参数到波表的组合、双 ADC 两路电压转换、三种测频结果汇总、显示布局、触发后复现和校准流程串联。
- 自写代码原则：不改正式模块；只在 `main.c` 组合 API。每个阶段文档都列出关键自写代码的逐行含义。

## 当前验证边界

已完成源码和 SysConfig 源文件整理，正式模块 `.c/.h` 未修改。尚未完成 CCS 全量 Generate/Build、实板烧录、ST7789 显示、DAC 波形、Comparator/Timer Capture、ADC/双通道校准和 20 kHz 波形质量验证，当前状态为 **BOARD NOT RUN**。100 kSPS、512 点帧在 20 kHz 时每周期约 5 个 DAC 更新点，高频失真和外部模拟滤波必须上板实测。
# 单次任意波形三槽位捕获与循环回放（本次重做）

捕获页针对持续时间 50～200 us 的单次任意波形重新设计，使用现有集成库模块链：

```text
双 ADC DMA（三缓冲连续采集，捕获页 1 MSPS）
 -> signal_trigger_capture（在相邻 DMA 块拼接窗口中寻找第一次有效越限）
 -> signal_trigger_capture（固定提取 256 点窗口，不判断波形结束沿）
 -> 三个 raw 波形槽位
 -> signal_arbitrary_wave（线性重采样）
 -> signal_dac_dma（1 MSPS、repeat=true）
```

## 按键操作

- `D`：启动三缓冲连续 ADC，等待未知时刻的第一次有效越限，并把固定 256 点窗口保存到下一个槽位；保存满三次后循环覆盖最旧槽位。
- `1`、`2`、`3`：选择 S1/S2/S3；槽位有效时立即循环输出该波形。
- `#`：再次输出当前选中的槽位。
- `A`：切换页面。

屏幕显示当前槽位、保存状态和裁剪后的有效波形。每个槽位最多 256 点；程序会去掉有效波形前后的长直流基线，但不会删除中间经过基线的点。捕获页采样率固定为 1 MSPS，因此最大保存时间为 256 us，覆盖题目要求的 50～200 us。回放 DAC 更新率同步设置为 1 MSPS，回放周期约为 `有效保存点数 / 1 MSPS`。

## 复制模块与少量自写代码

复制的模块没有修改：`signal_dual_adc_mspm0g3507`、`signal_trigger_capture`、`signal_arbitrary_wave`、`signal_dac_dma_mspm0g3507`。自写部分仅包括三槽位数组、连续 DMA 相邻块拼接、槽位选择键和页面状态显示；模块的 ADC 连续采集、触发搜索、固定片段提取、线性重采样、DMA 循环输出仍按各自 README 调用。

捕获页使用 `signal_single_capture_replay`：ADC0/PA25 持续连续 DMA，内部 COMP0/PA27 输出边沿中断作为触发标记，模块完成拼接、提取、裁剪、三槽管理、ST7789 绘图和 DAC 回放。SysConfig 继续使用原 ADC DMA、DAC DMA、COMP0 和 Timer/Event 配置；main 只提供工程专属 COMP 回调和页面按键逻辑。COMP0 只告诉模块“波形已越过门限”，ADC0 始终采集原始波形，避免软件检测后再启动 ADC0 造成波头丢失。

输入信号接 PA27，COMP0 的内部 DAC 参考默认约 1.65 V；PA25 同时接同一原始模拟信号。捕获页的 `DUR` 显示裁剪后有效波形持续时间（微秒）。
