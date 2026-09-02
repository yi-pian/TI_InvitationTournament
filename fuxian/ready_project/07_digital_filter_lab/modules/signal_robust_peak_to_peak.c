#include "signal_robust_peak_to_peak.h"

#include <limits.h>
#include <math.h>
#include <stddef.h>

static void SignalRobustP2P_Swap(float *left, float *right)
{
    float temporary = *left;
    *left = *right;
    *right = temporary;
}

static float SignalRobustP2P_Select(float *values, uint32_t count, uint32_t k)
{
    int32_t left = 0;
    int32_t right = (int32_t)count - 1;
    int32_t target = (int32_t)k;

    while (left < right)
    {
        float pivot = values[left + ((right - left) / 2)];
        int32_t lower = left;
        int32_t scan = left;
        int32_t upper = right;

        while (scan <= upper)
        {
            if (values[scan] < pivot)
            {
                SignalRobustP2P_Swap(&values[scan], &values[lower]);
                ++scan;
                ++lower;
            }
            else if (values[scan] > pivot)
            {
                SignalRobustP2P_Swap(&values[scan], &values[upper]);
                --upper;
            }
            else
            {
                ++scan;
            }
        }
        if (target < lower)
        {
            right = lower - 1;
        }
        else if (target > upper)
        {
            left = upper + 1;
        }
        else
        {
            return values[target];
        }
    }
    return values[left];
}

static float SignalRobustP2P_Quantile(
    float *values,
    uint32_t count,
    float quantile)
{
    float rank = quantile * (float)(count - 1U);
    uint32_t lower_index = (uint32_t)floorf(rank);
    uint32_t upper_index = (uint32_t)ceilf(rank);
    float lower = SignalRobustP2P_Select(values, count, lower_index);

    if (lower_index == upper_index)
    {
        return lower;
    }
    {
        float upper = SignalRobustP2P_Select(values, count, upper_index);
        float fraction = rank - (float)lower_index;
        return lower + fraction * (upper - lower);
    }
}

signal_algorithm_status_t SignalRobustPeakToPeak_Process(
    const float *voltage_v,
    uint32_t count,
    const signal_robust_peak_to_peak_config_t *config,
    float *workspace,
    uint32_t workspace_count,
    signal_robust_peak_to_peak_result_t *result)
{
    uint32_t index;

    if ((voltage_v == NULL) || (config == NULL) ||
        (workspace == NULL) || (result == NULL))
    {
        return SIGNAL_ALGORITHM_INVALID_ARGUMENT;
    }
    if ((count == 0U) || (count > (uint32_t)INT32_MAX))
    {
        return SIGNAL_ALGORITHM_INSUFFICIENT_DATA;
    }
    if (workspace_count < count)
    {
        return SIGNAL_ALGORITHM_BUFFER_TOO_SMALL;
    }
    if (!isfinite(config->lower_quantile) ||
        !isfinite(config->upper_quantile) ||
        (config->lower_quantile < 0.0f) ||
        (config->upper_quantile > 1.0f) ||
        (config->lower_quantile >= config->upper_quantile))
    {
        return SIGNAL_ALGORITHM_INVALID_ARGUMENT;
    }
    for (index = 0U; index < count; ++index)
    {
        if (!isfinite(voltage_v[index]))
        {
            return SIGNAL_ALGORITHM_NUMERIC_ERROR;
        }
        workspace[index] = voltage_v[index];
    }
    result->lower_voltage_v = SignalRobustP2P_Quantile(
        workspace, count, config->lower_quantile);
    result->upper_voltage_v = SignalRobustP2P_Quantile(
        workspace, count, config->upper_quantile);
    result->robust_vpp_v = result->upper_voltage_v - result->lower_voltage_v;
    return isfinite(result->robust_vpp_v)
               ? SIGNAL_ALGORITHM_OK
               : SIGNAL_ALGORITHM_NUMERIC_ERROR;
}
