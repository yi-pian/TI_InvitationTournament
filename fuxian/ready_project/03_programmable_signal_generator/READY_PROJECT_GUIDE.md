# 03 可编程信号发生器使用与复用说明

## 1. 工程作用

本工程利用片内 DAC、DMA、定时器和 Wave Output 链输出可调波形。支持 SINE、TRIANGLE、SAWTOOTH 和 SQUARE，并在 TFT 上显示当前波形及参数。

适合用于：

- 比赛现场提供测试正弦、三角、锯齿或方波；
- 给滤波器、放大器、比较器和测量工程提供激励；
- 作为需要键盘数字输入的 DAC/DDS 应用模板。

## 2. 默认接口与参数

| 项目 | 默认设置 |
|---|---|
| 模拟输出 | DAC0 / PA15 |
| DDS 更新率 | 100 kHz |
| 初始波形 | SINE |
| 初始频率 | 1000 Hz |
| 初始 Vpp | 1.0 V |
| 初始 Offset | 1.65 V |
| Duty / Symmetry | 50% |
| 显示 | ST7789 |
| 输入 | 4×4 键盘 |

输出参数会被限制，使 `Offset ± Vpp/2` 保持在 DAC 0～3.3 V 范围内。实际输出带宽、负载能力和幅值精度仍受 DAC、更新率及外部负载影响。

## 3. 参数意义

| 波形 | Frequency | Vpp | Offset | Duty | Symmetry |
|---|---:|---:|---:|---:|---:|
| SINE | 有效 | 有效 | 有效 | `--` | `--` |
| TRIANGLE | 有效 | 有效 | 有效 | `--` | 有效 |
| SAWTOOTH | 有效 | 有效 | 有效 | `--` | 有效 |
| SQUARE | 有效 | 有效 | 有效 | 有效 | `--` |

## 4. 按键操作

| 按键 | 功能 |
|---|---|
| A/B | 上一个/下一个波形 |
| C | 选择下一个参数 |
| 数字键 | 输入当前参数 |
| `*` | 删除一位；无输入时减小参数 |
| `#` | 确认输入；无输入时增大参数 |
| D | 确认并更新输出 |

Frequency 以 Hz 输入，Vpp 和 Offset 以 mV 输入，Duty 与 Symmetry 以百分数输入。

## 5. 运行数据流

```text
SysTick 每 5 ms 扫描键盘并入队
  -> 主循环处理波形/参数/数字输入
  -> 参数限幅
  -> 选择 Wave Output API 生成波表和 DMA 输出
  -> 只局部刷新发生变化的参数行和状态行
```

输出更新和 TFT 刷新都不在键盘中断中执行。

## 6. 复用的 fuyong 内容

| 来源 | 当前函数/能力 | 类型 | 用途 |
|---|---|---|---|
| `90_dds_usage` | `ApplyWaveform()` 中的输出调用 | `FUYONG_ADAPTED` | 设置频率和波形 |
| `91_dac_usage` | DAC/DMA 初始化闭包 | 模块原样复制 | 实际模拟输出 |
| `90_dds_usage` | Sine/Square/Triangle/Sawtooth 波形模块 | 模块原样复制 | 生成波表 |
| `70_keypad_usage` | 数字输入基本规则 | `FUYONG_ADAPTED` | 参数输入 |
| `moni01` | 8 项按键环形队列 | `FUYONG_ADAPTED` | 连续按键不覆盖 |
| `80_tft_usage` | TFT 字体和字段显示 | `FUYONG_ADAPTED` | 参数页面 |
| `moni01` | 静态标题 + 动态字段局部刷新 | `FUYONG_ADAPTED` | 降低 SPI 刷新量 |
| 本工程 | `ParseNumber()`、`ClampOutputParameters()`、参数语义 | `READY_PROJECT_LOCAL` | 形成可直接使用的仪器交互 |

## 7. 如何使用

1. CCS Generate、Clean、Build 并烧录。
2. 将 PA15 连接到被测电路输入，并连接公共地。
3. 用 A/B 选择波形，用 C 选择参数。
4. 输入数值后按 `#` 或 D 确认。
5. 查看 `OUTPUT: RUN`；若显示 ERROR，检查参数组合和底层输出状态。
6. 用示波器测量 PA15，确认实际 Vpp、Offset 和频率符合需要后再接入赛题电路。

不要直接驱动低阻、大电容或超出 DAC 驱动能力的负载；必要时增加运放缓冲。

## 8. 如何复用到其他工程

### 8.1 只复用固定正弦输出

复制 Wave Output、DAC DMA 模块和对应 SysConfig，然后从 `App_Init()` 复制输出初始化，从 `ApplyWaveform()` 提取 `SignalWaveOutput_SineWithOffset()` 调用。固定参数工程不需要复制数字输入状态机。

### 8.2 复用多波形输出

复制：

- `waveform_t`；
- `target_frequency_hz`、`target_vpp_v`、`target_offset_v`；
- Duty/Symmetry 参数；
- `ClampOutputParameters()`；
- `ApplyWaveform()`。

保持一个参数只表达一个物理意义，不要把 Duty 和 Symmetry 复用为同一变量。

### 8.3 复用键盘数字输入

复制 `number_input[]`、`ParseNumber()`、`CommitNumberInput()`、`AdjustSelectedParameter()` 和队列式 `HandleKeypad()`。目标工程可替换参数枚举和提交分支，但应保留输入长度限制、删除键和限幅步骤。

### 8.4 与测量工程组合

主动测量工程可复用本工程的输出链，同时复用 04/05 的 ADC 采集。输出频率改变后必须留出稳定时间，再采集一帧；不要在 DAC 波表尚未更新时立即测量。

## 9. 验证状态

- Generate / Compile / Link：PASS；
- `-Wall -Werror`：PASS；
- Flash / Board：NOT_RUN；
- SRAM：3707 B（11.31%）；
- Flash：34960 B（26.67%）。

当前只验证构建闭包，实际输出幅值、失真、最高可靠频率和负载能力需实板测量。
