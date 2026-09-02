# Backend Adapter

> **INTERNAL SUPPORT：** 这不是比赛功能选择入口。只有正式算法 Backend 明确要求 Q15/Q31/float 数据格式转换时才由其 README 引用；普通应用不要主动复制它。

## 第一次使用 Backend Adapter？从这里开始

目标：“在 float、Q15 和 ADC raw 之间转换数据表示，或累加 Q15 平方”。它是拼接支持模块，不负责 ADC、FFT 或物理电压校准。

### STEP 1：加入工程

链接 `MSPM0_Signal_Contest/04_dsp/backend_adapter/signal_backend_adapter.c`；Include Path 加本目录和 `MSPM0_Signal_Contest/03_measurement/common`。

### STEP 2：include

```c
#include "signal_backend_adapter.h"
```

### STEP 3：变量

按路径准备等长数组：

```c
float x_f32[N];
int16_t x_q15[N];
uint16_t adc_raw[N];
uint64_t sum_q30;
```

### STEP 4：参数

- `full_scale`：float 中对应归一化 1.0 的正物理量；写错会使转换比例错，超范围会饱和。
- `zero_code`：输入物理 0 对应的 ADC code，例如双极性偏置前端中点 2048。
- `positive_span_codes`：从 zero_code 到正满量程的码数，必须 >0。
- `count=N`：输入输出容量都至少 N。

它与 ADC To Voltage 不同：ADC To Voltage 输出物理 V；ADCRawToQ15 直接输出归一化定点数，适合定点 backend。

### STEP 5：SysConfig

**【不需要 SysConfig】**。但 zero code/ADC span 必须来自真实前端和 ADC 配置。

### STEP 6：初始化

没有 Init；需要转换时同步调用。

### STEP 7：真正调用

```c
(void)SignalBackendAdapter_FloatToQ15(x_f32, x_q15, N, 3.3f);
(void)SignalBackendAdapter_Q15ToFloat(x_q15, x_f32, N, 3.3f);
(void)SignalBackendAdapter_ADCRawToQ15(adc_raw, x_q15, N, 2048U, 2047U);
(void)SignalBackendAdapter_Q15SquareAccumulate(x_q15, N, &sum_q30);
```

只选择当前链真正需要的函数，不要求四个都调用。

### STEP 8：结果

Q15 的 -32768 表示 -1，32767 约为 +1；回转 float 的单位由 full_scale 决定；平方和每项为 Q30，累计在 uint64 中。

### STEP 9：连接

```text
ADC raw -> ADCRawToQ15 -> Q15 DSP backend
float voltage/normalized -> FloatToQ15 -> fixed DSP -> Q15ToFloat -> float consumer
```

若后续模块明确要 V，优先 `ADC To Voltage`，不要先转 Q15 再猜电压。

### STEP 10：Build

status header 缺失=common Include；undefined symbol=未链接 `.c`；大量 ±32768=full_scale/span 太小或输入真的越界；全接近 0=full_scale/span 太大或 zero code 错。

### STEP 11：验证

FloatToQ15 输入 `{-full_scale,0,+full_scale}` 应约为 `{-32768,0,32767}`；12 bit ADC 中点 2048 应转为约 0。

### STEP 12：常见修改

1. VREF/满量程改变：同步 full_scale 或 span，不改模块源码。
2. ADC 偏置中点不是 2048：用实测/calibration 的 zero_code。
3. float 链改 Q15 backend：先完成真值对比和饱和统计，再替换 backend。
4. N 增大：每个额外 Q15 workspace 增加 `2N` bytes。

### STEP 13：完整最小示例

```c
#include "signal_backend_adapter.h"
void Convert(void)
{
    const float x[3] = {-1.0f, 0.0f, 1.0f};
    int16_t q[3];
    (void)SignalBackendAdapter_FloatToQ15(x, q, 3U, 1.0f);
}
```

下面是四个 API 的逐参数说明、饱和规则和验证证据。

## 1 这个模块是干什么的？

它只负责“换数据表示”，不负责采 ADC，也不负责 FFT。比如 ADC 给出 0～4095，交流零点在 2048；本模块可以把它一次转换成约 -32768～32767 的 Q15，供定点 DSP 使用。

## 2 最简单的例子

`ADC RAW = {0, 2048, 4095}`，`zero_code=2048`，`positive_span_codes=2048`，输出约为 `{-32768, 0, 32751}`。

## 3 原理

先计算 `(raw-zero_code)/positive_span_codes`，再映射到 Q15。超过 ±1 的输入会饱和，避免整数回绕成完全相反的符号。

