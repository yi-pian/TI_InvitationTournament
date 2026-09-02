# CMSIS FFT 频谱

**等级：CMSIS RECIPE。** 普通 FFT 与复数幅值直接使用 SDK 自带 CMSIS-DSP；Peak、插值、谐波和 THD 再接竞赛专用模块。

## 输入

- 去 DC、加窗后的 `N` 点数据；
- Q15 默认候选输入采用交错复数 `q15_t fft[2*N]`，偶数位置实部、奇数位置虚部；
- `N` 必须对应当前 SDK 的 CFFT 实例，并满足 RAM/延迟要求。

## 输出

- `q15_t magnitude[N]`；实信号通常只使用 bin `0 ... N/2`；
- bin 频率为 `frequency_hz = bin * Fs / N`；
- 幅值还必须修正固定点 FFT 缩放、单边谱和窗口 coherent gain。

## 比赛现场直接复制这一段

<!-- DIRECT_COPY_BEGIN -->
```c
#include "arm_const_structs.h"
#include "arm_math.h"

#define FFT_N 256U
static q15_t fft_q15[2U * FFT_N];
static q15_t magnitude_q15[FFT_N];

/* 先填 fft_q15[2*n]；fft_q15[2*n+1] = 0 */
arm_cfft_q15(&arm_cfft_sR_q15_len256, fft_q15, 0U, 1U);
arm_cmplx_mag_q15(fft_q15, magnitude_q15, FFT_N);
```
<!-- DIRECT_COPY_END -->

## 每一步为什么存在

1. ADC 数据先去 DC，避免 bin 0 吞掉动态范围。
2. 非相干采样时先加 Hann 等窗口，降低泄漏；相干采样可评估矩形窗。
3. `arm_cfft_q15` 完成 FFT，`bitReverseFlag=1` 保证正常 bin 顺序。
4. `arm_cmplx_mag_q15` 把交错复数变为非负幅值。
5. 找主峰用 Peak；需要亚 bin 精度再接 Parabolic/Jacobsen/Quinn/Macleod。

## 什么时候改 Q31/F32

- Q15 量化或动态范围不够：改 Q31；
- 数据本来是 float、数组不大且资源足：可用 F32；
- 不能凭感觉决定，先看 `CMSIS_DATATYPE_SELECTION_GUIDE.md` 和 target matrix。

## 失效条件

- 忘记 Q15 归一化或发生饱和；
- 把 RFFT packed 格式当 CFFT 交错格式；
- 忘记窗增益、单边谱和固定点缩放；
- `Fs/N` 不能分开近邻频率；
- 峰值在搜索边界或多个强峰重叠。

## 验证状态

CMSIS API、SysConfig、TI Arm Clang compile/link 已验证；板上执行周期仍为 `PENDING_BOARD`。
