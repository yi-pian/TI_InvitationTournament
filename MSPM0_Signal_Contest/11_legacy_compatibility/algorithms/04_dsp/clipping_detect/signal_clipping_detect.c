#include "signal_clipping_detect.h"

#include <math.h>
#include <stddef.h>

signal_algorithm_status_t SignalClippingDetect_Process(
    const float *voltage_v,
    uint32_t count,
    const signal_clipping_detect_config_t *config,
    signal_clipping_detect_result_t *result)
{
    uint32_t index;
    uint32_t low_count = 0U;
    uint32_t high_count = 0U;

    if ((voltage_v == NULL) || (config == NULL) || (result == NULL))
    {
        return SIGNAL_ALGORITHM_INVALID_ARGUMENT;
    }
    if (count == 0U)
    {
        return SIGNAL_ALGORITHM_INSUFFICIENT_DATA;
    }
    if (!isfinite(config->low_limit_v) ||
        !isfinite(config->high_limit_v) ||
        (config->low_limit_v >= config->high_limit_v))
    {
        return SIGNAL_ALGORITHM_INVALID_ARGUMENT;
    }

    for (index = 0U; index < count; ++index)
    {
        if (!isfinite(voltage_v[index]))
        {
            return SIGNAL_ALGORITHM_NUMERIC_ERROR;
        }
        if (voltage_v[index] <= config->low_limit_v)
        {
            ++low_count;
        }
        else if (voltage_v[index] >= config->high_limit_v)
        {
            ++high_count;
        }
    }

    result->low_clipped_count = low_count;
    result->high_clipped_count = high_count;
    result->clipped_count = low_count + high_count;
    result->clipped_ratio = (float)result->clipped_count / (float)count;
    result->is_clipped = (result->clipped_count > 0U) ? 1U : 0U;
    return SIGNAL_ALGORITHM_OK;
}
