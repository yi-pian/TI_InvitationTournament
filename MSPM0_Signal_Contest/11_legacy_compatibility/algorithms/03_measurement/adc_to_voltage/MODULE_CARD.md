# MODULE CARD

RECOMMENDED_LEVEL: LEVEL_A_DIRECT_RECIPE；旧 API 兼容保留，新工程见 `00_docs/recipes/adc_to_voltage.md`。

MODULE: ADC To Voltage

CATEGORY: Measurement / Adapter

功能：把 `uint16_t` ADC 原始码换算为 `float` 物理电压。

输入：`raw_codes[]`（ADC code）、`count`、`adc_max_code`、`reference_voltage_v`、`input_scale`、`offset_voltage_v`。

输出：`voltage_v[]`，单位 V。

是否原地处理：NO。输入是 `uint16_t`，输出是 `float`，必须使用独立输出数组。

依赖：`signal_algorithm_status.h`、标准 C `math.h`。

典型用途：`ADC_DMA -> ADC_ToVoltage -> Mean/RMS/RemoveDC/FFT`。

不要用于：输入已经是 V 的 `float`；参考电压、前端增益未知时不要把结果称为校准电压。

计算量：LOW，O(N)，每点一次乘加。

RAM：模块内部 O(1)；调用者输出缓冲区 `4*N` 字节。1024 点输出为 4096 字节。

Benefits：集中管理 code→V 公式，避免每个 `main.c` 重写换算。

Trade-offs：使用 `float` 增加 RAM 和 Cortex‑M0+ 软件浮点计算量；精度受 VREF 与模拟前端误差限制。

可连接：

```text
ADC_DMA -> ADC_ToVoltage -> Mean / Vpp / RMS / RemoveDC / ClippingDetect
```

状态：PC_VERIFIED。2026-08-07 使用 PC GCC C11 严格警告编译并通过首批真值测试；尚未 BOARD_VERIFIED。
