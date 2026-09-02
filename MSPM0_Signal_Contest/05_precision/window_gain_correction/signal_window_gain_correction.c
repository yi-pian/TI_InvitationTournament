#include "signal_window_gain_correction.h"

#include <math.h>
#include <stddef.h>

signal_algorithm_status_t SignalWindowGainCorrection_Apply(
    const float *raw_magnitude,
    float *amplitude_peak,
    uint32_t bin_count,
    uint32_t fft_size,
    float coherent_gain)
{
    uint32_t bin;
    uint32_t expected_bin_count;
    float base_scale;

    if ((raw_magnitude == NULL) || (amplitude_peak == NULL))
    {
        return SIGNAL_ALGORITHM_INVALID_ARGUMENT;
    }
    if ((fft_size < 2U) || ((fft_size & 1U) != 0U))
    {
        return SIGNAL_ALGORITHM_INSUFFICIENT_DATA;
    }
    expected_bin_count = (fft_size / 2U) + 1U;
    if (bin_count != expected_bin_count)
    {
        return SIGNAL_ALGORITHM_OUT_OF_RANGE;
    }
    if (!isfinite(coherent_gain) || (coherent_gain <= 0.0f))
    {
        return SIGNAL_ALGORITHM_INVALID_ARGUMENT;
    }
    for (bin = 0U; bin < bin_count; ++bin)
    {
        if (!isfinite(raw_magnitude[bin]) || (raw_magnitude[bin] < 0.0f))
        {
            return SIGNAL_ALGORITHM_NUMERIC_ERROR;
        }
    }

    base_scale = 1.0f / ((float)fft_size * coherent_gain);
    for (bin = 0U; bin < bin_count; ++bin)
    {
        float one_sided_scale = ((bin == 0U) || (bin == (fft_size / 2U)))
                                    ? base_scale
                                    : (2.0f * base_scale);
        amplitude_peak[bin] = raw_magnitude[bin] * one_sided_scale;
    }
    return SIGNAL_ALGORITHM_OK;
}