## 4 比赛里什么时候用？

当整条链准备采用 CMSIS Q15 FFT/FIR，并希望避免 `RAW→整块float→Q15` 的重复转换时使用。

## 5 输入

- ADC RAW：`uint16_t`，单位为 ADC code；
- float：单位可以是 V，但 `full_scale` 必须也是 V；
- Q15：`int16_t`，约定 -1 到 +1。

## 6 输出

Q15、float 物理量，或供 RMS/能量继续计算的 Q30 平方和。

## 7 API 怎么调用

```c
SignalBackendAdapter_ADCRawToQ15(raw, q15, count, 2048U, 2048U);
```

## 8 参数怎么改

`zero_code` 应来自前端零点标定；`positive_span_codes` 表示希望哪个输入幅度映射为 Q15 满量程。

## 9 参数改大会怎样

增大 `positive_span_codes`：更不容易饱和，但每个 Q15 LSB 代表更大的输入变化，量化精度降低。

## 10 代价

收益是 RAM 和转换次数可控；代价是 Q15 量化与饱和，并且必须保存满量程标尺。

## 11 什么时候不要用

若后续全是 float 测量、数据量很小，额外转 Q15 没有收益。若真实尖峰必须保留，也不能用过小满量程让它饱和。

## 12 怎么接前一个模块

`ADC_DMA → const uint16_t raw[] → ADCRawToQ15`

## 13 怎么接后一个模块

`ADCRawToQ15 → Q15 FFT/FIR → Q15ToFloat → 物理结果`

## 14 最小 Demo

见第 7 节；buffer 均由调用者提供，无动态内存。

## 15 PC 测试

测试零点、正负满量程、过量程饱和、float 往返和平方累加；预期由明确整数真值给出。

## 16 MCU 资源

算法自身 O(1) RAM；输入和输出若同时保留，各占 `2*N` 字节。

## 17 验证状态

PC_VERIFIED；目标板运行周期尚未测量，不能标记 BOARD_RUNTIME_VERIFIED。

## 18. README Usability Upgrade：完整 API

以下内容逐项对应正式头文件；没有公开的范围或语义保留 `UNKNOWN / NOT EXPOSED`。

### `signal_algorithm_status_t SignalBackendAdapter_FloatToQ15( const float *input_samples, int16_t *output_q15, uint32_t count, float full_scale);`

- **作用：** 把以 full_scale 为满量程的 float 样本转换成 Q15。

| 参数 | 真实类型 | 含义/单位/要求 |
|---|---|---|
| `input_samples` | `const float *` | 输入样本，只读；单位由调用者决定。 |
| `output_q15` | `int16_t *` | 输出有符号 Q15，-32768 表示 -1，32767 约等于 +1。 |
| `count` | `uint32_t` | 样本数量，必须大于 0。 |
| `full_scale` | `float` | 输入中对应归一化幅值 1.0 的正数，单位与输入相同。 |

- **返回：** 成功返回 SIGNAL_ALGORITHM_OK；非有限数或非法参数返回错误码。
- **前置/后置：** 超过满量程的输入会饱和，不会发生整数回绕；允许原始输入被后续覆盖，但不支持重叠数组。

```c
signal_algorithm_status_t status_or_value = SignalBackendAdapter_FloatToQ15(input_samples, output_q15, count, full_scale);
```

### `signal_algorithm_status_t SignalBackendAdapter_Q15ToFloat( const int16_t *input_q15, float *output_samples, uint32_t count, float full_scale);`

- **作用：** 把 Q15 样本恢复为 float 物理量。

| 参数 | 真实类型 | 含义/单位/要求 |
|---|---|---|
| `input_q15` | `const int16_t *` | 输入 Q15 样本，只读。 |
| `output_samples` | `float *` | 输出 float 样本，单位由 full_scale 决定。 |
| `count` | `uint32_t` | 样本数量，必须大于 0。 |
| `full_scale` | `float` | Q15 幅值 1.0 对应的物理满量程，必须为正数。 |

- **返回：** 成功返回 SIGNAL_ALGORITHM_OK。
- **前置/后置：** UNKNOWN / NOT EXPOSED

```c
signal_algorithm_status_t status_or_value = SignalBackendAdapter_Q15ToFloat(input_q15, output_samples, count, full_scale);
```

### `signal_algorithm_status_t SignalBackendAdapter_ADCRawToQ15( const uint16_t *adc_raw, int16_t *output_q15, uint32_t count, uint16_t zero_code, uint16_t positive_span_codes);`

