#include "signal_moving_average.h"

#include <math.h>
#include <stddef.h>

signal_algorithm_status_t SignalMovingAverage_Process(
    const float *input_samples,
    float *output_samples,
    uint32_t count,
    uint32_t window_size)
{
    uint32_t index;
    float running_sum = 0.0f;

    if ((input_samples == NULL) || (output_samples == NULL) ||
        (input_samples == output_samples))
    {
        return SIGNAL_ALGORITHM_INVALID_ARGUMENT;
    }
    if ((count == 0U) || (window_size == 0U) || (window_size > count))
    {
        return SIGNAL_ALGORITHM_INSUFFICIENT_DATA;
    }
    for (index = 0U; index < count; ++index)
    {
        if (!isfinite(input_samples[index]))
        {
            return SIGNAL_ALGORITHM_NUMERIC_ERROR;
        }
    }

    for (index = 0U; index < count; ++index)
    {
        uint32_t active_count;

        running_sum += input_samples[index];
        if (index >= window_size)
        {
            running_sum -= input_samples[index - window_size];
            active_count = window_size;
        }
        else
        {
            active_count = index + 1U;
        }
        output_samples[index] = running_sum / (float)active_count;
        if (!isfinite(output_samples[index]))
        {
            return SIGNAL_ALGORITHM_NUMERIC_ERROR;
        }
    }
    return SIGNAL_ALGORITHM_OK;
}
