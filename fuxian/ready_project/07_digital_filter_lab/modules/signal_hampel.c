#include "signal_hampel.h"

#include <math.h>
#include <stddef.h>

#include "signal_mad.h"

signal_algorithm_status_t SignalHampel_Process(
    const float *input_samples,
    float *output_samples,
    uint32_t count,
    const signal_hampel_config_t *config,
    float *workspace,
    uint32_t workspace_count,
    signal_hampel_result_t *result)
{
    uint32_t index;
    uint32_t half_window;
    uint32_t replaced_count = 0U;

    if ((input_samples == NULL) || (output_samples == NULL) ||
        (config == NULL) || (workspace == NULL) || (result == NULL) ||
        (input_samples == output_samples))
    {
        return SIGNAL_ALGORITHM_INVALID_ARGUMENT;
    }
    if ((count == 0U) || (config->window_size == 0U) ||
        (config->window_size > count) ||
        ((config->window_size & 1U) == 0U))
    {
        return SIGNAL_ALGORITHM_INSUFFICIENT_DATA;
    }
    if (!isfinite(config->threshold_sigma) ||
        !isfinite(config->minimum_scale) ||
        (config->threshold_sigma <= 0.0f) ||
        (config->minimum_scale < 0.0f))
    {
        return SIGNAL_ALGORITHM_INVALID_ARGUMENT;
    }
    if (workspace_count < config->window_size)
    {
        return SIGNAL_ALGORITHM_BUFFER_TOO_SMALL;
    }
    for (index = 0U; index < count; ++index)
    {
        if (!isfinite(input_samples[index]))
        {
            return SIGNAL_ALGORITHM_NUMERIC_ERROR;
        }
    }

    half_window = config->window_size / 2U;
    for (index = 0U; index < count; ++index)
    {
        uint32_t start = (index > half_window) ? (index - half_window) : 0U;
        uint32_t end = index + half_window;
        uint32_t local_count;
        signal_mad_result_t mad_result;
        signal_algorithm_status_t status;
        float scale;
        float threshold;

        if (end >= count)
        {
            end = count - 1U;
        }
        local_count = end - start + 1U;
        status = SignalMAD_Process(&input_samples[start], local_count,
                                   workspace, workspace_count, &mad_result);
        if (status != SIGNAL_ALGORITHM_OK)
        {
            return status;
        }
        scale = (mad_result.robust_sigma_estimate > config->minimum_scale)
                    ? mad_result.robust_sigma_estimate
                    : config->minimum_scale;
        threshold = config->threshold_sigma * scale;
        if (fabsf(input_samples[index] - mad_result.median_value) > threshold)
        {
            output_samples[index] = mad_result.median_value;
            ++replaced_count;
        }
        else
        {
            output_samples[index] = input_samples[index];
        }
    }
    result->replaced_count = replaced_count;
    return SIGNAL_ALGORITHM_OK;
}
