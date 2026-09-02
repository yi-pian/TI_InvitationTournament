# 22_X 双路同步 ADC 相位测量实施步骤

## 1. 题目目标

在上一版双路同步 ADC、矩阵键盘 PLL 倍频和 ILI9341 李萨如图的基础上，测量 Y 相对 X 的相位差，并在 TFT 的 `PH:` 后显示整数角度。已知 X 频率为 1.5~2 kHz，Y 为 X 的 1~5 倍；当前键盘设置的 `g_pll_multiplier` 就是 `fY/fX`。

约定：Y 的上升过零点晚于 X，表示 Y 滞后，显示负值；Y 早于 X，显示正值；结果限制在 `[-180,+180]` 度。

## 2. 按比赛步骤选择模块

本次选择集成库正式模块：

| 选择的模块 | 用途 | 是否需要 SysConfig |
|---|---|---|
| `05_precision/dual_adc_phase_measurement` | 从一帧同步双路 ADC 原始码直接检测过零并计算相位 | 否 |
| `02_acquisition/adc_dual_sync` | 提供 X/Y 同步 ADC DMA 数据和真实采样率 | 已在前一步配置 |
| `01_bsp/tft_ili9341` | 显示 `PH:` 和相位数字 | 已在前一步配置 |

没有选择已有的 `03_measurement/phase` 作为唯一入口，是因为它要求调用者已经准备好两路对应过零位置；本题需要在一次调用中完成动态阈值、滞回过零、插值和配对，所以使用新的双路封装模块。没有新增 Timer、DMA、GPIO、Event、IRQ 或时钟资源。

## 3. 按 README 复制文件

从集成库复制到比赛工程 `signal_contest_template_final/modules/`：

```text
MSPM0_Signal_Contest/05_precision/dual_adc_phase_measurement/signal_dual_adc_phase.c
MSPM0_Signal_Contest/05_precision/dual_adc_phase_measurement/signal_dual_adc_phase.h
MSPM0_Signal_Contest/03_measurement/common/signal_algorithm_status.h
```

`.c/.h` 和公共算法状态头均为冻结模块文件，本次没有修改。`signal_status.h` 仍属于原有硬件模块，不能重命名替代 `signal_algorithm_status.h`。

## 4. SysConfig 配置

本模块 README 的结论是：**不需要 SysConfig**。因此本次没有增加外设实例、没有改引脚、没有改 ADC/Timer/DMA/Event，也没有手工编辑 `Debug/ti_msp_dl_config.c/.h`。

继续沿用上一版双 ADC 的配置：同一 Timer 触发两路 ADC，DMA 完成后 `SignalDualADC_IsFinished()` 才允许调用相位模块。相位模块的 `sample_rate_hz` 必须传真实同步采样率 `500000U`，不能传 Timer 输入时钟。

## 5. 从 README 复制到 main 的代码

### 5.1 头文件和配置结构

复制 README 最小示例的 `signal_dual_adc_phase.h` include、`signal_dual_adc_phase_config_t` 配置结构和 `SignalDualADCPhase_Process` 调用形状。比赛工程把 README 示例中的固定 `frequency_ratio = 1U` 改为每帧写入键盘当前值 `g_pll_multiplier`，这是本题唯一必要的应用层参数连接。

```c
static signal_dual_adc_phase_config_t g_phase_config = {
    .hysteresis_code = PHASE_HYSTERESIS_CODE,
    .min_amplitude_code = PHASE_MIN_AMPLITUDE,
    .frequency_ratio = 1U,
    .max_x_crossings = 16U,
    .max_y_crossings = 64U
};
```

参数含义：`16U` 是 ADC 码滞回，减少阈值附近噪声重复触发；`64U` 是最大小 Y 过零缓存数；`frequency_ratio` 由主循环动态更新。

### 5.2 README 调用代码在 main 中的位置

在 DMA 完成后调用：

```c
g_phase_config.frequency_ratio = g_pll_multiplier;
phase_status = SignalDualADCPhase_Process(
    g_raw_a, g_raw_b, SIGNAL_SAMPLE_COUNT,
    SIGNAL_SAMPLE_RATE_HZ, &g_phase_config, &phase_result);
```

输入 `g_raw_a/g_raw_b` 就是上一轮双 ADC 模块填充的同步数组；`SIGNAL_SAMPLE_COUNT` 为 1024；`SIGNAL_SAMPLE_RATE_HZ` 为 500 kSPS。

## 6. 自己编写的应用逻辑

模块 `.c/.h` 没有改动，自己写的部分只有以下组合逻辑：

1. 把键盘得到的 `g_pll_multiplier` 连接到相位模块的 `frequency_ratio`。
2. 检查模块返回码和 `result.valid`，有效时保存相位，无效时清除有效标志。
3. 在 TFT 左侧已有 `WV:` 的下一行增加 `PH:` 标签。
4. 只清除相位数值区域，再显示相位或 `----`；没有刷新蓝色李萨如边框。

## 7. 自写代码逐行解释

### 7.1 相位配置（main.c 第 57~63 行）

| 行 | 代码 | 解释 |
|---:|---|---|
| 57 | `static signal_dual_adc_phase_config_t g_phase_config` | 创建一个静态配置对象，整个程序循环都复用它。 |
| 58 | `.hysteresis_code = PHASE_HYSTERESIS_CODE` | 过零检测的滞回码值为 16，抑制阈值噪声。 |
| 59 | `.min_amplitude_code = PHASE_MIN_AMPLITUDE` | 峰峰值小于 64 码时认为没有可靠信号。 |
| 60 | `.frequency_ratio = 1U` | 上电默认按 Y/X=1；主循环每帧会覆盖成当前 PLL 倍频。 |
| 61 | `.max_x_crossings = 16U` | 允许保存的 X 上升过零数，不能超过模块头文件上限。 |
| 62 | `.max_y_crossings = 64U` | 允许保存的 Y 上升过零数，Y 最高为 X 的 5 倍，所以给更大容量。 |
| 63 | `};` | 结束结构体初始化。 |

