#include "signal_peak_detect.h"

#include <math.h>
#include <stddef.h>

signal_algorithm_status_t SignalPeakDetect_Process(
    const float *values,
    uint32_t count,
    uint32_t start_index,
    uint32_t end_index,
    signal_peak_detect_result_t *result)
{
    uint32_t index;
    uint32_t peak_index;
    float peak_value;

    if ((values == NULL) || (result == NULL))
    {
        return SIGNAL_ALGORITHM_INVALID_ARGUMENT;
    }
    if ((count == 0U) || (start_index > end_index) || (end_index >= count))
    {
        return SIGNAL_ALGORITHM_OUT_OF_RANGE;
    }
    if (!isfinite(values[start_index]))
    {
        return SIGNAL_ALGORITHM_NUMERIC_ERROR;
    }
    peak_index = start_index;
    peak_value = values[start_index];
    for (index = start_index + 1U; index <= end_index; ++index)
    {
        if (!isfinite(values[index]))
        {
            return SIGNAL_ALGORITHM_NUMERIC_ERROR;
        }
        if (values[index] > peak_value)
        {
            peak_value = values[index];
            peak_index = index;
        }
    }
    result->peak_index = peak_index;
    result->peak_value = peak_value;
    return SIGNAL_ALGORITHM_OK;
}
