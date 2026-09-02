#include "signal_statistics.h"
#include "signal_math_backend.h"

#include <math.h>
#include <stddef.h>

signal_algorithm_status_t SignalStatistics_Process(
    const float *samples,
    uint32_t count,
    signal_statistics_result_t *result)
{
    uint32_t index;
    float mean;
    float m2 = 0.0f;
    float min_value;
    float max_value;

    if ((samples == NULL) || (result == NULL))
    {
        return SIGNAL_ALGORITHM_INVALID_ARGUMENT;
    }
    if (count == 0U)
    {
        return SIGNAL_ALGORITHM_INSUFFICIENT_DATA;
    }
    if (!isfinite(samples[0]))
    {
        return SIGNAL_ALGORITHM_NUMERIC_ERROR;
    }

    mean = samples[0];
    min_value = samples[0];
    max_value = samples[0];
    for (index = 1U; index < count; ++index)
    {
        float delta;
        float delta_after_update;

        if (!isfinite(samples[index]))
        {
            return SIGNAL_ALGORITHM_NUMERIC_ERROR;
        }
        delta = samples[index] - mean;
        mean += delta / (float)(index + 1U);
        delta_after_update = samples[index] - mean;
        m2 += delta * delta_after_update;
        if (samples[index] < min_value)
        {
            min_value = samples[index];
        }
        if (samples[index] > max_value)
        {
            max_value = samples[index];
        }
    }

    /* 浮点舍入可能得到极小负值；只有这种量级才钳到 0。 */
    if ((m2 < 0.0f) && (m2 > -1.0e-12f))
    {
        m2 = 0.0f;
    }
    if (!isfinite(m2) || (m2 < 0.0f))
    {
        return SIGNAL_ALGORITHM_NUMERIC_ERROR;
    }

    result->count = count;
    result->mean_value = mean;
    result->min_value = min_value;
    result->max_value = max_value;
    result->population_variance = m2 / (float)count;
    result->population_stddev = SignalMathBackend_SqrtF(result->population_variance);
    if (count > 1U)
    {
        result->sample_variance = m2 / (float)(count - 1U);
        result->sample_stddev = SignalMathBackend_SqrtF(result->sample_variance);
        result->sample_variance_valid = 1U;
    }
    else
    {
        result->sample_variance = 0.0f;
        result->sample_stddev = 0.0f;
        result->sample_variance_valid = 0U;
    }
    return SIGNAL_ALGORITHM_OK;
}
