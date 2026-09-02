# 算法接口统一规范

> 本规范从 2026-08-11 起只约束 **Level C 正式算法模块**和必要的 Level B Helper。Level A Direct Recipe 不使用公共 status/config/context/result 形式，直接采用 Cookbook 中的短函数。分级规则见 [ALGORITHM_LEVEL_POLICY.md](ALGORITHM_LEVEL_POLICY.md)。

## 1. 命名

公开函数采用：

```text
Signal + 清楚的模块名 + 动作
```

例：`SignalRMS_Process()`、`SignalHann_Apply()`、`SignalFFT_Process()`。禁止使用 `calc1()`、`fun()`、`process_data2()` 等不能表达职责的名字。

类型和字段使用小写蛇形命名：

```c
signal_rms_result_t
sample_rate_hz
frequency_hz
phase_deg
amplitude_vpp
```

## 2. Level B/C 典型函数形状

Level C 无状态但有复杂边界/多输出的测量：

```c
signal_algorithm_status_t SignalXxx_Process(
    const float *samples,
    uint32_t count,
    const signal_xxx_config_t *config,
    signal_xxx_result_t *result);
```

数组变换：

```c
signal_algorithm_status_t SignalXxx_Apply(
    const float *input,
    float *output,
    uint32_t count,
    const signal_xxx_config_t *config);
```

没有配置项时不为了形式创建空 `config`。平均值、Vpp、普通 RMS 等 Level A 功能不再按本模板创建模块，直接使用 Cookbook Recipe。

## 3. Level C 返回值

正式算法模块统一返回 `signal_algorithm_status_t`；Direct Recipe 依靠调用前的固定数组/参数约束，不为了十几行计算引入公共状态码：

| 返回码 | 含义 |
|---|---|
| `SIGNAL_ALGORITHM_OK` | 调用成功，输出有效 |
| `SIGNAL_ALGORITHM_INVALID_ARGUMENT` | 空指针、非法阈值、非法配置 |
| `SIGNAL_ALGORITHM_INSUFFICIENT_DATA` | 点数为零或不足以完成算法 |
| `SIGNAL_ALGORITHM_OUT_OF_RANGE` | 输入 code、索引或参数超范围 |
| `SIGNAL_ALGORITHM_BUFFER_TOO_SMALL` | 调用者工作区或输出容量不足 |
| `SIGNAL_ALGORITHM_NO_FEATURE` | 没找到过零、峰值或周期等目标特征 |
| `SIGNAL_ALGORITHM_NUMERIC_ERROR` | 出现 NaN、Inf、除零或数值溢出 |
| `SIGNAL_ALGORITHM_NOT_SUPPORTED` | 当前实现不支持该配置 |

返回非 OK 时，调用者不得使用 result。算法不通过 `printf` 报错，也不死循环。

## 4. 输入、输出与 const

- 不修改的输入必须写 `const`。
- 输出缓冲区和 result 由调用者分配。
- 默认输入与输出不得重叠；支持原地处理的模块必须明确写 `允许 input == output`。
- `count` 表示元素个数，不表示字节数。
- 纯算法不持有 ADC/DMA 指针，不使用大型隐藏全局数组。
- 禁止无界动态内存；当前库不使用 `malloc/calloc/free`。

## 5. 单位

字段名尽量自带单位：

| 后缀 | 单位/含义 |
|---|---|
| `_hz` | Hz |
| `_s` | 秒 |
| `_v` | V |
| `_vpp` | V 峰峰值 |
| `_deg` | 度 |
| `_rad` | 弧度 |
| `_percent` | 百分数，1.0 表示 1% |
| `_ratio` | 无量纲比值，1.0 表示 100% |
| `_db` | dB |
| `_code` | ADC 原始码值 |
| `_index` | 从 0 开始的数组索引 |

不能通过变量名表达的单位，必须写在公开 API 注释和 README 中。

## 6. 数据类型

- ADC RAW：`uint16_t`。
- 点数、索引：`uint32_t`。
- 算法信号和物理量：默认 `float`。
- 逻辑标志：结构体内使用 `uint8_t`，避免不同 C 环境的 `bool` ABI 疑问。
- FFT 复数后续使用明确的 `{float real; float imag;}`，不依赖编译器复数扩展。

MSPM0G3507 的 Cortex‑M0+ 没有硬件 FPU；`float` 也由软件执行。选择它是为了接口清晰和竞赛开发速度，不代表它“免费”。高采样率实时链必须测周期预算。

## 7. 参数与 result

- 经常一起修改的参数放入 `config`。
- 结果不返回裸 `float`，而使用带单位字段名的 result，例如 `rms_v`、`frequency_hz`。
- 每个参数在 README 中回答：默认怎么选、改大/改小有什么收益和代价。
- 临时工作区由调用者显式提供，并带 `workspace_count`/`capacity`。

## 8. 数值和边界规则

- 空指针必须检查。
- `count == 0` 必须报错；需要至少 2/3 个点的算法检查自己的下限。
- 配置中的除数必须非零；频率和采样率必须为正。
- 基础浮点模块遇到 NaN/Inf 返回 `SIGNAL_ALGORITHM_NUMERIC_ERROR`。
- 不静默钳位会改变物理含义的输入；例如 ADC code 超过 `adc_max_code` 返回越界。

## 9. 头文件和依赖

- 每个头文件有 include guard。
- 公开头文件只包含自己真正需要的标准头和公共算法状态头。
- 模块间依赖必须写入 MODULE_CARD；能保持短小、可靠时允许模块自包含，避免隐藏调用链。
- 不包含芯片厂商寄存器头文件。

## 10. 验证状态

统一使用：

```text
DRAFT
BUILD_VERIFIED
PC_VERIFIED
BOARD_VERIFIED
CONTEST_VERIFIED
```

只有实际运行证据才能升级。PC 测试必须写 Expected、Measured、Absolute Error、Relative Error 和 PASS/FAIL。PC_VERIFIED 不等于实板验证。
