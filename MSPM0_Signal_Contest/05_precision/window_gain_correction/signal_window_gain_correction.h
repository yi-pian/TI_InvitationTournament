#ifndef SIGNAL_WINDOW_GAIN_CORRECTION_H
#define SIGNAL_WINDOW_GAIN_CORRECTION_H

#include <stdint.h>

#include "signal_algorithm_status.h"

/**
 * @brief 把实信号非负频率的原始 DFT magnitude 换算为单边峰值幅度。
 * @param raw_magnitude 输入 N/2+1 个原始 magnitude。
 * @param amplitude_peak 输出峰值幅度；允许与输入为同一数组。
 * @param bin_count 必须等于 fft_size/2+1。
 * @param fft_size FFT 点数，必须为偶数且至少 2。
 * @param coherent_gain 实际窗相干增益 mean(w)，必须为正。
 * @return 成功返回 SIGNAL_ALGORITHM_OK。
 * @note DC 与 Nyquist 不乘 2；其他单边 bin 乘 2。仅对接近 bin 中心的孤立正弦幅值最直接。
 */
signal_algorithm_status_t SignalWindowGainCorrection_Apply(
    const float *raw_magnitude,
    float *amplitude_peak,
    uint32_t bin_count,
    uint32_t fft_size,
    float coherent_gain);

#endif /* SIGNAL_WINDOW_GAIN_CORRECTION_H */
