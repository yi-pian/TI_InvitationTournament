# MSPM0G3507 FFT RAM 预算

目标芯片 RAM 为 32 KB。下面按 `uint16_t=2 bytes`、`float=4 bytes`、`signal_complex_f32_t=8 bytes` 计算，数值只统计明确数组，不包含链接器、C 运行库、外设状态、全局变量和 RTOS（若有）。

## Simple 版本：最容易理解

同时保留：

- ADC RAW：`2N`
- float 时域/加窗样本：`4N`
- N 点复数 FFT 输出/工作区：`8N`
- 单边 magnitude：`4×(N/2+1)=2N+4`
- 窗系数表：本库当前实时计算，`0`；若自行存表则再加 `4N`

合计（无窗表）：`16N+4 bytes`。

| N | ADC RAW | float samples | complex FFT | magnitude | 数组合计 | 再预留 2 KB stack | 32 KB判断 |
|---:|---:|---:|---:|---:|---:|---:|---|
| 512 | 1,024 | 2,048 | 4,096 | 1,028 | 8,196 | 10,244 | 宽松 |
| 1024 | 2,048 | 4,096 | 8,192 | 2,052 | 16,388 | 18,436 | 推荐上限，仍要给硬件留空间 |
| 2048 | 4,096 | 8,192 | 16,384 | 4,100 | 32,772 | 34,820 | 已超过 32 KB，禁止照此分配 |
| 4096 | 8,192 | 16,384 | 32,768 | 8,196 | 65,540 | 67,588 | 不可用 |

若额外保存 `float window[N]`，合计变为 `20N+4`：512 点 10,244 bytes，1024 点 20,484 bytes，2048 点 40,964 bytes。不要无脑存窗表；本库 Hann/Hamming/Blackman 直接计算系数，用 CPU 换 RAM。

## RAM-saving 版本：比赛熟练后再用

### 方案 A：直接填复数工作区

不建立独立 `float samples[N]`，把 ADC code 转为 V 后写进 `spectrum[n].real`，imag=0，再调用 `SignalFFT_ForwardComplexInPlace()`。若 RAW 仍保留，数组合计约：

`raw 2N + complex 8N + magnitude (2N+4) = 12N+4`。

| N | 合计 | +2 KB stack | 判断 |
|---:|---:|---:|---|
| 512 | 6,148 | 8,196 | 宽松 |
| 1024 | 12,292 | 14,340 | 推荐 |
| 2048 | 24,580 | 26,628 | 理论可放，但必须核对硬件/全局/栈余量 |
| 4096 | 49,156 | 51,204 | 不可用 |

这需要应用层写一个清晰适配循环或未来新增 `ADCToComplex` Adapter；不要改 ADC/DMA 驱动。

### 方案 B：采集完成后复用 RAW 所在存储

如果硬件接口和对齐允许、且不再需要 RAW，可把采集缓冲的生命周期结束后复用另一块已验证的复数工作区。仅保留 complex + magnitude 时为 `10N+4`：1024 点 10,244 bytes，2048 点 20,484 bytes，4096 点仍为 40,964 bytes，不可用。

不要用未声明的类型强制转换直接把 `uint16_t[]` 当 `float/complex`；大小、对齐、严格别名和 DMA 生命周期都可能出错。应通过明确的 union/链接器缓冲设计或分阶段静态区，并在工程级 map 文件中验证。

## 哪些 buffer 能覆盖？

| 阶段 | 可否原地/覆盖 | 原因 |
|---|---|---|
| Voltage -> RemoveDC | 可以 | RemoveDC 明确支持同一 float 输入输出 |
| Voltage -> Window | 可以 | 当前 Window API 支持同一 float 输入输出 |
| real samples -> complex FFT | Simple API 不可覆盖 | 元素大小从 4 变 8 bytes |
| complex FFT 内部蝶形 | 可以 | `SignalFFT_ForwardComplexInPlace` 就地工作 |
| complex FFT -> magnitude | 当前公开 API 按独立输出规划 | 不依赖未承诺的重叠行为 |
| magnitude -> GainCorrection | 可以 | 当前逐 bin 处理可原地，调用前看模块 README |

## 为什么不推荐 4096 点？

仅复数工作区已经 32,768 bytes，等于整片 RAM，栈、RAW、magnitude 和驱动状态无处放置。4096 点需求应优先考虑：降低 Fs/增加观测时间、过零多周期平均、1024/2048 点加插值、分块处理，或换 RAM 更大的器件；不是继续堆数组。

## 上板前必须做的检查

1. 查看 CCS/TI Arm Clang map 文件的 `.bss/.data` 与剩余 RAM。
2. 为最坏调用深度保留栈，并做栈水位测试；本文 2 KB 只是预算占位，不是证明。
3. 把 DMA 正在写的 buffer 与算法正在读写的 buffer 生命周期画出来。
4. 先用 512/1024 点完整链路上板，再考虑 2048 RAM-saving；任何点数都不能仅凭表格标记 BOARD_VERIFIED。
