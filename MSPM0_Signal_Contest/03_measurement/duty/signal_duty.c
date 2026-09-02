#include "signal_duty.h"

#include <stdbool.h>
#include <math.h>
#include <stddef.h>

typedef enum
{
    SIGNAL_DUTY_STATE_UNKNOWN = 0,
    SIGNAL_DUTY_STATE_LOW,
    SIGNAL_DUTY_STATE_HIGH
} signal_duty_state_t;

static signal_algorithm_status_t SignalDuty_CrossingPosition(
    uint32_t index,
    float previous,
    float current,
    float threshold,
    float *position)
{
    float delta;
    float crossing;

    if (position == NULL)
    {
        return SIGNAL_ALGORITHM_INVALID_ARGUMENT;
    }
    delta = current - previous;
    if ((delta == 0.0F) || !isfinite(delta))
    {
        return SIGNAL_ALGORITHM_NUMERIC_ERROR;
    }
    crossing = (float)(index - 1U) + ((threshold - previous) / delta);
    if (!isfinite(crossing))
    {
        return SIGNAL_ALGORITHM_NUMERIC_ERROR;
    }
    *position = crossing;
    return SIGNAL_ALGORITHM_OK;
}

signal_algorithm_status_t SignalDuty_GetDefaultConfig(signal_duty_config_t *config)
{
    if (config == NULL)
    {
        return SIGNAL_ALGORITHM_INVALID_ARGUMENT;
    }
    config->level_mode = SIGNAL_DUTY_LEVELS_AUTO_MIN_MAX;
    config->threshold_ratio = 0.5F;
    config->hysteresis_ratio = 0.05F;
    config->min_amplitude = 1.0e-6F;
    config->low_level = 0.0F;
    config->high_level = 1.0F;
    return SIGNAL_ALGORITHM_OK;
}

