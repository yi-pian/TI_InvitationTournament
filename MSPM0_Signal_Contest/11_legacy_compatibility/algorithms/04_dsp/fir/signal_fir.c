#include "signal_fir.h"

#include <math.h>
#include <stddef.h>

signal_algorithm_status_t SignalFIR_Init(
    signal_fir_t *instance,
    const float *coefficients,
    uint32_t tap_count,
    float *delay_line,
    uint32_t delay_line_count)
{
    uint32_t index;

    if ((instance == NULL) || (coefficients == NULL) || (delay_line == NULL))
    {
        return SIGNAL_ALGORITHM_INVALID_ARGUMENT;
    }
    if (tap_count == 0U)
    {
        return SIGNAL_ALGORITHM_INSUFFICIENT_DATA;
    }
    if (delay_line_count < tap_count)
    {
        return SIGNAL_ALGORITHM_BUFFER_TOO_SMALL;
    }
    for (index = 0U; index < tap_count; ++index)
    {
        if (!isfinite(coefficients[index]))
        {
            return SIGNAL_ALGORITHM_NUMERIC_ERROR;
        }
        delay_line[index] = 0.0f;
    }
    instance->coefficients = coefficients;
    instance->delay_line = delay_line;
    instance->tap_count = tap_count;
    instance->write_index = 0U;
    instance->initialized = 1U;
    return SIGNAL_ALGORITHM_OK;
}

signal_algorithm_status_t SignalFIR_Reset(signal_fir_t *instance)
{
    uint32_t index;

    if ((instance == NULL) || (instance->initialized == 0U) ||
        (instance->delay_line == NULL) || (instance->tap_count == 0U))
    {
        return SIGNAL_ALGORITHM_INVALID_ARGUMENT;
    }
    for (index = 0U; index < instance->tap_count; ++index)
    {
        instance->delay_line[index] = 0.0f;
    }
    instance->write_index = 0U;
    return SIGNAL_ALGORITHM_OK;
}

signal_algorithm_status_t SignalFIR_Process(
    signal_fir_t *instance,
    const float *input_samples,
    float *output_samples,
    uint32_t count)
{
    uint32_t sample_index;

    if ((instance == NULL) || (input_samples == NULL) ||
        (output_samples == NULL) || (instance->initialized == 0U) ||
        (instance->coefficients == NULL) || (instance->delay_line == NULL) ||
        (instance->tap_count == 0U))
    {
        return SIGNAL_ALGORITHM_INVALID_ARGUMENT;
    }
    if (count == 0U)
    {
        return SIGNAL_ALGORITHM_INSUFFICIENT_DATA;
    }
    for (sample_index = 0U; sample_index < count; ++sample_index)
    {
        if (!isfinite(input_samples[sample_index]))
        {
            return SIGNAL_ALGORITHM_NUMERIC_ERROR;
        }
    }

    for (sample_index = 0U; sample_index < count; ++sample_index)
    {
        uint32_t tap_index;
        uint32_t state_index;
        float accumulator = 0.0f;

        instance->delay_line[instance->write_index] = input_samples[sample_index];
        state_index = instance->write_index;
        for (tap_index = 0U; tap_index < instance->tap_count; ++tap_index)
        {
            accumulator += instance->coefficients[tap_index] *
                           instance->delay_line[state_index];
            state_index = (state_index == 0U)
                              ? (instance->tap_count - 1U)
                              : (state_index - 1U);
        }
        if (!isfinite(accumulator))
        {
            return SIGNAL_ALGORITHM_NUMERIC_ERROR;
        }
        output_samples[sample_index] = accumulator;
        ++instance->write_index;
        if (instance->write_index >= instance->tap_count)
        {
            instance->write_index = 0U;
        }
    }
    return SIGNAL_ALGORITHM_OK;
}
