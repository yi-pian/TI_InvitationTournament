#include "signal_zero_cross.h"

#include <math.h>
#include <stddef.h>

signal_algorithm_status_t SignalZeroCross_Process(
    const float *voltage_v,
    uint32_t count,
    const signal_zero_cross_config_t *config,
    signal_zero_cross_event_t *events,
    uint32_t event_capacity,
    signal_zero_cross_result_t *result)
{
    uint32_t index;
    uint32_t event_count = 0U;
    uint32_t rising_count = 0U;
    uint32_t falling_count = 0U;
    uint8_t rising_armed;
    uint8_t falling_armed;
    float lower_arm_v;
    float upper_arm_v;

    if ((voltage_v == NULL) || (config == NULL) ||
        (events == NULL) || (result == NULL))
    {
        return SIGNAL_ALGORITHM_INVALID_ARGUMENT;
    }
    if ((count < 2U) || (event_capacity == 0U))
    {
        return SIGNAL_ALGORITHM_INSUFFICIENT_DATA;
    }
    if (!isfinite(config->threshold_v) ||
        !isfinite(config->hysteresis_v) ||
        (config->hysteresis_v < 0.0f) ||
        (config->direction > SIGNAL_ZERO_CROSS_BOTH))
    {
        return SIGNAL_ALGORITHM_INVALID_ARGUMENT;
    }
    if (!isfinite(voltage_v[0]))
    {
        return SIGNAL_ALGORITHM_NUMERIC_ERROR;
    }

    lower_arm_v = config->threshold_v - config->hysteresis_v;
    upper_arm_v = config->threshold_v + config->hysteresis_v;
    if (!isfinite(lower_arm_v) || !isfinite(upper_arm_v))
    {
        return SIGNAL_ALGORITHM_NUMERIC_ERROR;
    }

    rising_armed = (voltage_v[0] <= lower_arm_v) ? 1U : 0U;
    falling_armed = (voltage_v[0] >= upper_arm_v) ? 1U : 0U;

    for (index = 1U; index < count; ++index)
    {
        float previous_v = voltage_v[index - 1U];
        float current_v = voltage_v[index];

        if (!isfinite(current_v))
        {
            return SIGNAL_ALGORITHM_NUMERIC_ERROR;
        }

        if (((config->direction == SIGNAL_ZERO_CROSS_RISING) ||
             (config->direction == SIGNAL_ZERO_CROSS_BOTH)) &&
            (rising_armed != 0U) &&
            (previous_v < config->threshold_v) &&
            (current_v >= config->threshold_v))
        {
            if (event_count >= event_capacity)
            {
                return SIGNAL_ALGORITHM_BUFFER_TOO_SMALL;
            }
            events[event_count].left_index = index - 1U;
            events[event_count].right_index = index;
            events[event_count].direction = SIGNAL_ZERO_CROSS_RISING;
            ++event_count;
            ++rising_count;
            rising_armed = 0U;
        }

        if (((config->direction == SIGNAL_ZERO_CROSS_FALLING) ||
             (config->direction == SIGNAL_ZERO_CROSS_BOTH)) &&
            (falling_armed != 0U) &&
            (previous_v > config->threshold_v) &&
            (current_v <= config->threshold_v))
        {
            if (event_count >= event_capacity)
            {
                return SIGNAL_ALGORITHM_BUFFER_TOO_SMALL;
            }
            events[event_count].left_index = index - 1U;
            events[event_count].right_index = index;
            events[event_count].direction = SIGNAL_ZERO_CROSS_FALLING;
            ++event_count;
            ++falling_count;
            falling_armed = 0U;
        }

        if (current_v <= lower_arm_v)
        {
            rising_armed = 1U;
        }
        if (current_v >= upper_arm_v)
        {
            falling_armed = 1U;
        }
    }

    if (event_count == 0U)
    {
        return SIGNAL_ALGORITHM_NO_FEATURE;
    }
    result->event_count = event_count;
    result->rising_count = rising_count;
    result->falling_count = falling_count;
    return SIGNAL_ALGORITHM_OK;
}
