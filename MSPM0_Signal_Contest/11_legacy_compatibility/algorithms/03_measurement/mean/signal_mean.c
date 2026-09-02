#include "signal_mean.h"

#include <math.h>
#include <stddef.h>

signal_algorithm_status_t SignalMean_Process(
    const float *samples,
    uint32_t count,
    signal_mean_result_t *result)
{
    uint32_t index;
    float sum = 0.0f;
    float compensation = 0.0f;

    if ((samples == NULL) || (result == NULL))
    {
        return SIGNAL_ALGORITHM_INVALID_ARGUMENT;
    }
    if (count == 0U)
    {
        return SIGNAL_ALGORITHM_INSUFFICIENT_DATA;
    }

    for (index = 0U; index < count; ++index)
    {
        float corrected;
        float next_sum;

        if (!isfinite(samples[index]))
        {
            return SIGNAL_ALGORITHM_NUMERIC_ERROR;
        }
        corrected = samples[index] - compensation;
        next_sum = sum + corrected;
        compensation = (next_sum - sum) - corrected;
        sum = next_sum;
    }
    result->mean_value = sum / (float)count;
    return isfinite(result->mean_value) ? SIGNAL_ALGORITHM_OK
                                        : SIGNAL_ALGORITHM_NUMERIC_ERROR;
}
