#include "signal_slew_rate.h"

#include <math.h>

static float crossing(float y0, float y1, float threshold, uint32_t index)
{
    float delta = y1 - y0;
    float fraction = 0.0f;
    if (fabsf(delta) > 0.000001f) {
        fraction = (threshold - y0) / delta;
    }
    if (fraction < 0.0f) fraction = 0.0f;
    if (fraction > 1.0f) fraction = 1.0f;
    return (float)index + fraction;
}

signal_algorithm_status_t SignalSlewRate_Process(
    const float *samples, uint32_t sample_count, float low_voltage_v,
    float high_voltage_v, uint32_t sample_rate_hz,
    const signal_slew_rate_config_t *config,
    signal_slew_rate_result_t *result)
{
    uint32_t index = 1U;
    float rise_sum = 0.0f;
    float fall_sum = 0.0f;
    float span;
    float low_threshold;
    float high_threshold;

    if ((samples == 0) || (config == 0) || (result == 0) ||
        (sample_count < 2U) || (sample_rate_hz == 0U) ||
        (high_voltage_v <= low_voltage_v) ||
        (config->low_ratio < 0.0f) ||
        (config->high_ratio > 1.0f) ||
        (config->high_ratio <= config->low_ratio)) {
        return SIGNAL_ALGORITHM_INVALID_ARGUMENT;
    }

    result->rise_time_us = 0.0f;
    result->fall_time_us = 0.0f;
    result->rise_count = 0U;
    result->fall_count = 0U;
    span = high_voltage_v - low_voltage_v;
    low_threshold = low_voltage_v + config->low_ratio * span;
    high_threshold = low_voltage_v + config->high_ratio * span;

    while (index < sample_count) {
        if ((samples[index - 1U] < low_threshold) &&
            (samples[index] >= low_threshold)) {
            float start = crossing(samples[index - 1U], samples[index],
                low_threshold, index - 1U);
            uint32_t search = index + 1U;
            while (search < sample_count) {
                if ((samples[search - 1U] < high_threshold) &&
                    (samples[search] >= high_threshold)) {
                    float stop = crossing(samples[search - 1U], samples[search],
                        high_threshold, search - 1U);
                    if (stop > start) {
                        rise_sum += stop - start;
                        ++result->rise_count;
                    }
                    index = search + 1U;
                    break;
                }
                ++search;
            }
            if (search >= sample_count) break;
            continue;
        }
        if ((samples[index - 1U] > high_threshold) &&
            (samples[index] <= high_threshold)) {
            float start = crossing(samples[index - 1U], samples[index],
                high_threshold, index - 1U);
            uint32_t search = index + 1U;
            while (search < sample_count) {
                if ((samples[search - 1U] > low_threshold) &&
                    (samples[search] <= low_threshold)) {
                    float stop = crossing(samples[search - 1U], samples[search],
                        low_threshold, search - 1U);
                    if (stop > start) {
                        fall_sum += stop - start;
                        ++result->fall_count;
                    }
                    index = search + 1U;
                    break;
                }
                ++search;
            }
            if (search >= sample_count) break;
            continue;
        }
        ++index;
    }

    if (result->rise_count > 0U) {
        result->rise_time_us = rise_sum * 1000000.0f /
            ((float)result->rise_count * (float)sample_rate_hz);
    }
    if (result->fall_count > 0U) {
        result->fall_time_us = fall_sum * 1000000.0f /
            ((float)result->fall_count * (float)sample_rate_hz);
    }
    return (result->rise_count == 0U && result->fall_count == 0U) ?
        SIGNAL_ALGORITHM_INSUFFICIENT_DATA : SIGNAL_ALGORITHM_OK;
}
