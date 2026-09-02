#include "signal_correlation.h"

#include <limits.h>
#include <math.h>
#include <stddef.h>

signal_algorithm_status_t SignalCorrelation_Process(
    const float *samples_a,
    const float *samples_b,
    uint32_t count,
    uint32_t max_lag_samples,
    float *coefficients,
    uint32_t coefficient_capacity,
    signal_correlation_result_t *result)
{
    int32_t lag;
    uint32_t required_count;
    float best_coefficient = -2.0f;
    float best_absolute = -1.0f;
    int32_t best_lag = 0;
    int32_t best_absolute_lag = 0;
    float best_absolute_coefficient = 0.0f;
    uint32_t index;

    if ((samples_a == NULL) || (samples_b == NULL) ||
        (coefficients == NULL) || (result == NULL))
    {
        return SIGNAL_ALGORITHM_INVALID_ARGUMENT;
    }
    if ((count == 0U) || (count > (uint32_t)INT32_MAX) ||
        (max_lag_samples >= count) ||
        (max_lag_samples > ((uint32_t)INT32_MAX / 2U)))
    {
        return SIGNAL_ALGORITHM_OUT_OF_RANGE;
    }
    required_count = (2U * max_lag_samples) + 1U;
    if (coefficient_capacity < required_count)
    {
        return SIGNAL_ALGORITHM_BUFFER_TOO_SMALL;
    }
    for (index = 0U; index < count; ++index)
    {
        if (!isfinite(samples_a[index]) || !isfinite(samples_b[index]))
        {
            return SIGNAL_ALGORITHM_NUMERIC_ERROR;
        }
    }

    for (lag = -(int32_t)max_lag_samples;
         lag <= (int32_t)max_lag_samples; ++lag)
    {
        uint32_t start_a = (lag < 0) ? (uint32_t)(-lag) : 0U;
        uint32_t end_a = (lag > 0) ? (count - (uint32_t)lag) : count;
        uint32_t sample_index;
        float cross_sum = 0.0f;
        float energy_a = 0.0f;
        float energy_b = 0.0f;
        float coefficient;
        uint32_t output_index = (uint32_t)(lag + (int32_t)max_lag_samples);

        for (sample_index = start_a; sample_index < end_a; ++sample_index)
        {
            uint32_t index_b = (uint32_t)((int32_t)sample_index + lag);
            float a = samples_a[sample_index];
            float b = samples_b[index_b];
            cross_sum += a * b;
            energy_a += a * a;
            energy_b += b * b;
        }
        if ((energy_a <= 0.0f) || (energy_b <= 0.0f))
        {
            return SIGNAL_ALGORITHM_NO_FEATURE;
        }
        coefficient = cross_sum / sqrtf(energy_a * energy_b);
        if (!isfinite(coefficient))
        {
            return SIGNAL_ALGORITHM_NUMERIC_ERROR;
        }
        coefficients[output_index] = coefficient;
        if (coefficient > best_coefficient)
        {
            best_coefficient = coefficient;
            best_lag = lag;
        }
        if (fabsf(coefficient) > best_absolute)
        {
            best_absolute = fabsf(coefficient);
            best_absolute_coefficient = coefficient;
            best_absolute_lag = lag;
        }
    }
    result->best_lag_samples = best_lag;
    result->best_coefficient = best_coefficient;
    result->best_absolute_lag_samples = best_absolute_lag;
    result->best_absolute_coefficient = best_absolute_coefficient;
    return SIGNAL_ALGORITHM_OK;
}
