#ifndef SIGNAL_FFT_PARABOLIC_INTERPOLATION_H
#define SIGNAL_FFT_PARABOLIC_INTERPOLATION_H

#include <stdint.h>

#include "signal_algorithm_status.h"

typedef struct
{
    float bin_offset;
    float fractional_bin;
    float frequency_hz;
    float interpolated_magnitude;
} signal_fft_parabolic_result_t;

/**
 * @brief 用峰 bin 及左右相邻 magnitude 做三点抛物线插值。
 * @param magnitude 原始或统一标度的线性 magnitude 数组。
 * @param bin_count 数组长度。
 * @param peak_index 整数峰索引，必须有左右邻点且自身为局部最大。
 * @param sample_rate_hz 采样率，Hz。
 * @param fft_size FFT 点数。
 * @param result 输出偏移、fractional bin、频率 Hz 和插值峰值。
 * @return 成功返回 SIGNAL_ALGORITHM_OK；平顶/非局部峰或参数错误返回对应码。
 * @note 线性 magnitude 抛物线是近似；Hann 下 log-parabolic 常有不同偏差。
 */
signal_algorithm_status_t SignalFFTParabolicInterpolation_Process(
    const float *magnitude,
    uint32_t bin_count,
    uint32_t peak_index,
    float sample_rate_hz,
    uint32_t fft_size,
    signal_fft_parabolic_result_t *result);

#endif /* SIGNAL_FFT_PARABOLIC_INTERPOLATION_H */
