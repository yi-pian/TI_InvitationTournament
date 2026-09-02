# 步骤08：main.c 逐段解释与复制边界

本文件按当前 `main.c` 行号说明每一段代码。行号以本次提交的文件为准；若 CCS 自动格式化，按函数名定位即可。

| 行号 | 代码段 | 说明 | 来源/性质 |
|---|---|---|---|
| 20–36 | `#include` | 引入 DriverLib 生成头、题目参数和已复制模块头；`signal_dds.h` 用于把 1024 点查表重采样为 100 点循环输出。 | 模块 README 的 include，DDS 组合补充 |
| 38–77 | 宏、数组、模块对象 | `APP_SWEEP_MIN_HZ`、`APP_SWEEP_MAX_HZ`、`APP_TARGET_FREQUENCY_HZ`、`APP_HARMONIC_BASE_HZ` 是本题频率参数；`APP_KEYPAD_SCAN_PERIOD_MS=5` 复制 22_X 的扫描周期；`APP_WAVE_SAMPLES_PER_PERIOD=100` 固定每个输出周期的 DAC 更新点数，1 kHz 时为 100 kS/s、10 kHz 时为 1 MSPS；其余数组保存 ADC 输入/输出、DDS 查表源、DAC DMA 周期缓冲和锁相放大临时数组。 | 题目参数、缓冲区和周期点数为少量应用逻辑 |
| 79–114 | `App_GenerateDDS` | 先把 DAC 更新率设为“请求频率×100”并读回实际频率；再生成 1024 点查表，按实际更新率初始化 DDS、填充 100 点 DMA 缓冲，并把实际频率返回调用者。 | DAC DMA、Sine、DDS API 按 README；频率闭环封装是组合逻辑 |
| 113–125 | `App_Capture` | 第 121 行启动双 ADC DMA；第 123 行等待完成并睡眠；第 124 行返回成功。 | `SignalDualADC_Start/IsFinished` 按 README |
| 127–156 | `App_MeasureResponse` | 第 133–136 行准备 Lock-In 配置和结果；第 137 行采样；第 139–142 行把 12 位 ADC 码换算成电压；第 144–145 行分别处理输入、输出；第 149 行求幅值比；第 150–153 行相位相减并包到 ±180°。 | Lock-In 调用按 README；换算、比值、相位包络是题目逻辑 |
| 161–187 | `App_RunSweep` | 设置 0.5 kHz～10 kHz 对数扫频配置，生成频点；逐点生成 100 点完整周期、启动 DAC、采样测量、停止 DAC；实际频率写回表并置扫频完成标志。 | `SignalFrequencySweep_Generate`、DAC DMA 按 README；循环顺序是比赛步骤逻辑 |
| 184–226 | `App_Run1kCompensation` | 第 191–204 行把实测通道响应转换成逆增益和负相位；第 206–208 行调用校正模块插值到 1 kHz；第 210–213 行限制补偿增益；第 215–225 行输出预补偿波形，并以实际播放频率重新测量误差。 | 校正模块 API 按 README；逆表填充、限幅和误差计算是题目逻辑 |
| 237–293 | `App_RunHarmonics` | 依次测量 1、2、3 kHz，按“目标权重/实测增益”和负相位，以 `sin(nωt)` 合成 1024 点查表；正弦基准与单音发生模块一致，避免余弦导致三项相对相位改变。再以 100 点完整周期用 DDS 播放合成表，最后记录完成。 | 单音测量 API 按 README；三谐波合成是允许的少量逻辑 |
| 288–331 | `App_DrawPage` | 清屏后使用 ST7789 的 `DrawString/DrawFloat`，字体固定 `TFT_ST7789_FONT_8X16`；按页显示 4 个代表扫频点、1 kHz 误差或 1/2/3 次谐波状态。扫频最后一行在 y=148，`A/D PAGE` 固定画在 y=216，避免遮挡测量数据。 | ST7789/8×16 API 按 README；页面布局自写 |
| 321–341 | `SysTick_Handler` | 按 22_X 的方式用静态毫秒计数，每 5 ms 扫描一次 4×4 键盘；返回状态写入 `g_keypad_status`，新字符写入 `volatile` 挂起变量；中断不执行扫频和浮点算法。 | 键盘扫描调用按 22_X/README；投递状态机为应用逻辑 |
| 343–356 | `App_ProcessKey` | 主循环取出按键；A/D 循环切页，1/2/3 分别运行扫频、1 kHz 补偿和 1/2/3 次谐波补偿；完成后刷新显示。 | 按键映射和步骤串联为应用逻辑 |
| 367–382 | `main` | 先运行 `SYSCFG_DL_init`，再正常初始化已修复的双 ADC 模块；DMA_CH0、DMA_CH2 的完成中断已由模块内部打开。随后初始化 DAC DMA、ST7789，配置 SysTick，显示首页，循环处理按键并 `__WFI`。 | 各模块初始化按 README；启动顺序为比赛步骤逻辑 |

模块目录中的 `signal_dual_adc_mspm0g3507.c` 已严格同步集成库和 22_X 的版本，包含两路 DMA 完成中断使能；此前故障由误复制旧 example05 副本造成。其余模块 `.c/.h/.inc` 没有改写。`signal_config.h` 已恢复母版默认值。新增和调整均位于 `main.c`、组合 README 与步骤文档。
