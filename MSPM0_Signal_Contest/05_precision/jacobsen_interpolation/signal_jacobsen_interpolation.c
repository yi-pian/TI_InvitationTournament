#include "signal_jacobsen_interpolation.h"

#include <math.h>
#include <stddef.h>

signal_algorithm_status_t SignalJacobsen_Process(
    const signal_complex_f32_t *spectrum,
    uint32_t spectrum_count,
    uint32_t peak_index,
    float sample_rate_hz,
    uint32_t fft_size,
    signal_jacobsen_result_t *result)
{
    signal_jacobsen_result_t temporary;
    const signal_complex_f32_t *left;
    const signal_complex_f32_t *center;
    const signal_complex_f32_t *right;
    float numerator_real;
    float numerator_imag;
    float denominator_real;
    float denominator_imag;
    float denominator_power;
    float center_power;
    float left_power;
    float right_power;

    if ((spectrum == NULL) || (result == NULL))
    {
        return SIGNAL_ALGORITHM_INVALID_ARGUMENT;
    }
    if ((spectrum_count < 3U) || (fft_size < 2U))
    {
        return SIGNAL_ALGORITHM_INSUFFICIENT_DATA;
    }
    if ((peak_index == 0U) || ((peak_index + 1U) >= spectrum_count) ||
        !isfinite(sample_rate_hz) || (sample_rate_hz <= 0.0f))
    {
        return SIGNAL_ALGORITHM_OUT_OF_RANGE;
    }

    left = &spectrum[peak_index - 1U];
    center = &spectrum[peak_index];
    right = &spectrum[peak_index + 1U];
    if (!isfinite(left->real) || !isfinite(left->imag) ||
        !isfinite(center->real) || !isfinite(center->imag) ||
        !isfinite(right->real) || !isfinite(right->imag))
    {
        return SIGNAL_ALGORITHM_NUMERIC_ERROR;
    }

    center_power = (center->real * center->real) + (center->imag * center->imag);
    left_power = (left->real * left->real) + (left->imag * left->imag);
    right_power = (right->real * right->real) + (right->imag * right->imag);
    if ((center_power <= 1.0e-30f) || (center_power < left_power) ||
        (center_power < right_power))
    {
        return SIGNAL_ALGORITHM_NO_FEATURE;
    }

    numerator_real = left->real - right->real;
    numerator_imag = left->imag - right->imag;
    denominator_real = (2.0f * center->real) - left->real - right->real;
    denominator_imag = (2.0f * center->imag) - left->imag - right->imag;
    denominator_power = (denominator_real * denominator_real) +
                        (denominator_imag * denominator_imag);
    if (!isfinite(denominator_power) || (denominator_power <= 1.0e-30f))
    {
        return SIGNAL_ALGORITHM_NO_FEATURE;
    }

    temporary.fractional_bin =
        ((numerator_real * denominator_real) +
         (numerator_imag * denominator_imag)) / denominator_power;
    if (!isfinite(temporary.fractional_bin))
    {
        return SIGNAL_ALGORITHM_NUMERIC_ERROR;
    }
    if (fabsf(temporary.fractional_bin) > 0.5001f)
    {
        return SIGNAL_ALGORITHM_NO_FEATURE;
    }
    temporary.interpolated_bin = (float)peak_index + temporary.fractional_bin;
    temporary.frequency_hz = temporary.interpolated_bin * sample_rate_hz /
                             (float)fft_size;
    *result = temporary;
    return SIGNAL_ALGORITHM_OK;
}
