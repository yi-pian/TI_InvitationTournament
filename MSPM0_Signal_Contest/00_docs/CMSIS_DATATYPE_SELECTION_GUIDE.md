# CMSIS-DSP 数据类型选择指南

适用环境：MSPM0G3507、Cortex-M0+、TI Arm Clang、MSPM0 SDK 2.11.00.07、CMSIS-DSP 1.16.2。

## 先给结论

| 类型 | 默认定位 | 典型用途 | 主要代价 |
|---|---|---|---|
| Q15 | FFT/长数组高速处理的 `DEFAULT_CANDIDATE` | FFT、FIR、频谱前端 | 量化、饱和、逐级缩放必须理解 |
| Q31 | 精度优先的定点候选 | 高动态范围 FFT、精细幅相 | RAM 是 Q15 的约 2 倍；周期仍待板测 |
| F32 | 小规模计算和易读 Recipe | 标量后处理、RMS、Mean、校准、快速验证 | M0+ 无 FPU；不能假定比定点快 |

`DEFAULT_CANDIDATE` 不等于“已证明最快”。当前真实证据只有 PC 数值测试和 TI Clang 的 Build/Link、Flash/SRAM；执行周期仍为 `PENDING_BOARD`。

## Q15

Q15 通常把 `-1.0 ... 接近 +1.0` 映射到 `-32768 ... 32767`。ADC 电压进入 Q15 前必须先归一化，并为噪声、窗函数和多音叠加留下峰值余量。

```c
float32_t normalized = voltage_v / full_scale_v;
if (normalized > 0.999969F) normalized = 0.999969F;
if (normalized < -1.0F) normalized = -1.0F;
q15_t sample_q15 = (q15_t) (normalized * 32768.0F);
```

Q15 CFFT 的输入/输出是交错复数：`real0, imag0, real1, imag1, ...`，需要 `2*N*sizeof(q15_t)`。固定点 FFT 内部有缩放；幅值恢复不能照搬 F32 的 `2/N`。比赛前用已知幅值正弦完成端到端标定，并查 `CMSIS_Q15_FFT_SCALING.md`。

## Q31

Q31 同样表示归一化有符号小数，但使用 32 位。它更能保留低电平细节，代价是 FFT 复数 buffer 为 `8*N` 字节。当前 PC golden 结果显示 Q31 数值误差明显小于 Q15，但这不证明板上更快。

## F32

F32 最适合初学者和小数组：单位直接保留为 V、Hz、ratio，CMSIS Recipe 清楚。MSPM0G3507 Cortex-M0+ 无硬件 FPU，因此大 FFT/长 FIR 使用 F32 前必须看 map 和板上周期。

## 选择流程

1. 先决定结果精度和峰值动态范围。
2. 估算峰值同时存活 RAM：输入、FFT 交错 buffer、幅值、窗表、workspace。
3. Q15/Q31 必须写清楚 Q 格式、归一化满量程和饱和策略。
4. 用同一测试向量比较误差；用同一优化等级和同一板载计时方式比较周期。
5. 未板测时保留 Q15 为默认候选；精度不足时切 Q31；实现/理解优先且资源够时用 F32。

## 当前 N 的资源事实

详见 `10_tests/backend_benchmark/build_target/fft_target_build_matrix.csv`。CFFT 工作 buffer 仅按数据宽度估算：Q15 `4*N` 字节，Q31/F32 `8*N` 字节；真实应用还必须加 ADC、窗、幅值和栈。

## MATHACL

MATHACL 不是 CMSIS FFT 的替代品。SDK 的 IQMath 路径可对定点 `sqrt/div/sin-cos/atan2/multiply` 使用 MATHACL。只有这些标量/小向量操作成为瓶颈，且 RTS 与 MATHACL 的板上周期和误差比较完成后，才标为 `SPECIAL_BACKEND`。
