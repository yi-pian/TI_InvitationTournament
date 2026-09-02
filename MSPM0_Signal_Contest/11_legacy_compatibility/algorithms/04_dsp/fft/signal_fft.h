#ifndef SIGNAL_FFT_H
#define SIGNAL_FFT_H

#include <stdint.h>

#include "signal_algorithm_status.h"
#include "signal_complex.h"

/**
 * @brief 对复数数组执行未归一化的原地 radix-2 前向 FFT。
 * @param data 输入/输出复数数组，会被频谱覆盖。
 * @param count FFT 点数，必须是至少 2 的 2 次幂。
 * @return 成功返回 SIGNAL_ALGORITHM_OK；点数或数值非法返回错误码。
 * @note 前向符号为 exp(-j*2*pi*k*n/N)，不除以 N；无 twiddle 大表，节省 RAM。
 */
signal_algorithm_status_t SignalFFT_ForwardComplexInPlace(
    signal_complex_f32_t *data,
    uint32_t count);

/**
 * @brief 把实数输入复制进复数输出并执行前向 FFT（小白优先的简单接口）。
 * @param input_samples 实数时域输入，只读。
 * @param spectrum 输出复频谱，容量至少 count。
 * @param count 2 次幂 FFT 点数，至少 2。
 * @param spectrum_capacity 复数输出容量。
 * @return 成功返回 SIGNAL_ALGORITHM_OK。
 * @note 输出占 `8*count` 字节；RAM 紧时可自己填复数 buffer 后调用原地接口。
 */
signal_algorithm_status_t SignalFFT_ForwardReal(
    const float *input_samples,
    signal_complex_f32_t *spectrum,
    uint32_t count,
    uint32_t spectrum_capacity);

#endif /* SIGNAL_FFT_H */
