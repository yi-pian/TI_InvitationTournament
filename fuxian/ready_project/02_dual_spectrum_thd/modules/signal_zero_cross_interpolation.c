#include "signal_zero_cross_interpolation.h"

#include <math.h>
#include <stddef.h>

signal_algorithm_status_t SignalZeroCrossInterpolation_Process(
    const float *voltage_v,
    uint32_t sample_count,
    float threshold_v,
    const signal_zero_cross_event_t *events,
    uint32_t event_count,
    float *crossing_positions_samples,
    uint32_t position_capacity,
    signal_zero_cross_interpolation_result_t *result)
{
    uint32_t event_index;

    if ((voltage_v == NULL) || (events == NULL) ||
        (crossing_positions_samples == NULL) || (result == NULL))
    {
        return SIGNAL_ALGORITHM_INVALID_ARGUMENT;
    }
    if ((sample_count < 2U) || (event_count == 0U))
    {
        return SIGNAL_ALGORITHM_INSUFFICIENT_DATA;
    }
    if (position_capacity < event_count)
    {
        return SIGNAL_ALGORITHM_BUFFER_TOO_SMALL;
    }
    if (!isfinite(threshold_v))
    {
        return SIGNAL_ALGORITHM_INVALID_ARGUMENT;
    }

    /* 先验证所有事件，避免错误时写出部分位置。 */
    for (event_index = 0U; event_index < event_count; ++event_index)
    {
        uint32_t left = events[event_index].left_index;
        uint32_t right = events[event_index].right_index;
        float left_v;
        float right_v;

        if ((right != (left + 1U)) || (right >= sample_count))
        {
            return SIGNAL_ALGORITHM_OUT_OF_RANGE;
        }
        left_v = voltage_v[left];
        right_v = voltage_v[right];
        if (!isfinite(left_v) || !isfinite(right_v) || (left_v == right_v))
        {
            return SIGNAL_ALGORITHM_NUMERIC_ERROR;
        }
        if (((threshold_v < left_v) && (threshold_v < right_v)) ||
            ((threshold_v > left_v) && (threshold_v > right_v)))
        {
            return SIGNAL_ALGORITHM_OUT_OF_RANGE;
        }
    }

    for (event_index = 0U; event_index < event_count; ++event_index)
    {
        uint32_t left = events[event_index].left_index;
        uint32_t right = events[event_index].right_index;
        float left_v = voltage_v[left];
        float right_v = voltage_v[right];
        float fraction = (threshold_v - left_v) / (right_v - left_v);

        if (!isfinite(fraction) || (fraction < 0.0f) || (fraction > 1.0f))
        {
            return SIGNAL_ALGORITHM_NUMERIC_ERROR;
        }
        crossing_positions_samples[event_index] = (float)left + fraction;
    }

    result->position_count = event_count;
    return SIGNAL_ALGORITHM_OK;
}
