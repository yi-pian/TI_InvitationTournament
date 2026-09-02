#include "signal_dual_adc_phase.h"

#include <stddef.h>

typedef struct
{
    uint16_t minimum;
    uint16_t maximum;
} signal_dual_adc_phase_bounds_t;

static void SignalDualADCPhase_FindBounds(
    const uint16_t *samples,
    uint32_t sample_count,
    signal_dual_adc_phase_bounds_t *bounds)
{
    uint32_t index;

    bounds->minimum = samples[0];
    bounds->maximum = samples[0];
    for (index = 1U; index < sample_count; ++index)
    {
        if (samples[index] < bounds->minimum)
        {
            bounds->minimum = samples[index];
        }
        if (samples[index] > bounds->maximum)
        {
            bounds->maximum = samples[index];
        }
    }
}

static uint16_t SignalDualADCPhase_FindRisingCrossings(
    const uint16_t *samples,
    uint32_t sample_count,
    uint16_t threshold,
    uint16_t hysteresis,
    uint32_t *crossings,
    uint16_t crossing_capacity)
{
    uint32_t index;
    uint16_t crossing_count = 0U;
    int32_t lower_level = (int32_t)threshold - (int32_t)hysteresis;
    uint8_t armed;

    if (lower_level < 0)
    {
        lower_level = 0;
    }
    armed = ((int32_t)samples[0] <= lower_level) ? 1U : 0U;

    for (index = 0U; (index + 1U < sample_count) &&
         (crossing_count < crossing_capacity); ++index)
    {
        int32_t difference;
        int32_t offset;

        if (armed == 0U)
        {
            if ((int32_t)samples[index] <= lower_level)
            {
                armed = 1U;
            }
            continue;
        }

        if ((samples[index] <= threshold) &&
            (samples[index + 1U] > threshold))
        {
            difference = (int32_t)samples[index + 1U] -
                         (int32_t)samples[index];
            offset = ((int32_t)threshold - (int32_t)samples[index]) << 16;
            crossings[crossing_count] = (index << 16) +
                (uint32_t)(offset / difference);
            ++crossing_count;
            armed = 0U;
        }
    }

    return crossing_count;
}

static uint32_t SignalDualADCPhase_AveragePeriod(
    const uint32_t *crossings,
    uint16_t crossing_count)
{
    if (crossing_count < 2U)
    {
        return 0U;
    }
    return (crossings[crossing_count - 1U] - crossings[0]) /
        (uint32_t)(crossing_count - 1U);
}

static int32_t SignalDualADCPhase_AverageWrapped(
    const int16_t *phases,
    uint16_t phase_count)
{
    int32_t base = phases[0];
    int32_t sum = base;
    uint16_t index;

    for (index = 1U; index < phase_count; ++index)
    {
        int32_t phase = phases[index];
        int32_t difference = phase - base;

        while (difference > 180)
        {
            phase -= 360;
            difference -= 360;
        }
        while (difference < -180)
        {
            phase += 360;
            difference += 360;
        }
        sum += phase;
    }

    sum /= (int32_t)phase_count;
    while (sum > 180)
    {
        sum -= 360;
    }
    while (sum < -180)
    {
        sum += 360;
    }
    return sum;
}

