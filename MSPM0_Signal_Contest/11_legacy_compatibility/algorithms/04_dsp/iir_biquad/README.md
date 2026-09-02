# IIR_Biquad：二阶节级联滤波器

> 新比赛工程默认：CMSIS DIRECT。使用 `arm_biquad_cascade_df1_init_*` 与 `arm_biquad_cascade_df1_*`。必须核对设计工具与 CMSIS 的反馈系数符号。

> **CMSIS DIRECT / FROZEN_COMPATIBILITY：** SOS 系数、跨帧状态和稳定性仍需严格管理，但新工程由 CMSIS 实例与 Recipe 管理。本目录旧核心只维护既有 Application。

**新比赛工程不复制本目录源码。** 直接使用当前 SDK 的 `arm_biquad_cascade_df1_init_*` 与 `arm_biquad_cascade_df1_*`。Application 仍须用可靠工具按真实 Fs 生成 SOS 系数并核对反馈符号；无 SysConfig/Pin。

## 1 这个算法是干什么的？

它用少量状态和系数实现 IIR 滤波。复杂高阶 IIR 不直接写成一条很长公式，而拆成多个 Biquad/SOS 依次处理，数值更容易控制。

## 2 一个最简单的例子

系数 `b0=0.5,b1=0.5,b2=0,a1=0,a2=0` 时，`y[n]=0.5x[n]+0.5x[n-1]`。冲激输入输出 `0.5,0.5,0,0`。

## 3 原理

本实现用 Direct Form II Transposed，每节保存 d1/d2。反馈项让 IIR 能用较低阶实现陡响应，但也让过去输出影响未来。不同频率通常得到不同相位偏移，所以 IIR 一般不是线性相位。

## 4 比赛里什么时候用？

CPU/RAM 紧、需要陡低通/高通/带通或 50/60 Hz 陷波，并能接受相位畸变时。相位测量链要特别谨慎。

## 5 输入

每节 `{b0,b1,b2,a1,a2}`，约定 a0 已归一化为 1；每节一个 state；输入 float 块。系数必须由可靠设计工具按 Fs 生成。

## 6 输出

float 滤波块，可原地。单位/增益由频率响应决定，不保证原幅值。

## 7 API怎么调用

```c
signal_iir_biquad_coefficients_t sos[]={{b0,b1,b2,a1,a2}};
signal_iir_biquad_state_t state[1];
signal_iir_biquad_t iir;
SignalIIRBiquad_Init(&iir,sos,1U,state,1U);
SignalIIRBiquad_Process(&iir,in_v,out_v,count);
```

## 8 参数怎么改

不要现场凭感觉改单个系数。改 Fs、截止频率、Q 或滤波器类型后，用设计工具重新生成完整 SOS，验证极点、增益、冲激响应，再重新 Init。

## 9 参数改大会怎样

section_count 增加可实现更高阶/更陡响应，但 CPU、状态和数值风险增加。Q 增大通常峰更尖、振铃更长。反馈系数小改动也可能大幅改变稳定性。

## 10 这个算法的代价是什么

Benefits：每节只需两个状态，低阶就可较陡，适合 M0+ 资源约束。

Trade-offs：非线性相位、启动瞬态、反馈量化敏感、错误系数可能发散；不能自动证明稳定。

## 11 什么时候不要用

需要严格线性相位、系数未验证、相位是关键测量量、或 THD 前会衰减待测谐波时。此时考虑 FIR 或不滤波。

## 12 怎么和前一个模块接

```text
ADC_ToVoltage -> IIRBiquad_Init once -> Process each block
```

## 13 怎么和后一个模块接

```text
┌──── IIR SOS ────┐
│ section 1 d1/d2 │
│       ↓         │
│ section 2 ...   │
└───────┬─────────┘
        ├──> RMS / ZeroCross
        └──> 检查相位延迟后再做 Phase
```

## 14 最小Demo

