#include "signal_quinn_interpolation.h"

#include <math.h>
#include <stddef.h>

static float signal_quinn_tau(float squared_delta)
{
    const float root_six = 2.449489742783178f;
    const float root_two_thirds = 0.816496580927726f;
    float first;
    float ratio;

    first = (3.0f * squared_delta * squared_delta) +
            (6.0f * squared_delta) + 1.0f;
    ratio = (squared_delta + 1.0f - root_two_thirds) /
            (squared_delta + 1.0f + root_two_thirds);
    return (0.25f * logf(first)) - ((root_six / 24.0f) * logf(ratio));
}

signal_algorithm_status_t SignalQuinnSecond_Process(
    const signal_complex_f32_t *spectrum,
    uint32_t spectrum_count,
    uint32_t peak_index,
    float sample_rate_hz,
    uint32_t fft_size,
    signal_quinn_result_t *result)
{
    signal_quinn_result_t temporary;
    const signal_complex_f32_t *left;
    const signal_complex_f32_t *center;
    const signal_complex_f32_t *right;
    float center_power;
    float left_power;
    float right_power;
    float beta_minus;
    float beta_plus;
    float delta_minus;
    float delta_plus;

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
    beta_minus = ((left->real * center->real) + (left->imag * center->imag)) /
                 center_power;
    beta_plus = ((right->real * center->real) + (right->imag * center->imag)) /
                center_power;
    if ((fabsf(1.0f - beta_minus) <= 1.0e-12f) ||
        (fabsf(1.0f - beta_plus) <= 1.0e-12f))
    {
        return SIGNAL_ALGORITHM_NO_FEATURE;
    }
    delta_minus = beta_minus / (1.0f - beta_minus);
    delta_plus = -beta_plus / (1.0f - beta_plus);
    temporary.fractional_bin = 0.5f * (delta_minus + delta_plus) +
        signal_quinn_tau(delta_plus * delta_plus) -
        signal_quinn_tau(delta_minus * delta_minus);
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
