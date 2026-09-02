#ifndef SIGNAL_HARMONIC_H
#define SIGNAL_HARMONIC_H

#include <stdint.h>

#include "signal_algorithm_status.h"

#define SIGNAL_HARMONIC_MAX_ORDER 10U

typedef struct
{
    float fundamental_frequency_hz;
    uint32_t first_order;
    uint32_t last_order;
    uint32_t radius_bins;
} signal_harmonic_config_t;

typedef struct
{
    uint32_t order;
    float target_frequency_hz;
    float target_fractional_bin;
    uint32_t center_bin;
    uint32_t start_bin;
    uint32_t end_bin;
    float energy;
    float root_sum_square;
} signal_harmonic_item_t;

typedef struct
{
    uint32_t first_order;
    uint32_t last_order;
    signal_harmonic_item_t items[SIGNAL_HARMONIC_MAX_ORDER + 1U];
} signal_harmonic_result_t;

/**
 * @brief 按已知基波频率定位各次谐波，并对每次谐波附近多个 bin 积分。
 * @param magnitude FFT 非负频率线性 magnitude。
 * @param bin_count 通常为 fft_size/2+1。
 * @param sample_rate_hz 采样率，Hz。
 * @param fft_size FFT 点数。
 * @param config 基波频率、阶数范围 1~10、bin 半径。
 * @param result 输出每阶目标频率、中心/范围和能量。
 * @return 成功返回 SIGNAL_ALGORITHM_OK；超 Nyquist、窗口重叠或参数非法返回错误码。
 * @note BASIC 设 radius=0；COMPETITION 对 Hann 可从 radius=2 起测试。
 */
signal_algorithm_status_t SignalHarmonic_Process(
    const float *magnitude,
    uint32_t bin_count,
    float sample_rate_hz,
    uint32_t fft_size,
    const signal_harmonic_config_t *config,
    signal_harmonic_result_t *result);

#endif /* SIGNAL_HARMONIC_H */
