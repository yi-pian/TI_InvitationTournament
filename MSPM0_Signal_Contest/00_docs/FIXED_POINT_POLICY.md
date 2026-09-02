# 定点数据政策

## 1 核心原则

先决定一条链是 float 还是定点，再进入 DSP。不要在每个模块边界反复做 `uint16→float→Q15→IQ24→Q31→float`。

推荐两条清楚路线：

```text
教学/通用路线：ADC RAW(uint16) → float V → float DSP → float 结果

比赛定点路线：ADC RAW(uint16) → 居中Q15(int16) → Q15 DSP/FFT
              → int32/int64能量 → 最后一次转float物理结果
```

## 2 每种类型表示什么

| 类型 | 用途 | 单位/标尺 | 注意 |
|---|---|---|---|
| `uint16_t` | ADC RAW | ADC code | 不带正负号，不等于 V |
| `int16_t` | 库边界 Q15 | -32768≈-1，32767≈+1 | 公共 Adapter 不依赖 CMSIS typedef |
| `q15_t` | CMSIS 内部 Q15 | 与 `int16_t` 相同 | 只在 CMSIS backend 内出现 |
| `int32_t/q31_t` | Q31 信号、Q15 乘积 | Q31 或 Q30，必须写清 | 同为 32 位不代表同一个 Q 格式 |
| `int64_t/uint64_t` | 平方和、能量累加 | 常见为 Q30 累加 | 防止 N 点累加溢出 |
| `_iq`/IQ24 | IQMath 标量数学 | 默认 24 个小数位 | 适合 sqrt/atan2 等，不是 FFT 数据格式 |
| `float` | 物理量和最终结果 | V、Hz、deg、%、dB | M0+ 无 FPU，方便但可能较慢 |

## 3 ADC RAW 到 Q15

使用：

```c
SignalBackendAdapter_ADCRawToQ15(
    raw, q15, count,
    zero_code,
    positive_span_codes);
```

`zero_code` 是物理零点对应的 ADC 码；`positive_span_codes` 是从零点到希望映射为 +1 的码数。参数太小会饱和，太大则浪费 Q15 有效位。

例如 12 位双极性前端：零点约 2048，正向跨度约 2048。真实零点应由校准结果替代硬编码。

## 4 Q15 FFT 后的类型

- CFFT 输入：交错 `real, imag`，共 `2*N` 个 Q15，即 `4*N` 字节；
- CMSIS Q15 前向 CFFT 内部按级缩放，原始输出约等于未归一化 DFT 的 `1/N`；
- 若继续做能量，不要先把每个 bin 转 float；Q15 平方为 Q30，用 64 位累加；
- 最终幅值、V、THD%、相位角才转 float。

本轮为兼容旧公开 API，FFT wrapper 会把 CMSIS 结果恢复为旧的“未归一化 float 复频谱”。它方便迁移，但不会获得原生 Q15 的全部 RAM 收益。

## 5 RMS 定点累加上限

一个满幅 Q15 平方约为 `2^30`。`uint64_t` 理论上可累加约 `2^34` 个满幅样本才接近溢出，比赛常用 512～4096 点有很大余量。仍要在接口文档中写明累计点数，禁止无限流式累加不清零。

## 6 IQMath 使用边界

IQMath 是“标量定点数学后端”，适合 sqrt、division、sin/cos、atan2。它不替代 CMSIS FFT。若数据已经是 Q15，可在清楚标尺后转换一次到 IQ24；若算法大部分仍是 float，只为一次除法做 float→IQ24→float 往返通常没有收益。

## 7 推荐给四个重点应用

| 应用 | 推荐数据链 |
|---|---|
| Frequency Meter C | 若走 FFT：RAW→Q15/CMSIS Q31 或保留 float；最终 Hz 为 float |
| Spectrum Analyzer | 稳定优先 CMSIS Q31 backend，公开输出仍为 float spectrum |
| THD Analyzer | CMSIS Q31 FFT→float magnitude/多 bin 能量→float `%` |
| Phase Meter | CMSIS Q31 FFT；atan2 默认 float，MATHACL 仅显式目标档 |

## 8 禁止事项

- 不知道零点就直接把 `uint16_t` 强转 `int16_t`；
- 把 Q15、Q30、Q31 都叫 `int32 result`；
- FFT 已按 `1/N` 缩放后再盲目除一次 N；
- 为“看起来用了硬件”而在每个标量操作前后转换数据；
- 未标满量程、窗增益和 FFT 长度就报告电压幅值。
