# DualADCPhaseMeasurement：双路同步 ADC 相位测量

> **LEVEL C / REAL ALGORITHM MODULE：** 这个模块把一帧已经完成的双路同步 ADC 原始码直接换算成 Y 相对 X 的相位差。它不启动 ADC，也不占用 MCU 外设。

## 1. 什么时候使用

本题的输入条件是：X 频率 1.5~2 kHz，Y 是 X 的 1~5 倍，且两路由同一个 Timer 触发同步采样。模块先找 X 的上升过零点，再在其前后找最近的 Y 上升过零点，最后按已知频率比计算相位。

不要把它用于两路不同步、频率比未知、DMA 尚未完成或信号幅度太小的情况。若只需要把两个已经提取好的过零位置换算成相位，可直接使用已有的 `03_measurement/phase` 模块；本模块的区别是把过零检测也包含在一次调用中。

## 2. 原理和数据流

```text
双 ADC DMA 完成
        |
        v
raw_x[N]、raw_y[N] -- 求各自 min/max 和动态中点阈值
        |
        v
上升过零 + 固定 ADC 码滞回 + Q16 线性插值
        |
        +--> X 过零序列 --> X 平均周期 TX
        |
        +--> Y 过零序列 --> 每个 X 点前后两个候选
                                  |
                                  v
                         选择时间距离更小的 Y 点
                                  |
                                  v
           phase = -360 * (fY/fX) * (tY - tX) / TX
                                  |
                                  v
                        多个 X 周期环绕平均
```

`tX`、`tY` 和 `TX` 都是 sample 或 Q16 sample。因为分子和分母使用同一采样率，实际计算中 `Fs` 会约掉；接口仍要求传入真实 `sample_rate_hz`，用于确认调用者确实提供了有效时基。

符号约定：Y 的上升过零比 X 晚，表示 Y 滞后，结果为负；Y 比 X 早，结果为正。结果归一化到 `[-180, +180]`。

## 3. 复制文件

从集成库复制到比赛工程 `modules/`：

```text
03_measurement/common/signal_algorithm_status.h
05_precision/dual_adc_phase_measurement/signal_dual_adc_phase.c
05_precision/dual_adc_phase_measurement/signal_dual_adc_phase.h
```

`signal_algorithm_status.h` 是本模块唯一的公共状态依赖。不要把 `signal_status.h` 重命名来替代它，也不要修改本模块的 `.c/.h`。

## 4. SysConfig / Pin

**本模块不需要 SysConfig。** 它没有 GPIO、ADC、Timer、DMA、Event、IRQ 或时钟配置。

上游 `adc_dual_sync` 模块仍需要按它的 README 配置双 ADC、同一 Timer 触发、Event 路由和 DMA。只有在 `SignalDualADC_IsFinished()` 成功后，才可以把两个 raw 缓冲区传给本模块；DMA 正在写入时禁止调用。

## 5. 参数教程

### 比赛必须会

| 参数 | 类型/单位 | 本题建议 | 影响 |
|---|---|---:|---|
| `samples_x/y` | `uint16_t[]`，ADC code | ADC DMA 的两路数组 | 必须等长、同一次同步采样、调用期间只读 |
| `sample_count` | 点数 | `1024U` | 至少 2；窗口过短会没有两个 X 周期 |
| `sample_rate_hz` | Hz | `500000U` | 必须为真实同步采样率；只做接口校验 |
| `frequency_ratio` | 无量纲 | `g_pll_multiplier`，1~5 | 对应 `fY/fX`，错误会让相位比例错误 |
| `hysteresis_code` | ADC code | `16U` | 抑制阈值附近噪声重复触发；噪声较大时可增大 |
| `min_amplitude_code` | ADC code | `64U` | 小于该峰峰码值时返回 `NO_FEATURE` |
| `max_x/y_crossings` | 个数 | `16U/64U` | 内部固定缓存容量上限，不能超过头文件宏 |

窗口关系：`Tframe = sample_count / sample_rate_hz = 1024 / 500000 = 2.048 ms`。X 为 1.5~2 kHz 时，一帧约包含 3~4 个周期，足以形成至少两个 X 上升过零点；若后续增大 N，要同步评估 RAM。

### 出问题再理解

- 动态中点为 `(minimum + maximum) / 2`，所以不要求信号直流偏置正好位于 ADC 满量程中点。
- 滞回只在重新“准备检测”时使用：样本必须先回到 `threshold - hysteresis` 以下，随后才接受一次上升穿越。
- Q16 位置把第 `index` 个采样点表示为 `index << 16`，并用相邻样本的线性比例补出小数部分。

### 以后进阶

当前模块使用固定内部数组，不支持动态分配和多帧双缓冲。若要在更高频率或更长窗口下工作，应先重新计算交叉点上限和栈空间，再修改模块接口设计。

## 6. 调用顺序

```text
SYSCFG_DL_init / SignalDualADC_Init       上电一次
        |
SignalDualADC_Start                       每帧开始
        |
SignalDualADC_IsFinished                  等待 DMA 完成
        |
SignalDualADCPhase_Process                每帧调用一次
        |
检查 status == SIGNAL_ALGORITHM_OK
        |
读取 result.phase_degrees，交给 TFT/UART
```

本模块没有 `Init`。它是同步计算函数，返回前完成所有检测；成功后结果归调用者所有，下一次调用可以复用同一组 ADC 缓冲区。

## 7. API 教程

### `SignalDualADCPhase_Process`

用途：对一帧同步双通道 ADC 原始码执行完整相位测量。

最小调用形状：

