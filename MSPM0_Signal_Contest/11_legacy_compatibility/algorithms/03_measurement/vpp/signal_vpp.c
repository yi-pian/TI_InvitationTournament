#include "signal_vpp.h"

#include <math.h>
#include <stddef.h>

signal_algorithm_status_t SignalVPP_Process(
    const float *voltage_v,
    uint32_t count,
    signal_vpp_result_t *result)
{
    uint32_t index;
    float min_voltage_v;
    float max_voltage_v;

    if ((voltage_v == NULL) || (result == NULL))
    {
        return SIGNAL_ALGORITHM_INVALID_ARGUMENT;
    }
    if (count == 0U)
    {
        return SIGNAL_ALGORITHM_INSUFFICIENT_DATA;
    }
    if (!isfinite(voltage_v[0]))
    {
        return SIGNAL_ALGORITHM_NUMERIC_ERROR;
    }

    min_voltage_v = voltage_v[0];
    max_voltage_v = voltage_v[0];
    for (index = 1U; index < count; ++index)
    {
        if (!isfinite(voltage_v[index]))
        {
            return SIGNAL_ALGORITHM_NUMERIC_ERROR;
        }
        if (voltage_v[index] < min_voltage_v)
        {
            min_voltage_v = voltage_v[index];
        }
        if (voltage_v[index] > max_voltage_v)
        {
            max_voltage_v = voltage_v[index];
        }
    }

    result->min_voltage_v = min_voltage_v;
    result->max_voltage_v = max_voltage_v;
    result->amplitude_vpp = max_voltage_v - min_voltage_v;
    return SIGNAL_ALGORITHM_OK;
}
