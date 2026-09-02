#include "signal_robust_rms.h"

#include <math.h>
#include <stddef.h>

#include "signal_robust_peak_to_peak.h"

static float SignalRobustRMS_Clamp(float value, float low, float high)
{
    if (value < low)
    {
        return low;
    }
    if (value > high)
    {
        return high;
    }
    return value;
}

signal_algorithm_status_t SignalRobustRMS_Process(
    const float *voltage_v,
    uint32_t count,
    const signal_robust_rms_config_t *config,
    float *workspace,
    uint32_t workspace_count,
    signal_robust_rms_result_t *result)
{
    signal_robust_peak_to_peak_config_t p2p_config;
    signal_robust_peak_to_peak_result_t p2p_result;
    signal_algorithm_status_t status;
    uint32_t index;
    uint32_t winsorized_count = 0U;
    float sum = 0.0f;
    float mean;
    float sum_squares = 0.0f;

    if ((voltage_v == NULL) || (config == NULL) ||
        (workspace == NULL) || (result == NULL))
    {
        return SIGNAL_ALGORITHM_INVALID_ARGUMENT;
    }
    if (count == 0U)
    {
        return SIGNAL_ALGORITHM_INSUFFICIENT_DATA;
    }
    if (config->remove_dc > 1U)
    {
        return SIGNAL_ALGORITHM_INVALID_ARGUMENT;
    }
    p2p_config.lower_quantile = config->lower_quantile;
    p2p_config.upper_quantile = config->upper_quantile;
    status = SignalRobustPeakToPeak_Process(
        voltage_v, count, &p2p_config, workspace, workspace_count, &p2p_result);
    if (status != SIGNAL_ALGORITHM_OK)
    {
        return status;
    }
    for (index = 0U; index < count; ++index)
    {
        float clamped = SignalRobustRMS_Clamp(
            voltage_v[index], p2p_result.lower_voltage_v,
            p2p_result.upper_voltage_v);
        if (clamped != voltage_v[index])
        {
            ++winsorized_count;
        }
        sum += clamped;
    }
    mean = sum / (float)count;
    for (index = 0U; index < count; ++index)
    {
        float clamped = SignalRobustRMS_Clamp(
            voltage_v[index], p2p_result.lower_voltage_v,
            p2p_result.upper_voltage_v);
        float value = (config->remove_dc != 0U) ? (clamped - mean) : clamped;
        sum_squares += value * value;
    }
    result->lower_limit_v = p2p_result.lower_voltage_v;
    result->upper_limit_v = p2p_result.upper_voltage_v;
    result->winsorized_mean_v = mean;
    result->robust_rms_v = sqrtf(sum_squares / (float)count);
    result->winsorized_count = winsorized_count;
    return isfinite(result->robust_rms_v)
               ? SIGNAL_ALGORITHM_OK
               : SIGNAL_ALGORITHM_NUMERIC_ERROR;
}
