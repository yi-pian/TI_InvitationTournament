#include "signal_rms.h"
#include "signal_math_backend.h"

#include <math.h>
#include <stddef.h>

signal_algorithm_status_t SignalRMS_Process(
    const float *voltage_v,
    uint32_t count,
    signal_rms_result_t *result)
{
    uint32_t index;
    float sum_squares = 0.0f;
    float compensation = 0.0f;

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
        float square;
        float corrected;
        float next_sum;

        if (!isfinite(voltage_v[index]))
        {
            return SIGNAL_ALGORITHM_NUMERIC_ERROR;
        }
        square = voltage_v[index] * voltage_v[index];
        if (!isfinite(square))
        {
            return SIGNAL_ALGORITHM_NUMERIC_ERROR;
        }
        corrected = square - compensation;
        next_sum = sum_squares + corrected;
        compensation = (next_sum - sum_squares) - corrected;
        sum_squares = next_sum;
    }

    result->rms_v = SignalMathBackend_SqrtF(sum_squares / (float)count);
    if (!isfinite(result->rms_v))
    {
        return SIGNAL_ALGORITHM_NUMERIC_ERROR;
    }
    return SIGNAL_ALGORITHM_OK;
}