```c
signal_dual_adc_phase_result_t result;
signal_algorithm_status_t status = SignalDualADCPhase_Process(
    raw_x, raw_y, 1024U, 500000U, &config, &result);
if ((status == SIGNAL_ALGORITHM_OK) && (result.valid != 0U)) {
    int16_t phase = result.phase_degrees;
    /* phase 是 Y-X，单位 degree。 */
}
```

参数含义：

- `samples_x`：X 路只读 ADC 原始码。
- `samples_y`：Y 路只读 ADC 原始码，必须与 X 路同步且等长。
- `sample_count`：两路数组长度。实现接受 2~65535 点。
- `sample_rate_hz`：实际同步采样率，不能填 ADC 时钟或估计值。
- `config`：检测阈值、频率比和内部结果容量。
- `result`：输出相位和诊断计数。返回失败时 `valid` 为 0。

返回值：

- `SIGNAL_ALGORITHM_OK`：已得到有效相位。
- `SIGNAL_ALGORITHM_INVALID_ARGUMENT`：空指针、采样率为零、频率比不在 1~5 或容量超限。
- `SIGNAL_ALGORITHM_INSUFFICIENT_DATA`：样本不足、X 没有两个周期、Y 没有足够过零点或没有可配对的最近点。
- `SIGNAL_ALGORITHM_NO_FEATURE`：任一路峰峰码值小于 `min_amplitude_code`。

成功时的结果成员：

| 成员 | 含义 |
|---|---|
| `phase_degrees` | Y 相对 X 的相位，单位 degree，范围 `[-180,+180]` |
| `valid_phase_count` | 实际参与环绕平均的 X/Y 配对数 |
| `x_crossing_count` | 本帧检测到的 X 上升过零数 |
| `y_crossing_count` | 本帧检测到的 Y 上升过零数 |
| `valid` | 1 表示结果有效，0 表示无效 |

## 8. 首次建议和常见错误

第一次接入时先保持 `N=1024`、`Fs=500 kSPS`、`hysteresis=16`、`min_amplitude=64`，用已知 0 度或 90 度相位的两路信号检查正负号。最常见错误是把 `g_pll_multiplier` 传成 0、把 ADC Timer 输入时钟当成采样率、在 DMA 完成前调用，或把 Y 路数组顺序写反。

不要在本模块内部加入 TFT 绘图、GPIO 控制、延时或 `malloc`。这些属于应用层或上游硬件模块。

## 9. 完整使用案例

比赛工程中的典型链路是：双 ADC 模块完成一帧采集后，复制 `README_MINIMAL_EXAMPLE.c` 中的配置和调用形状；把 `frequency_ratio` 换成键盘设置的 PLL 倍频数；成功后把 `phase_degrees` 交给 TFT 的 `PH:` 数值区域。`README_FULL_EXAMPLE.c` 另外展示了错误返回和诊断计数读取。

## 10. 验证状态

当前模块已创建完整 `.c/.h`、README、最小示例、完整示例和模块卡；模块源码已通过 TI Arm Clang 主机端编译检查，板级验证仍未执行。当前状态：`PC_VERIFIED`，板级验证：`NOT_RUN`。

## 11. 统一 API 教程

本节按仓库的 [Beginner README 标准](../../00_docs/BEGINNER_README_STANDARD.md) 组织，先复制 `README_MINIMAL_EXAMPLE.c` 完成一帧正常数据流，再按需要参考 `README_FULL_EXAMPLE.c` 的错误处理和诊断字段。

### `SignalDualADCPhase_Process`

**作用：** 对一帧已经完成 DMA 的同步 X/Y ADC 原始码，完成动态中点、滞回上升过零、Q16 线性插值、X 周期估计、Y 过零配对和相位环绕平均。

| 参数 | 初学者解释 |
|---|---|
| `samples_x` | X 路 ADC 原始码数组，只读；必须与 Y 路来自同一次同步采样。 |
| `samples_y` | Y 路 ADC 原始码数组，只读；长度和采样时刻必须与 X 相同。 |
| `sample_count` | 每路数组的元素个数，不是字节数；本题建议 `1024U`。 |
| `sample_rate_hz` | 实际同步采样率，单位 Hz；本题建议 `500000U`，不能填 Timer 输入时钟。 |
| `config` | 调用者填写的检测配置；`frequency_ratio` 必须等于 `fY/fX`，范围 1~5。 |
| `result` | 调用者提供的输出对象；只有返回成功且 `valid != 0U` 时才读取结果。 |

返回 `SIGNAL_ALGORITHM_OK` 且 `result.valid != 0U` 时，`result.phase_degrees` 是 Y 相对 X 的相位，单位度，范围 `[-180,+180]`；Y 晚于 X 为负值。失败时按返回码检查参数、数据长度、幅度和过零数量，不能把旧结果当新结果。

### 最小调用形状

```c
signal_dual_adc_phase_result_t result;
signal_algorithm_status_t status = SignalDualADCPhase_Process(
    raw_x, raw_y, 1024U, 500000U, &config, &result);
if ((status == SIGNAL_ALGORITHM_OK) && (result.valid != 0U)) {
    int16_t phase = result.phase_degrees;
    /* phase 是 Y-X，单位 degree。 */
}
```

逐行看：第一行给结果留出存储空间；第二行调用模块并传入两路缓冲区、点数、真实采样率、配置和结果地址；第三行同时检查返回码和有效标志；第四行只在有效时取出相位，避免显示无效数据。

### 模块链和验收

`双 ADC 同步采集 -> DMA 完成 -> SignalDualADCPhase_Process -> 检查 status/valid -> TFT/UART`。本模块不提供 `Init`，不配置 SysConfig，不访问 GPIO、Timer、DMA、IRQ，也不修改输入数组。README/API、两个示例和头文件应保持同一函数签名；文档检查通过只代表接口一致，不代替板级验证。
