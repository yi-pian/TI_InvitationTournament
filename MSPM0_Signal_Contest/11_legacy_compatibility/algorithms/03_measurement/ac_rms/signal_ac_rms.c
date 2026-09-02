#include "signal_ac_rms.h"
#include "signal_math_backend.h"

#include <math.h>
#include <stddef.h>

signal_algorithm_status_t SignalACRMS_Process(
    const float *voltage_v,
    uint32_t count,
    signal_ac_rms_result_t *result)
{
    uint32_t index;
    float sum = 0.0f;
    float sum_compensation = 0.0f;
    float mean_voltage_v;
    float sum_squares = 0.0f;
    float squares_compensation = 0.0f;

    if ((voltage_v == NULL) || (result == NULL))
    {
        return SIGNAL_ALGORITHM_INVALID_ARGUMENT;
    }
    if (count == 0U)
    {
        return SIGNAL_ALGORITHM_INSUFFICIENT_DATA;
    }

    for (index = 0U; index < count; ++index)
    {
        float corrected;
        float next_sum;

        if (!isfinite(voltage_v[index]))
        {
            return SIGNAL_ALGORITHM_NUMERIC_ERROR;
        }
        corrected = voltage_v[index] - sum_compensation;
        next_sum = sum + corrected;
        sum_compensation = (next_sum - sum) - corrected;
        sum = next_sum;
    }
    mean_voltage_v = sum / (float)count;

    for (index = 0U; index < count; ++index)
    {
        float ac_voltage_v = voltage_v[index] - mean_voltage_v;
        float square = ac_voltage_v * ac_voltage_v;
        float corrected = square - squares_compensation;
        float next_sum = sum_squares + corrected;

        if (!isfinite(square))
        {
            return SIGNAL_ALGORITHM_NUMERIC_ERROR;
        }
        squares_compensation = (next_sum - sum_squares) - corrected;
        sum_squares = next_sum;
    }

    result->mean_voltage_v = mean_voltage_v;
    result->ac_rms_v = SignalMathBackend_SqrtF(sum_squares / (float)count);
    if (!isfinite(result->ac_rms_v))
    {
        return SIGNAL_ALGORITHM_NUMERIC_ERROR;
    }
    return SIGNAL_ALGORITHM_OK;
}
