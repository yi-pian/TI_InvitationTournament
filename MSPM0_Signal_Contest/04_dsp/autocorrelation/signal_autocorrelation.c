#include "signal_autocorrelation.h"

#include <math.h>
#include <stddef.h>

signal_algorithm_status_t SignalAutocorrelation_Process(
    const float *samples,
    uint32_t count,
    uint32_t max_lag_samples,
    float *coefficients,
    uint32_t coefficient_capacity,
    signal_autocorrelation_result_t *result)
{
    uint32_t lag;
    uint32_t index;

    if ((samples == NULL) || (coefficients == NULL) || (result == NULL))
    {
        return SIGNAL_ALGORITHM_INVALID_ARGUMENT;
    }
    if ((count == 0U) || (max_lag_samples >= count))
    {
        return SIGNAL_ALGORITHM_OUT_OF_RANGE;
    }
    if (coefficient_capacity < (max_lag_samples + 1U))
    {
        return SIGNAL_ALGORITHM_BUFFER_TOO_SMALL;
    }
    for (index = 0U; index < count; ++index)
    {
        if (!isfinite(samples[index]))
        {
            return SIGNAL_ALGORITHM_NUMERIC_ERROR;
        }
    }

    for (lag = 0U; lag <= max_lag_samples; ++lag)
    {
        float cross_sum = 0.0f;
        float energy_left = 0.0f;
        float energy_right = 0.0f;
        uint32_t sample_index;

        for (sample_index = 0U; sample_index < (count - lag); ++sample_index)
        {
            float left = samples[sample_index];
            float right = samples[sample_index + lag];
            cross_sum += left * right;
            energy_left += left * left;
            energy_right += right * right;
        }
        if ((energy_left <= 0.0f) || (energy_right <= 0.0f))
        {
            return SIGNAL_ALGORITHM_NO_FEATURE;
        }
        coefficients[lag] = cross_sum / sqrtf(energy_left * energy_right);
        if (!isfinite(coefficients[lag]))
        {
            return SIGNAL_ALGORITHM_NUMERIC_ERROR;
        }
    }
    result->lag_count = max_lag_samples + 1U;
    return SIGNAL_ALGORITHM_OK;
}

signal_algorithm_status_t SignalAutocorrelation_FindPeriod(
    const float *coefficients,
    uint32_t coefficient_count,
    uint32_t min_lag_samples,
    uint32_t max_lag_samples,
    float sample_rate_hz,
    signal_autocorrelation_period_result_t *result)
{
    uint32_t lag;
    uint32_t best_lag;
    float best_coefficient;

    if ((coefficients == NULL) || (result == NULL))
    {
        return SIGNAL_ALGORITHM_INVALID_ARGUMENT;
    }
    if ((coefficient_count == 0U) || (min_lag_samples == 0U) ||
        (min_lag_samples > max_lag_samples) ||
        (max_lag_samples >= coefficient_count))
    {
        return SIGNAL_ALGORITHM_OUT_OF_RANGE;
    }
    if (!isfinite(sample_rate_hz) || (sample_rate_hz <= 0.0f))
    {
        return SIGNAL_ALGORITHM_INVALID_ARGUMENT;
    }
    best_lag = min_lag_samples;
    best_coefficient = coefficients[min_lag_samples];
    if (!isfinite(best_coefficient))
    {
        return SIGNAL_ALGORITHM_NUMERIC_ERROR;
    }
    for (lag = min_lag_samples + 1U; lag <= max_lag_samples; ++lag)
    {
        if (!isfinite(coefficients[lag]))
        {
            return SIGNAL_ALGORITHM_NUMERIC_ERROR;
        }
        if (coefficients[lag] > best_coefficient)
        {
            best_coefficient = coefficients[lag];
            best_lag = lag;
        }
    }
    if (best_coefficient <= 0.0f)
    {
        return SIGNAL_ALGORITHM_NO_FEATURE;
    }
    result->period_lag_samples = best_lag;
    result->peak_coefficient = best_coefficient;
    result->frequency_hz = sample_rate_hz / (float)best_lag;
    return SIGNAL_ALGORITHM_OK;
}
