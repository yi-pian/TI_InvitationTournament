#ifndef SIGNAL_LOG_PARABOLIC_INTERPOLATION_H
#define SIGNAL_LOG_PARABOLIC_INTERPOLATION_H

#include <stdint.h>

#include "signal_algorithm_status.h"

typedef struct
{
    float bin_offset;
    float fractional_bin;
    float frequency_hz;
    float interpolated_magnitude;
} signal_log_parabolic_result_t;

/**
 * @brief 对峰值及左右三个正的线性 magnitude 取自然对数后做抛物线插值。
 * @param magnitude 线性幅值谱，三个参与值必须有限且大于 0。
 * @param bin_count magnitude 数组长度。
 * @param peak_index 整数峰值索引，必须有左右相邻点且自身为局部最大。
 * @param sample_rate_hz 采样率，Hz。
 * @param fft_size FFT 点数。
 * @param result 输出 bin 偏移、分数 bin、频率 Hz 和插值线性幅值。
 * @return 成功返回 SIGNAL_ALGORITHM_OK；非正幅值、平顶或非法参数返回错误码。
 * @note 这是局部近似；不同窗函数和低 SNR 会产生不同系统偏差。
 */
signal_algorithm_status_t SignalLogParabolicInterpolation_Process(
    const float *magnitude,
    uint32_t bin_count,
    uint32_t peak_index,
    float sample_rate_hz,
    uint32_t fft_size,
    signal_log_parabolic_result_t *result);

#endif /* SIGNAL_LOG_PARABOLIC_INTERPOLATION_H */
