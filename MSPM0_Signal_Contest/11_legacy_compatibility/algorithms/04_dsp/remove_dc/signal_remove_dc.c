#include "signal_remove_dc.h"

#include <math.h>
#include <stddef.h>

signal_algorithm_status_t SignalRemoveDC_Process(
    const float *input_voltage_v,
    float *output_centered_v,
    uint32_t count,
    signal_remove_dc_result_t *result)
{
    uint32_t index;
    float sum = 0.0f;
    float compensation = 0.0f;
    float mean_voltage_v;

    if ((input_voltage_v == NULL) || (output_centered_v == NULL) ||
        (result == NULL))
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

        if (!isfinite(input_voltage_v[index]))
        {
            return SIGNAL_ALGORITHM_NUMERIC_ERROR;
        }
        corrected = input_voltage_v[index] - compensation;
        next_sum = sum + corrected;
        compensation = (next_sum - sum) - corrected;
        sum = next_sum;
    }
    mean_voltage_v = sum / (float)count;

    for (index = 0U; index < count; ++index)
    {
        output_centered_v[index] = input_voltage_v[index] - mean_voltage_v;
    }
    result->removed_mean_v = mean_voltage_v;
    return SIGNAL_ALGORITHM_OK;
}