signal_algorithm_status_t SignalDualADCPhase_Process(
    const uint16_t *samples_x,
    const uint16_t *samples_y,
    uint32_t sample_count,
    uint32_t sample_rate_hz,
    const signal_dual_adc_phase_config_t *config,
    signal_dual_adc_phase_result_t *result)
{
    signal_dual_adc_phase_bounds_t bounds_x;
    signal_dual_adc_phase_bounds_t bounds_y;
    /* The application stack is 512 bytes; keep the bounded work buffers in
     * static storage instead of consuming roughly 776 bytes per call. */
    static uint32_t x_crossings[SIGNAL_DUAL_ADC_PHASE_MAX_X_CROSSINGS];
    static uint32_t y_crossings[SIGNAL_DUAL_ADC_PHASE_MAX_Y_CROSSINGS];
    static int16_t phase_values[SIGNAL_DUAL_ADC_PHASE_MAX_X_CROSSINGS];
    uint16_t x_threshold;
    uint16_t y_threshold;
    uint16_t x_count;
    uint16_t y_count;
    uint16_t phase_count = 0U;
    uint16_t x_index;
    uint16_t y_index = 0U;
    uint32_t x_period_q16;

    if ((samples_x == NULL) || (samples_y == NULL) ||
        (config == NULL) || (result == NULL))
    {
        return SIGNAL_ALGORITHM_INVALID_ARGUMENT;
    }
    result->phase_degrees = 0;
    result->valid_phase_count = 0U;
    result->x_crossing_count = 0U;
    result->y_crossing_count = 0U;
    result->valid = 0U;

    if ((sample_count < 2U) || (sample_count > 65535U))
    {
        return SIGNAL_ALGORITHM_INSUFFICIENT_DATA;
    }
    if ((sample_rate_hz == 0U) || (config->frequency_ratio < 1U) ||
        (config->frequency_ratio > 5U) ||
        (config->max_x_crossings == 0U) ||
        (config->max_x_crossings > SIGNAL_DUAL_ADC_PHASE_MAX_X_CROSSINGS) ||
        (config->max_y_crossings == 0U) ||
        (config->max_y_crossings > SIGNAL_DUAL_ADC_PHASE_MAX_Y_CROSSINGS))
    {
        return SIGNAL_ALGORITHM_INVALID_ARGUMENT;
    }

    SignalDualADCPhase_FindBounds(samples_x, sample_count, &bounds_x);
    SignalDualADCPhase_FindBounds(samples_y, sample_count, &bounds_y);
    if (((uint32_t)bounds_x.maximum - bounds_x.minimum) <
            config->min_amplitude_code ||
        ((uint32_t)bounds_y.maximum - bounds_y.minimum) <
            config->min_amplitude_code)
    {
        return SIGNAL_ALGORITHM_NO_FEATURE;
    }

    x_threshold = bounds_x.minimum +
        (uint16_t)(((uint32_t)bounds_x.maximum - bounds_x.minimum) / 2U);
    y_threshold = bounds_y.minimum +
        (uint16_t)(((uint32_t)bounds_y.maximum - bounds_y.minimum) / 2U);
    x_count = SignalDualADCPhase_FindRisingCrossings(
        samples_x, sample_count, x_threshold, config->hysteresis_code,
        x_crossings, config->max_x_crossings);
    y_count = SignalDualADCPhase_FindRisingCrossings(
        samples_y, sample_count, y_threshold, config->hysteresis_code,
        y_crossings, config->max_y_crossings);
    result->x_crossing_count = x_count;
    result->y_crossing_count = y_count;
    x_period_q16 = SignalDualADCPhase_AveragePeriod(x_crossings, x_count);
    if ((x_period_q16 == 0U) || (y_count < 2U))
    {
        return SIGNAL_ALGORITHM_INSUFFICIENT_DATA;
    }

    for (x_index = 0U; x_index < x_count; ++x_index)
    {
        uint32_t x_crossing = x_crossings[x_index];
        uint32_t before_crossing;
        uint32_t after_crossing;
        int64_t before_distance;
        int64_t after_distance;
        int64_t delta_q16;
        int64_t phase;

        while ((y_index < y_count) &&
               (y_crossings[y_index] < x_crossing))
        {
            ++y_index;
        }
        if ((y_index == 0U) || (y_index >= y_count))
        {
            continue;
        }

        before_crossing = y_crossings[y_index - 1U];
        after_crossing = y_crossings[y_index];
        before_distance = (int64_t)x_crossing - before_crossing;
        after_distance = (int64_t)after_crossing - x_crossing;
        delta_q16 = (before_distance <= after_distance) ?
            (int64_t)x_crossing - before_crossing :
            (int64_t)x_crossing - after_crossing;
        phase = (delta_q16 * 360LL * (int64_t)config->frequency_ratio) /
                (int64_t)x_period_q16;
        if (phase > 180)
        {
            phase = 180;
        }
        if (phase < -180)
        {
            phase = -180;
        }
        phase_values[phase_count] = (int16_t)phase;
        ++phase_count;
    }

    if (phase_count == 0U)
    {
        return SIGNAL_ALGORITHM_INSUFFICIENT_DATA;
    }
    result->phase_degrees = (int16_t)SignalDualADCPhase_AverageWrapped(
        phase_values, phase_count);
    result->valid_phase_count = phase_count;
    result->valid = 1U;
    return SIGNAL_ALGORITHM_OK;
}