signal_algorithm_status_t SignalDuty_Process(
    const float *samples,
    uint32_t count,
    float sample_rate_hz,
    const signal_duty_config_t *config,
    signal_duty_result_t *result)
{
    uint32_t index;
    float low_level;
    float high_level;
    float amplitude;
    float threshold;
    float lower_guard;
    float upper_guard;
    signal_duty_state_t state;
    bool candidate_valid = false;
    bool last_rise_valid = false;
    bool fall_after_rise_valid = false;
    float candidate = 0.0F;
    float last_rise = 0.0F;
    float fall_after_rise = 0.0F;
    float sum_period_samples = 0.0F;
    float sum_high_samples = 0.0F;
    uint32_t valid_cycles = 0U;
    uint32_t rising_edges = 0U;
    uint32_t falling_edges = 0U;
    signal_duty_result_t local_result;

    if ((samples == NULL) || (config == NULL) || (result == NULL))
    {
        return SIGNAL_ALGORITHM_INVALID_ARGUMENT;
    }
    if (count < 3U)
    {
        return SIGNAL_ALGORITHM_INSUFFICIENT_DATA;
    }
    if (!isfinite(sample_rate_hz) || (sample_rate_hz <= 0.0F) ||
        !isfinite(config->threshold_ratio) ||
        !isfinite(config->hysteresis_ratio) ||
        !isfinite(config->min_amplitude) ||
        !isfinite(config->low_level) ||
        !isfinite(config->high_level))
    {
        return SIGNAL_ALGORITHM_NUMERIC_ERROR;
    }
    if ((config->level_mode != SIGNAL_DUTY_LEVELS_AUTO_MIN_MAX) &&
        (config->level_mode != SIGNAL_DUTY_LEVELS_EXPLICIT))
    {
        return SIGNAL_ALGORITHM_INVALID_ARGUMENT;
    }
    if ((config->threshold_ratio <= 0.0F) ||
        (config->threshold_ratio >= 1.0F) ||
        (config->hysteresis_ratio < 0.0F) ||
        (config->hysteresis_ratio >= config->threshold_ratio) ||
        (config->hysteresis_ratio >= (1.0F - config->threshold_ratio)) ||
        (config->min_amplitude < 0.0F))
    {
        return SIGNAL_ALGORITHM_OUT_OF_RANGE;
    }

    if (!isfinite(samples[0]))
    {
        return SIGNAL_ALGORITHM_NUMERIC_ERROR;
    }
    low_level = (config->level_mode == SIGNAL_DUTY_LEVELS_EXPLICIT) ?
        config->low_level : samples[0];
    high_level = (config->level_mode == SIGNAL_DUTY_LEVELS_EXPLICIT) ?
        config->high_level : samples[0];
    for (index = 1U; index < count; ++index)
    {
        if (!isfinite(samples[index]))
        {
            return SIGNAL_ALGORITHM_NUMERIC_ERROR;
        }
        if (config->level_mode == SIGNAL_DUTY_LEVELS_AUTO_MIN_MAX)
        {
            if (samples[index] < low_level)
            {
                low_level = samples[index];
            }
            if (samples[index] > high_level)
            {
                high_level = samples[index];
            }
        }
    }

    if (high_level <= low_level)
    {
        return (config->level_mode == SIGNAL_DUTY_LEVELS_AUTO_MIN_MAX) ?
            SIGNAL_ALGORITHM_NO_FEATURE : SIGNAL_ALGORITHM_OUT_OF_RANGE;
    }
    amplitude = high_level - low_level;
    if (!isfinite(amplitude))
    {
        return SIGNAL_ALGORITHM_NUMERIC_ERROR;
    }
    if (amplitude <= config->min_amplitude)
    {
        return SIGNAL_ALGORITHM_NO_FEATURE;
    }

    threshold = low_level + config->threshold_ratio * amplitude;
    lower_guard = low_level +
        (config->threshold_ratio - config->hysteresis_ratio) * amplitude;
    upper_guard = low_level +
        (config->threshold_ratio + config->hysteresis_ratio) * amplitude;
    if (!isfinite(threshold) || !isfinite(lower_guard) || !isfinite(upper_guard))
    {
        return SIGNAL_ALGORITHM_NUMERIC_ERROR;
    }

    state = SIGNAL_DUTY_STATE_UNKNOWN;
    if (samples[0] <= lower_guard)
    {
        state = SIGNAL_DUTY_STATE_LOW;
    }
    else if (samples[0] >= upper_guard)
    {
        state = SIGNAL_DUTY_STATE_HIGH;
    }

    for (index = 1U; index < count; ++index)
    {
        float previous = samples[index - 1U];
        float current = samples[index];
        signal_algorithm_status_t status;

        if (state == SIGNAL_DUTY_STATE_UNKNOWN)
        {
            if (current <= lower_guard)
            {
                state = SIGNAL_DUTY_STATE_LOW;
            }
            else if (current >= upper_guard)
            {
                state = SIGNAL_DUTY_STATE_HIGH;
            }
            continue;
        }

        if (state == SIGNAL_DUTY_STATE_LOW)
        {
            if (!candidate_valid && (previous < threshold) &&
                (current >= threshold))
            {
                status = SignalDuty_CrossingPosition(
                    index, previous, current, threshold, &candidate);
                if (status != SIGNAL_ALGORITHM_OK)
                {
                    return status;
                }
                candidate_valid = true;
            }
            if (candidate_valid)
            {
                if (current >= upper_guard)
                {
                    float rise = candidate;
                    ++rising_edges;
                    if (last_rise_valid && fall_after_rise_valid &&
                        (last_rise < fall_after_rise) &&
                        (fall_after_rise < rise))
                    {
                        float period = rise - last_rise;
                        float high_width = fall_after_rise - last_rise;
                        if ((high_width > 0.0F) && (high_width < period))
                        {
                            sum_period_samples += period;
                            sum_high_samples += high_width;
                            ++valid_cycles;
                        }
                    }
                    last_rise = rise;
                    last_rise_valid = true;
                    fall_after_rise_valid = false;
                    candidate_valid = false;
                    state = SIGNAL_DUTY_STATE_HIGH;
                }
                else if (current <= lower_guard)
                {
                    candidate_valid = false;
                }
            }
        }
        else
        {
            if (!candidate_valid && (previous > threshold) &&
                (current <= threshold))
            {
                status = SignalDuty_CrossingPosition(
                    index, previous, current, threshold, &candidate);
                if (status != SIGNAL_ALGORITHM_OK)
                {
                    return status;
                }
                candidate_valid = true;
            }
            if (candidate_valid)
            {
                if (current <= lower_guard)
                {
                    float fall = candidate;
                    ++falling_edges;
                    if (last_rise_valid && (fall > last_rise) &&
                        !fall_after_rise_valid)
                    {
                        fall_after_rise = fall;
                        fall_after_rise_valid = true;
                    }
                    candidate_valid = false;
                    state = SIGNAL_DUTY_STATE_LOW;
                }
                else if (current >= upper_guard)
                {
                    candidate_valid = false;
                }
            }
        }
    }

    if ((valid_cycles == 0U) || (sum_period_samples <= 0.0F))
    {
        return SIGNAL_ALGORITHM_NO_FEATURE;
    }
    {
        float mean_period_samples = sum_period_samples / (float)valid_cycles;
        float mean_high_samples = sum_high_samples / (float)valid_cycles;
        float mean_low_samples = mean_period_samples - mean_high_samples;

        local_result.duty_ratio = sum_high_samples / sum_period_samples;
        local_result.duty_percent = 100.0F * local_result.duty_ratio;
        local_result.period_s = mean_period_samples / sample_rate_hz;
        local_result.frequency_hz = 1.0F / local_result.period_s;
        local_result.high_width_s = mean_high_samples / sample_rate_hz;
        local_result.low_width_s = mean_low_samples / sample_rate_hz;
        local_result.low_level = low_level;
        local_result.high_level = high_level;
        local_result.threshold_level = threshold;
        local_result.valid_cycle_count = valid_cycles;
        local_result.rising_edge_count = rising_edges;
        local_result.falling_edge_count = falling_edges;
    }
    if (!isfinite(local_result.duty_ratio) ||
        !isfinite(local_result.duty_percent) ||
        !isfinite(local_result.period_s) ||
        !isfinite(local_result.frequency_hz) ||
        !isfinite(local_result.high_width_s) ||
        !isfinite(local_result.low_width_s))
    {
        return SIGNAL_ALGORITHM_NUMERIC_ERROR;
    }

    *result = local_result;
    return SIGNAL_ALGORITHM_OK;
}
