# Math/DSP library selection

## 一句话选择

- 数组、滤波、FFT、复数和统计运算：先看 CMSIS-DSP。
- 单个定点乘除、sqrt、sin/cos、atan2：先看 IQMath；在 G3507 上优先测
  MathACL 版本。
- 简单控制、状态判断、小规模标定：普通 C。
- PC 真值、教学、回归对照：Reference C float/double。

应用层不能直接调用 `arm_*` 或 `_IQ*`；只通过 `algorithm_backends/` 或保持原有
`Signal*` 接口调用。

## 当前模块选择表

| 运算/模块 | 当前实现 | 推荐后端 | 结论 |
|---|---|---|---|
| FFT/IFFT | 自写 float radix-2 | CMSIS-DSP Q15 | 比赛默认；保留 Reference C 对照 |
| Magnitude | 自写 float + `sqrtf` | CMSIS-DSP Q15/F32 | 与 FFT 数据格式一致时迁移；注意 Q15 输出是 2.14 |
| FIR | 自写 float/double 累加 | CMSIS-DSP | 长滤波器或重复块处理优先；很短滤波保留 C 更清楚 |
| IIR/Biquad | 正式库中不存在 | CMSIS-DSP | 仅在 recipe 真正需要时新增，不为审计虚构模块 |
| RMS/mean/min/max | 自写 C | CMSIS-DSP 或 C | 大块连续数据优先 CMSIS；低频单次测量可保留 C |
| Complex math | 仅类型、FFT magnitude | CMSIS-DSP | 复乘、复幅值等数组运算适合 CMSIS |
| Correlation | 自写归一化、限定 lag | 保留 C 编排 | CMSIS raw correlation 不能直接替代归一化和 lag 搜索；以后可只加速内核 |
| Phase/atan2 | 自写 float | IQMath MathACL 候选 | 先做板上周期/误差测试，再替换内部实现 |
| DDS | 整数相位累加 + LUT | 保留现有 C | 热路径已高效；IQMath 只考虑低频配置或生成表 |
| SineFit | double 累加和矩阵求解 | 保留 C/reference | 数值条件敏感，不在缺少端到端误差测试时定点化 |
| Calibration | 简单 float | 保留 C | 调用频率低，改定点收益小 |
| Peak interpolation | 简单 float | 保留 C | 运算量小，float 更易审查 |

## 为什么默认 FFT 是 CMSIS-DSP Q15

已经完成的证据：

- 512/1024/2048/4096 点均在 PC 上与 Reference FFT 数值对照通过。
- 4 种点数均通过 TI Arm Clang 5.1.1 目标编译链接。
- 4096 点只有 Q15 方案能在单 FFT 探针中保留足够的非 FFT SRAM；Reference、
  Q31、F32 的 32768 B 数据缓冲连同程序状态无法链接。
- MSPM0G3507 无 FPU，不能预设 float32 更快。

因此 `DEFAULT COMPETITION FFT BACKEND = CMSIS_DSP_Q15`。这是后端选择，不改变
`ADC_DMA → RemoveDC → Hann → FFT → Peak → THD` 的 recipe。板上周期仍为
`PENDING_BOARD`，若真实周期结果否定该选择，允许只换后端而不换上层接口。

当前 `04_dsp/fft/SignalFFT_Execute()` 的参数契约是 float complex；本次审计没有把
它偷偷改成 Q15，也没有在内部额外分配转换缓冲。正式系统集成时，应在组装层选择
Q15 数据路径并调用薄后端，同时让原函数继续作为 Reference API。这样不会让一个
表面为 float 的接口产生隐藏缩放或双份 RAM。

## Q15、Q31、F32 的取舍

| 后端 | 复数缓冲 | 动态范围/误差 | 缩放与风险 | 适用场景 |
|---|---:|---|---|---|
| CMSIS Q15 | `4N` B | 约 15 位小数；实测最大 FFT 误差 ≤2.45e-4 | 逐级缩放，输入过满量程仍有饱和风险 | 默认比赛 FFT，尤其 2048/4096 点 |
| CMSIS Q31 | `8N` B | 精度高；实测最大误差 ≤5.57e-8 | 仍需理解逐级缩放；RAM 是主要限制 | 小 N、高动态范围且内存允许 |
| CMSIS F32 | `8N` B | 动态范围宽；无需定点量化 | 无 FPU，周期尚未上板；4096 点 RAM 不可行 | PC/reference 或小 N 且实测速度可接受 |
| Reference F32 | `8N` B | 便于审计和教学 | 每级计算 sin/cos，目标端预计昂贵 | PC 对照，不作为比赛热路径 |

## IQMath RTS 与 MathACL

两者使用同一 `IQmathLib.h` API，但链接不同 `iqmath.a`：

- RTS：纯软件实现，用作兼容基线。
- MathACL：面向带 MATHACL 的 MSPM0G3507，适合标量乘除、sqrt、sin/cos、atan2。

IQMath 不替代 FFT。MAC/SAC 是 MATHACL 硬件操作，但当前薄 IQMath wrapper 没有伪造
对应 API；如果后续确有热点，再基于 DriverLib 建立独立、可测的直接 MATHACL wrapper。

## 状态含义

- `REFERENCE_VERIFIED`：PC reference 真实运行通过。
- `CMSIS_HOST_RUNTIME_VERIFIED`：CMSIS 源码在 PC 真实编译和运行通过。
- `CMSIS_TARGET_BUILD_VERIFIED`：目标 CMSIS archive 在 TI Clang 5.1.1 实际链接通过。
- `IQMATH_RTS_TARGET_BUILD_VERIFIED`：RTS archive 实际链接通过，目标运行待测。
- `IQMATH_MATHACL_TARGET_BUILD_VERIFIED`：MathACL archive 实际链接通过，目标运行待测。
- 只有导入 CCS、下载到 LP-MSPM0G3507 并看到 pass 后，才升级 IQMath 的运行状态。

完整证据、内存表和路径见
[`DSP_MATH_BACKEND_AUDIT.md`](DSP_MATH_BACKEND_AUDIT.md)。
