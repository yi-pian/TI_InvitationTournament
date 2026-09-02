#include "signal_median_filter.h"

#include <math.h>
#include <stddef.h>

static void SignalMedianFilter_InsertionSort(float *values, uint32_t count)
{
    uint32_t index;

    for (index = 1U; index < count; ++index)
    {
        float value = values[index];
        uint32_t position = index;
        while ((position > 0U) && (values[position - 1U] > value))
        {
            values[position] = values[position - 1U];
            --position;
        }
        values[position] = value;
    }
}

signal_algorithm_status_t SignalMedianFilter_Process(
    const float *input_samples,
    float *output_samples,
    uint32_t count,
    uint32_t window_size,
    float *workspace,
    uint32_t workspace_count)
{
    uint32_t index;
    uint32_t half_window;

    if ((input_samples == NULL) || (output_samples == NULL) ||
        (workspace == NULL) || (input_samples == output_samples))
    {
        return SIGNAL_ALGORITHM_INVALID_ARGUMENT;
    }
    if ((count == 0U) || (window_size == 0U) ||
        (window_size > count) || ((window_size & 1U) == 0U))
    {
        return SIGNAL_ALGORITHM_INSUFFICIENT_DATA;
    }
    if (workspace_count < window_size)
    {
        return SIGNAL_ALGORITHM_BUFFER_TOO_SMALL;
    }
    for (index = 0U; index < count; ++index)
    {
        if (!isfinite(input_samples[index]))
        {
            return SIGNAL_ALGORITHM_NUMERIC_ERROR;
        }
    }

    half_window = window_size / 2U;
    for (index = 0U; index < count; ++index)
    {
        uint32_t start = (index > half_window) ? (index - half_window) : 0U;
        uint32_t end = index + half_window;
        uint32_t local_count;
        uint32_t local_index;

        if (end >= count)
        {
            end = count - 1U;
        }
        local_count = end - start + 1U;
        for (local_index = 0U; local_index < local_count; ++local_index)
        {
            workspace[local_index] = input_samples[start + local_index];
        }
        SignalMedianFilter_InsertionSort(workspace, local_count);
        if ((local_count & 1U) != 0U)
        {
            output_samples[index] = workspace[local_count / 2U];
        }
        else
        {
            output_samples[index] = 0.5f *
                (workspace[(local_count / 2U) - 1U] +
                 workspace[local_count / 2U]);
        }
    }
    return SIGNAL_ALGORITHM_OK;
}
