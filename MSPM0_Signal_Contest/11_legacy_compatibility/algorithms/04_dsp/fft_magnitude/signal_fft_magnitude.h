#ifndef SIGNAL_FFT_MAGNITUDE_H
#define SIGNAL_FFT_MAGNITUDE_H

#include <stdint.h>

#include "signal_algorithm_status.h"
#include "signal_complex.h"

typedef struct
{
    uint32_t bin_count;
} signal_fft_magnitude_result_t;

/**
 * @brief 计算实信号 FFT 的非负频率原始 DFT magnitude。
 * @param spectrum N 点复频谱。
 * @param fft_size FFT 点数，至少 2 且为偶数。
 * @param magnitude 输出数组，容量至少 N/2+1。
 * @param magnitude_capacity 输出容量。
 * @param result 输出有效 bin 数 N/2+1。
 * @return 成功返回 SIGNAL_ALGORITHM_OK。
 * @note 输出尚未除以 N、做单边 x2 或窗增益修正，不能直接称为 Vpeak。
 */
signal_algorithm_status_t SignalFFTMagnitude_Process(
    const signal_complex_f32_t *spectrum,
    uint32_t fft_size,
    float *magnitude,
    uint32_t magnitude_capacity,
    signal_fft_magnitude_result_t *result);

#endif /* SIGNAL_FFT_MAGNITUDE_H */
