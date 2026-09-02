#include "signal_mad.h"

#include <limits.h>
#include <math.h>
#include <stddef.h>

#define SIGNAL_MAD_NORMAL_SCALE_F 1.4826f

static void SignalMAD_Swap(float *left, float *right)
{
    float temporary = *left;
    *left = *right;
    *right = temporary;
}

static float SignalMAD_SelectKth(float *values, uint32_t count, uint32_t k)
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
                SignalMAD_Swap(&values[scan], &values[lower]);
                ++scan;
                ++lower;
            }
            else if (values[scan] > pivot)
            {
                SignalMAD_Swap(&values[scan], &values[upper]);
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

static float SignalMAD_Median(float *values, uint32_t count)
{
    uint32_t upper_index = count / 2U;
    float upper = SignalMAD_SelectKth(values, count, upper_index);

    if ((count & 1U) != 0U)
    {
        return upper;
    }
    return 0.5f *
        (upper + SignalMAD_SelectKth(values, count, upper_index - 1U));
}

signal_algorithm_status_t SignalMAD_Process(
    const float *samples,
    uint32_t count,
    float *workspace,
    uint32_t workspace_count,
    signal_mad_result_t *result)
{
    uint32_t index;
    float median_value;
    float mad_value;

    if ((samples == NULL) || (workspace == NULL) || (result == NULL))
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

    for (index = 0U; index < count; ++index)
    {
        if (!isfinite(samples[index]))
        {
            return SIGNAL_ALGORITHM_NUMERIC_ERROR;
        }
        workspace[index] = samples[index];
    }
    median_value = SignalMAD_Median(workspace, count);
    for (index = 0U; index < count; ++index)
    {
        workspace[index] = fabsf(samples[index] - median_value);
    }
    mad_value = SignalMAD_Median(workspace, count);

    result->median_value = median_value;
    result->mad_value = mad_value;
    result->robust_sigma_estimate = SIGNAL_MAD_NORMAL_SCALE_F * mad_value;
    return isfinite(result->robust_sigma_estimate)
               ? SIGNAL_ALGORITHM_OK
               : SIGNAL_ALGORITHM_NUMERIC_ERROR;
}
