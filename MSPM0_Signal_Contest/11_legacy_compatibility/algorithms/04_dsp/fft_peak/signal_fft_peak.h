#ifndef SIGNAL_FFT_PEAK_H
#define SIGNAL_FFT_PEAK_H

#include <stdint.h>

#include "signal_algorithm_status.h"

typedef struct
{
    uint32_t bin;
    float peak_value;
    float frequency_hz;
} signal_fft_peak_result_t;

signal_algorithm_status_t SignalFFTPeak_Process(
    const float *magnitude,
    uint32_t magnitude_count,
    uint32_t first_bin,
    uint32_t last_bin,
    float sample_rate_hz,
    uint32_t fft_size,
    signal_fft_peak_result_t *result);

#endif
