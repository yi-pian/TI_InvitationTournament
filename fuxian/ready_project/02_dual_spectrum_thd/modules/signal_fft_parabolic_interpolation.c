#include "signal_fft_parabolic_interpolation.h"

#include <math.h>
#include <stddef.h>

signal_algorithm_status_t SignalFFTParabolicInterpolation_Process(
    const float *magnitude,
    uint32_t bin_count,
    uint32_t peak_index,
    float sample_rate_hz,
    uint32_t fft_size,
    signal_fft_parabolic_result_t *result)
{
    float left;
    float center;
    float right;
    float denominator;
    float offset;

    if ((magnitude == NULL) || (result == NULL))
    {
        return SIGNAL_ALGORITHM_INVALID_ARGUMENT;
    }
    if ((bin_count < 3U) || (peak_index == 0U) ||
        (peak_index >= (bin_count - 1U)) || (fft_size < 2U))
    {
        return SIGNAL_ALGORITHM_OUT_OF_RANGE;
    }
    if (!isfinite(sample_rate_hz) || (sample_rate_hz <= 0.0f))
    {
        return SIGNAL_ALGORITHM_INVALID_ARGUMENT;
    }
    left = magnitude[peak_index - 1U];
    center = magnitude[peak_index];
    right = magnitude[peak_index + 1U];
    if (!isfinite(left) || !isfinite(center) || !isfinite(right) ||
        (left < 0.0f) || (center < 0.0f) || (right < 0.0f))
    {
        return SIGNAL_ALGORITHM_NUMERIC_ERROR;
    }
    if ((center < left) || (center < right))
    {
        return SIGNAL_ALGORITHM_NO_FEATURE;
    }
    denominator = left - (2.0f * center) + right;
    if (fabsf(denominator) <= 1.0e-20f)
    {
        return SIGNAL_ALGORITHM_NO_FEATURE;
    }
    offset = 0.5f * (left - right) / denominator;
    if (!isfinite(offset) || (offset < -0.5f) || (offset > 0.5f))
    {
        return SIGNAL_ALGORITHM_NUMERIC_ERROR;
    }
    result->bin_offset = offset;
    result->fractional_bin = (float)peak_index + offset;
    result->frequency_hz = result->fractional_bin * sample_rate_hz /
                           (float)fft_size;
    result->interpolated_magnitude = center -
        (0.25f * (left - right) * offset);
    return (isfinite(result->frequency_hz) &&
            isfinite(result->interpolated_magnitude))
               ? SIGNAL_ALGORITHM_OK
               : SIGNAL_ALGORITHM_NUMERIC_ERROR;
}
