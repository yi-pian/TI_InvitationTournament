#include "signal_adc_gain_offset_calibration.h"

#include <math.h>
#include <stddef.h>

signal_algorithm_status_t SignalADCGainOffsetCalibration_Compute(
    float measured_low_v,
    float true_low_v,
    float measured_high_v,
    float true_high_v,
    signal_adc_gain_offset_calibration_t *calibration)
{
    float measured_span;

    if (calibration == NULL)
    {
        return SIGNAL_ALGORITHM_INVALID_ARGUMENT;
    }
    if (!isfinite(measured_low_v) || !isfinite(true_low_v) ||
        !isfinite(measured_high_v) || !isfinite(true_high_v))
    {
        return SIGNAL_ALGORITHM_INVALID_ARGUMENT;
    }
    measured_span = measured_high_v - measured_low_v;
    if (fabsf(measured_span) <= 1.0e-20f)
    {
        return SIGNAL_ALGORITHM_NO_FEATURE;
    }
    calibration->gain = (true_high_v - true_low_v) / measured_span;
    calibration->offset_v = true_low_v -
                            (calibration->gain * measured_low_v);
    return (isfinite(calibration->gain) && isfinite(calibration->offset_v))
               ? SIGNAL_ALGORITHM_OK
               : SIGNAL_ALGORITHM_NUMERIC_ERROR;
}

signal_algorithm_status_t SignalADCGainOffsetCalibration_Apply(
    const float *input_voltage_v,
    float *output_voltage_v,
    uint32_t count,
    const signal_adc_gain_offset_calibration_t *calibration)
{
    uint32_t index;

    if ((input_voltage_v == NULL) || (output_voltage_v == NULL) ||
        (calibration == NULL))
    {
        return SIGNAL_ALGORITHM_INVALID_ARGUMENT;
    }
    if (count == 0U)
    {
        return SIGNAL_ALGORITHM_INSUFFICIENT_DATA;
    }
    if (!isfinite(calibration->gain) || !isfinite(calibration->offset_v))
    {
        return SIGNAL_ALGORITHM_INVALID_ARGUMENT;
    }
    for (index = 0U; index < count; ++index)
    {
        if (!isfinite(input_voltage_v[index]))
        {
            return SIGNAL_ALGORITHM_NUMERIC_ERROR;
        }
    }
    for (index = 0U; index < count; ++index)
    {
        output_voltage_v[index] =
            calibration->gain * input_voltage_v[index] +
            calibration->offset_v;
    }
    return SIGNAL_ALGORITHM_OK;
}
