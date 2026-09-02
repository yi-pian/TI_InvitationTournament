#include "signal_iir_biquad.h"

#include <math.h>
#include <stddef.h>

signal_algorithm_status_t SignalIIRBiquad_Init(
    signal_iir_biquad_t *instance,
    const signal_iir_biquad_coefficients_t *sections,
    uint32_t section_count,
    signal_iir_biquad_state_t *states,
    uint32_t state_count)
{
    uint32_t section_index;

    if ((instance == NULL) || (sections == NULL) || (states == NULL))
    {
        return SIGNAL_ALGORITHM_INVALID_ARGUMENT;
    }
    if (section_count == 0U)
    {
        return SIGNAL_ALGORITHM_INSUFFICIENT_DATA;
    }
    if (state_count < section_count)
    {
        return SIGNAL_ALGORITHM_BUFFER_TOO_SMALL;
    }
    for (section_index = 0U; section_index < section_count; ++section_index)
    {
        const signal_iir_biquad_coefficients_t *c = &sections[section_index];
        if (!isfinite(c->b0) || !isfinite(c->b1) || !isfinite(c->b2) ||
            !isfinite(c->a1) || !isfinite(c->a2))
        {
            return SIGNAL_ALGORITHM_NUMERIC_ERROR;
        }
        states[section_index].d1 = 0.0f;
        states[section_index].d2 = 0.0f;
    }
    instance->sections = sections;
    instance->states = states;
    instance->section_count = section_count;
    instance->initialized = 1U;
    return SIGNAL_ALGORITHM_OK;
}

signal_algorithm_status_t SignalIIRBiquad_Reset(signal_iir_biquad_t *instance)
{
    uint32_t section_index;

    if ((instance == NULL) || (instance->initialized == 0U) ||
        (instance->states == NULL) || (instance->section_count == 0U))
    {
        return SIGNAL_ALGORITHM_INVALID_ARGUMENT;
    }
    for (section_index = 0U; section_index < instance->section_count;
         ++section_index)
    {
        instance->states[section_index].d1 = 0.0f;
        instance->states[section_index].d2 = 0.0f;
    }
    return SIGNAL_ALGORITHM_OK;
}

signal_algorithm_status_t SignalIIRBiquad_Process(
    signal_iir_biquad_t *instance,
    const float *input_samples,
    float *output_samples,
    uint32_t count)
{
    uint32_t sample_index;

    if ((instance == NULL) || (input_samples == NULL) ||
        (output_samples == NULL) || (instance->initialized == 0U) ||
        (instance->sections == NULL) || (instance->states == NULL) ||
        (instance->section_count == 0U))
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
        uint32_t section_index;
        float value = input_samples[sample_index];

        for (section_index = 0U; section_index < instance->section_count;
             ++section_index)
        {
            const signal_iir_biquad_coefficients_t *c =
                &instance->sections[section_index];
            signal_iir_biquad_state_t *state = &instance->states[section_index];
            float output = (c->b0 * value) + state->d1;
            float next_d1 = (c->b1 * value) - (c->a1 * output) + state->d2;
            float next_d2 = (c->b2 * value) - (c->a2 * output);

            if (!isfinite(output) || !isfinite(next_d1) || !isfinite(next_d2))
            {
                return SIGNAL_ALGORITHM_NUMERIC_ERROR;
            }
            state->d1 = next_d1;
            state->d2 = next_d2;
            value = output;
        }
        output_samples[sample_index] = value;
    }
    return SIGNAL_ALGORITHM_OK;
}