### 7.2 TFT 相位显示函数（main.c 第 217~235 行）

| 行 | 代码 | 解释 |
|---:|---|---|
| 217~218 | `App_DrawPhaseDegrees(int16_t phase_degrees, uint8_t valid)` | 定义应用层显示函数；输入相位整数和有效标志。 |
| 220 | `tft_ili9341_status_t status;` | 保存每次 TFT 调用的返回状态。 |
| 222~224 | `FillRect(&g_tft, 234, 120, 48, 16, BLACK)` | 只擦除 `PH:` 后的 48x16 数值区域，避免旧数字残留。 |
| 226 | `if (valid == 0U)` | 判断本帧是否得到可靠相位。 |
| 227~230 | `DrawString(..., "----", ...)` | 无效时显示占位符，不把旧相位伪装成新结果。 |
| 232~234 | `DrawInt32(..., phase_degrees, ...)` | 有效时显示带正负号的整数角度。 |

### 7.3 初始化标签（main.c 第 338~349 行）

第 338~341 行调用模块已有的 `DrawString`，在 `WV:` 下方画固定标签 `PH:`。第 342~345 行绘制已有 PLL/YV/WV 值，并调用新增的 `App_DrawPhaseDegrees` 显示初始 `----`。第 346~349 行记录各显示版本号，避免主循环重复重画没有变化的固定数值。

### 7.4 DMA 完成后的相位处理（main.c 第 369~382 行）

| 行 | 代码 | 解释 |
|---:|---|---|
| 369 | `signal_dual_adc_phase_result_t phase_result;` | 为模块输出分配本轮结果对象。 |
| 370 | `signal_algorithm_status_t phase_status;` | 保存算法层返回码，与硬件层 `signal_result_t` 区分。 |
| 371 | `g_phase_config.frequency_ratio = g_pll_multiplier;` | 把键盘设置的 1~5 倍频数作为 `fY/fX` 传给模块。 |
| 372~374 | `SignalDualADCPhase_Process(...)` | 复制 README 的正式调用，输入同步 X/Y 数组、N、Fs、配置和结果地址。 |
| 375~376 | `status == OK && result.valid != 0U` | 两个条件都满足才接受结果。 |
| 377 | `g_phase_degrees = phase_result.phase_degrees;` | 保存模块计算出的 Y-X 相位角。 |
| 378 | `g_phase_valid = 1U;` | 标记本轮相位可显示。 |
| 379~381 | `else ... g_phase_valid = 0U;` | 过零不足、幅度太小或参数错误时显示 `----`。 |
| 382 | `++g_phase_display_revision;` | 每完成一帧就通知主循环更新相位数值区域。 |

## 8. 算法原理（比赛时口述）

1. 分别求 X/Y 本帧最小值和最大值，取动态中点作为阈值。
2. 样本先低于“阈值-滞回”才重新武装；随后检测从阈值以下到阈值以上的上升穿越。
3. 用相邻两点的线性比例得到 Q16 小数过零位置，减少整数采样点量化误差。
4. 用多个 X 上升过零点平均得到 X 周期 `TX`。
5. 对每个 X 过零点，在前后找最近的 Y 上升过零点；Y 频率为 X 的整数倍时，最近点仍对应同一相位分支。
6. 按 `phase = -360 * (fY/fX) * (tY - tX) / TX` 计算，并环绕到 `[-180,+180]`，多个配对结果再做环绕平均。

采样率在分子分母中会约掉，但接口仍传入真实 `500000U`，用于保证时间基准参数有效。窗口为 `1024/500000 = 2.048 ms`，X=1.5~2 kHz 时约覆盖 3~4 个周期。

## 9. 验证记录

| 项目 | 结果 |
|---|---|
| 新模块 `.c/.h` 和两个 README 示例 TI Arm Clang `-Wall -Werror` 语法检查 | PASS |
| `22_X/main.c` 使用当前生成头文件和模块头文件的 TI Arm Clang 语法检查 | PASS |
| 使用现有 ADC/TFT/键盘对象，加新相位对象的完整 TI Arm Clang 链接 | PASS，生成 `Debug/phase_check/signal_contest_template_final_phase.out` |
| SysConfig | PASS，新增纯算法模块不需要配置；未修改生成文件 |
| 集成库索引/模块完整性 | PASS，新模块已登记为正式可选模块，状态 `PC_VERIFIED` |
| 实际开发板、输入信号和 TFT 相位数值 | `NOT_RUN`，本次没有连接和烧录实板 |

## 10. 赛场复现顺序

1. 选择 `dual_adc_phase_measurement`，阅读 README 的复制清单和“不需要 SysConfig”说明。
2. 将两个 `.c/.h` 和 `signal_algorithm_status.h` 复制到工程 `modules/`，Refresh 并确认新 `.c` 未 Exclude from Build。
3. 保持双 ADC 的同步 Timer、DMA 和 Event 配置不变，确认 DMA 完成后再处理数组。
4. 从 README 最小示例复制配置结构和 `SignalDualADCPhase_Process` 调用；只把 `frequency_ratio` 接到键盘的 `g_pll_multiplier`。
5. 在 TFT 固定文字区增加 `PH:`，在相位数值区做局部刷新；不要全屏刷新或重画蓝色边框。
6. Clean/Build，先检查返回码和 `valid`，再下载到板上，用已知相位的两路信号检查正负号和倍频比。