```c
float x[]={1,0,0,0};
signal_iir_biquad_coefficients_t c={{0.5f,0.5f,0,0,0}};
signal_iir_biquad_state_t s[1]; signal_iir_biquad_t f;
(void)SignalIIRBiquad_Init(&f,&c,1U,s,1U);
(void)SignalIIRBiquad_Process(&f,x,x,4U);
```

## 15 PC测试

已用可手算单节冲激响应 Expected=`0.5,0.5,0,0` 验证系数符号、状态更新和原地处理，全部 PASS。尚未把某个具体截止频率系数标为已验证。

排查：输出发散立即停用并检查 a0 归一化、a1/a2 符号和极点；幅相不符确认设计工具的系数约定；每帧瞬态检查是否误 Reset。

## 16 MCU资源

每节状态 8 字节，计算 O(N·S)，每节约 5 乘法和 4 加减。系数 const 可放 Flash。真实周期和精确 Flash 需 TI Arm Clang 构建后记录。

## 17 验证状态

PC_VERIFIED（执行核）：2026-08-07，严格编译和单节冲激响应通过。具体滤波器系数设计仍需逐套验证；未 BOARD_VERIFIED。

## 18. 完整 Public API Reference

### `SignalIIRBiquad_Init(instance, sections, section_count, states, state_count)`

实例、只读 SOS 系数数组和可写状态数组均非空；section_count>0；state_count 至少 section_count。每节系数为 `b0,b1,b2,a1,a2`，约定 a0 已归一化为 1。成功清零状态并初始化；函数明确不自动判断稳定性。

### `SignalIIRBiquad_Reset(instance)`

Init 后清零所有节的 d1/d2；开始独立记录时使用，连续块之间不要 Reset。

### `SignalIIRBiquad_Process(instance, input_samples, output_samples, count)`

已 Init 实例；输入/输出各至少 count 个 float，允许完全原地；count>0。按 Direct Form II Transposed 顺序执行全部节，状态跨块保留。参数/状态/数值错误返回对应码。

## 19. Realistic Example / Buffer Rules

```c
static const signal_iir_biquad_coefficients_t sos[] = {
    /* 此处必须放由目标 Fs/频响设计并验证的真实系数 */
};
static signal_iir_biquad_state_t state[SECTION_COUNT];
signal_iir_biquad_t filter;
SignalIIRBiquad_Init(&filter, sos, SECTION_COUNT, state, SECTION_COUNT);
SignalIIRBiquad_Process(&filter, voltage, voltage, N);
```

README 不给伪造的“通用低通系数”。系数生命周期覆盖使用期；state RAM=`8*section_count` bytes，系数通常 `20*section_count` bytes；非原地 output 另需 `4N`。每通道使用独立 state/instance。

## 20. Parameter / Config vs SysConfig

section_count 增大可实现更高阶响应，但 CPU O(N*sections)、状态和数值误差增大。截止频率/带通范围不是本 API 参数，完全由按真实 Fs 设计的 SOS 决定；Fs 改变必须重算系数。全部 CONFIG ONLY；改真实 Fs 才需上游 SysConfig。

常见错误：使用不匹配 Fs 的系数、a1/a2 符号约定错、未验证极点稳定性、每块 Reset、两通道共享 state、系数顺序/节级联增益导致溢出、误以为模块内置低通/带通。

## 21. Quick Modify Table

| 我想改什么 | 去哪里 | 改什么 | 影响 | SysConfig? |
|---|---|---|---|---|
| 截止/带宽/Q | 外部 SOS 设计 | sections[] | 频响/稳定性 | 否 |
| 滤波器阶数 | sections/state/init | `section_count` | CPU/RAM/数值范围 | 否 |
| 新记录清状态 | call site | `SignalIIRBiquad_Reset` | 启动瞬态 | 否 |
| 原地处理 | Process | input==output | 省输出RAM | 否 |
| Fs | Acquisition + 重新设计SOS | actual Fs | Hz频响 | 改硬件Fs时是 |
