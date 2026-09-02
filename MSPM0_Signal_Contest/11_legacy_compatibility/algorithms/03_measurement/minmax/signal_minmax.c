#include "signal_minmax.h"

#include <math.h>
#include <stddef.h>

signal_algorithm_status_t SignalMinMax_Process(
    const float *samples,
    uint32_t count,
    signal_minmax_result_t *result)
{
    uint32_t index;
    float min_value;
    float max_value;
    uint32_t min_index = 0U;
    uint32_t max_index = 0U;

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
    min_value = samples[0];
    max_value = samples[0];
    for (index = 1U; index < count; ++index)
    {
        if (!isfinite(samples[index]))
        {
            return SIGNAL_ALGORITHM_NUMERIC_ERROR;
        }
        if (samples[index] < min_value)
        {
            min_value = samples[index];
            min_index = index;
        }
        if (samples[index] > max_value)
        {
            max_value = samples[index];
            max_index = index;
        }
    }
    result->min_value = min_value;
    result->max_value = max_value;
    result->min_index = min_index;
    result->max_index = max_index;
    return SIGNAL_ALGORITHM_OK;
}
