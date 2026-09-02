# FIR：外部系数、可跨帧的有限冲激响应滤波器

> 新比赛工程默认：CMSIS DIRECT。使用 `arm_fir_init_*` 与 `arm_fir_*`；本目录普通 FIR 核心冻结兼容，不再发展第二套实现。状态长度、系数和 block size 示例见 CMSIS Cookbook。

> **CMSIS DIRECT / FROZEN_COMPATIBILITY：** FIR 有跨帧状态和系数约束，但这些由 CMSIS 实例与 Recipe 管理。本目录旧核心只维护既有 Application。

**新比赛工程不复制本目录源码。** 直接使用 `arm_fir_init_f32/q15/q31` 与 `arm_fir_f32/q15/q31`；系数、state 长度、block size 和完整最小代码见 `00_docs/CMSIS_DSP_CONTEST_COOKBOOK.md`。无 SysConfig/Pin。

## 1 这个算法是干什么的？

FIR 把当前和若干历史样本按系数加权相加。模块不写死截止频率：你把设计好的 coefficients 传进来，它只负责可靠执行。

## 2 一个最简单的例子

系数 `{0.25,0.5,0.25}`，输入冲激 `{1,0,0,0}`，输出正好 `{0.25,0.5,0.25,0}`，所以可用冲激响应直接检查实现。

## 3 原理

`y[n]=Σ h[k]x[n-k]`。若系数关于中心对称，所有频率获得相同形式的线性相位延迟，因此波形形状较容易保持；代价是通常约 `(T-1)/2` 样本群延迟。

## 4 比赛里什么时候用？

需要明确设计的低通/高通/带通，或相位关系重要且能接受较多计算时。只想简单平滑可先用 MovingAverage。

## 5 输入

初始化传外部 `coefficients[T]`、tap_count、`delay_line[T]`；Process 传 float 输入/输出和 count。系数 0 号乘当前样本。

## 6 输出

float 滤波数组，可与输入同一地址。单位取决于系数总增益；低通 DC 增益常设计为系数和 1。

## 7 API怎么调用

```c
const float h[3]={0.25f,0.5f,0.25f};
float state[3];
signal_fir_t fir;
SignalFIR_Init(&fir,h,3U,state,3U);
SignalFIR_Process(&fir,in_v,out_v,count);
```

## 8 参数怎么改

截止频率不能直接在 Process 中改；要用滤波器设计工具按当前 `sample_rate_hz` 生成新系数，并重新 Init。tap_count 必须与系数和状态容量一致。

## 9 参数改大会怎样

taps 增大通常可获得更陡过渡带/更好阻带，但 CPU、状态 RAM 和群延迟线性增加。截止频率相同但 Fs 改变时，旧系数对应的实际 Hz 会改变。

## 10 这个算法的代价是什么

Benefits：稳定、外部系数、可线性相位、可原地、跨帧状态保留。

Trade-offs：每输出点 T 次乘加；高阶耗时；滤波必然改变幅值；对称 FIR 仍有固定延迟。

## 11 什么时候不要用

没验证频率响应、系数来源不明、实时周期不够、要测被滤掉的谐波、或不能接受延迟时。

## 12 怎么和前一个模块接

```text
ADC_ToVoltage -> FIR_Init once -> FIR_Process each block
```

## 13 怎么和后一个模块接

```text
┌──────── FIR ────────┐
│ external h[k]       │
│ state across blocks │
└─────────┬───────────┘
          ├──> ZeroCross / RMS
          └──> Decimation（先验证抗混叠）
```

## 14 最小Demo

```c
float x[]={1,0,0,0}, s[3];
const float h[]={0.25f,0.5f,0.25f};
signal_fir_t f;
(void)SignalFIR_Init(&f,h,3U,s,3U);
(void)SignalFIR_Process(&f,x,x,4U);
```

## 15 PC测试

3-tap 冲激响应分成两块调用，Expected `0.25,0.5,0.25,0`，证明系数次序、原地和跨块状态都正确。全部 PASS。

排查：幅值错先求系数和/画频响；跨帧起点异常检查是否错误 Reset；频率响应随 Fs 变化检查系数设计采样率。

## 16 MCU资源

状态 `4T` 字节，计算 O(NT)，每 tap 一次乘加与环形索引。Cortex‑M0+ 无 FPU，高阶滤波必须实测周期；当前 PC_VERIFIED 不代表实时预算通过。

## 17 验证状态

PC_VERIFIED：2026-08-07，严格编译、冲激响应、原地及跨块测试通过；未 BOARD_VERIFIED。

## 18. 完整 Public API Reference

### `SignalFIR_Init(instance, coefficients, tap_count, delay_line, delay_line_count)`

实例、只读系数和可写状态数组均非空；tap_count>0；状态容量至少 tap_count。`coefficients[0]` 乘当前样本，系数生命周期覆盖整个实例使用期。成功清零状态并标记 initialized；容量/参数错误返回对应状态。

### `SignalFIR_Reset(instance)`

Init 后调用，清零历史但不改系数。用于开始一段独立记录；连续块处理中不要每块 Reset，否则块边界会产生启动瞬态。

### `SignalFIR_Process(instance, input_samples, output_samples, count)`

实例已 Init；输入/输出至少 count 个 float；支持完全原地；count>0。成功后状态保留给下一块。非有限样本/未 Init/参数错误返回对应状态。

```text
coefficients + delay_line -> Init -> Process block1 -> Process block2
                                      `-> Reset（仅新记录）
```

## 19. Realistic Example / Buffer Rules

```c
static const float taps[] = {0.25f, 0.25f, 0.25f, 0.25f};
static float delay[4];
signal_fir_t fir;
SignalFIR_Init(&fir, taps, 4U, delay, 4U);
SignalFIR_Process(&fir, voltage, voltage, N); /* 完全原地 */
```

系数只读且不能是短生命周期临时数组；delay 由调用者拥有，RAM=`4*tap_count`；非原地 output 另需 `4N`。同一实例包含状态，不可无保护地给两条通道或 ISR/主循环并发共享；每通道建独立实例/state。

## 20. Parameter / Config vs SysConfig

tap_count 增大可实现更复杂响应，但 CPU O(N*taps)、state/coeff RAM 增大。截止频率、通带/阻带和增益完全由外部系数与设计 Fs 决定；本模块不提供 `cutoff_hz` 参数。Fs 改变后同一系数对应的 Hz 响应会变化，必须重新设计/验证系数。

全部 CONFIG ONLY；改变真实采样 Fs 才触及上游 SysConfig。

常见错误：系数顺序错误、delay 容量小、每块 Reset、两通道共享 state、Fs 改了不重算系数、把 FIR 当零相位离线双向滤波、在栈上创建超大数组。

## 21. Quick Modify Table

| 我想改什么 | 去哪里 | 改什么 | 影响 | SysConfig? |
|---|---|---|---|---|
| 截止/带宽 | 外部滤波器设计 | `coefficients[]`（按Fs设计） | 频响/相位 | 否 |
| 阶数 | coefficients/delay/init | `tap_count` | CPU/RAM/过渡带 | 否 |
| 新记录清状态 | call site | `SignalFIR_Reset` | 启动瞬态 | 否 |
| 原地处理 | Process call | input==output | 省`4N` RAM | 否 |
| Fs | Acquisition + 系数设计 | actual Fs/重算系数 | Hz频响 | 改硬件Fs时是 |