- **作用：** 将无符号 ADC RAW 直接居中并映射到 Q15，避免先生成整块 float 数组。

| 参数 | 真实类型 | 含义/单位/要求 |
|---|---|---|
| `adc_raw` | `const uint16_t *` | ADC 原始码数组，例如 12 位 ADC 的 0..4095。 |
| `output_q15` | `int16_t *` | 输出 Q15 数组。 |
| `count` | `uint32_t` | 样本数量，必须大于 0。 |
| `zero_code` | `uint16_t` | 输入物理量为 0 时的 ADC 码，例如双极性前端中点 2048。 |
| `positive_span_codes` | `uint16_t` | 从 zero_code 到正满量程的码数，必须大于 0。 |

- **返回：** 成功返回 SIGNAL_ALGORITHM_OK；超过 int16 可表示范围的配置返回错误码。
- **前置/后置：** 结果会饱和到 [-32768,32767]；本函数不知道 ADC 寄存器和 DMA。

```c
signal_algorithm_status_t status_or_value = SignalBackendAdapter_ADCRawToQ15(adc_raw, output_q15, count, zero_code, positive_span_codes);
```

### `signal_algorithm_status_t SignalBackendAdapter_Q15SquareAccumulate( const int16_t *input_q15, uint32_t count, uint64_t *sum_squares_q30);`

- **作用：** 累加 Q15 样本平方，供 RMS/能量算法继续使用。

| 参数 | 真实类型 | 含义/单位/要求 |
|---|---|---|
| `input_q15` | `const int16_t *` | 输入 Q15 样本。 |
| `count` | `uint32_t` | 样本数量，必须大于 0。 |
| `sum_squares_q30` | `uint64_t *` | 输出平方和；每一项为 Q30，累加器为 uint64_t。 |

- **返回：** 成功返回 SIGNAL_ALGORITHM_OK。
- **前置/后置：** 32768 个满幅样本仍不会溢出 uint64_t；更长数据需由调用者检查累计上限。

```c
signal_algorithm_status_t status_or_value = SignalBackendAdapter_Q15SquareAccumulate(input_q15, count, sum_squares_q30);
```

## 19. Call Sequence / Connecting / Buffer Rules

```text
准备输入/config/workspace -> SignalBackendAdapter_FloatToQ15 / SignalBackendAdapter_Q15ToFloat / SignalBackendAdapter_ADCRawToQ15 / SignalBackendAdapter_Q15SquareAccumulate -> 检查返回码 -> 读取 result/output -> 交给下一模块
```

算法无 SysConfig、无动态内存。输入/输出、是否允许原地、workspace 和 capacity 以第18节真实 `@param/@note` 为准；capacity 是元素数。失败时不得把 result/output 当有效数据。模块上下游和更完整示例见原 README 第12～14节。

## 20. Parameter Guide / Config vs SysConfig

| 可修改项 | 真实入口 | 改变后的主要影响 | RAM/速度 | SysConfig? |
|---|---|---|---|---|
| `count` | 公开 API 参数 | 增大通常增加数据量、邻域或阶数，并增加 RAM/CPU；减小则相反，必须满足下限 | 由 N/容量/复杂度决定 | 否 |
| `full_scale` | 公开 API 参数 | 改变比例或幅值含义，需检查满量程/标度 | 由 N/容量/复杂度决定 | 否 |

改变 ADC/DAC pin、Timer/DMA/Event 或真实 Fs 属于上游硬件配置，不是本算法 SysConfig。常见错误：单位混用、capacity 不足、忽略返回码、原地规则错误、Fs/N 与数据不一致、把 raw/能量/幅值/百分数混为一谈。

## 21. Verification / Quick Modify

先用原 README 已列 PC 真值/边界方法；再在完整链中用已知输入核对单位和误差。未实板不得写 BOARD_VERIFIED。

| 我想改什么 | 去哪里 | 改什么 | 会影响什么 | SysConfig? |
|---|---|---|---|---|
| N/容量 | Application buffer + API | count/capacity | RAM、CPU、观察长度 | 否 |
| Fs/频率 | Application config/API | actual Fs/target Hz | 频率刻度 | 改真实硬件Fs时才是 |
| 阈值/阶数/半径 | config/API | 对应真实字段 | 灵敏度、带宽或计算量 | 否 |
| 输入单位/标度 | 上游 Adapter/校准 | V、magnitude、energy 等 | 结果物理意义 | 否 |
