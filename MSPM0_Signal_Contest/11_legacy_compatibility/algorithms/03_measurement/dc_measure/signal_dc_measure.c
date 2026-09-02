#include "signal_dc_measure.h"

#include <math.h>
#include <stddef.h>

#include "signal_mean.h"

signal_algorithm_status_t SignalDCMeasure_FromRawLinear(
    const uint16_t *raw_codes,
    uint32_t count,
    const signal_adc_to_voltage_config_t *conversion,
    signal_dc_measure_result_t *result)
{
    signal_dc_measure_result_t temporary;
    uint32_t index;
    float sum = 0.0f;
    float compensation = 0.0f;
    float volts_per_code;

    if ((raw_codes == NULL) || (conversion == NULL) || (result == NULL))
    {
        return SIGNAL_ALGORITHM_INVALID_ARGUMENT;
    }
    if (count == 0U)
    {
        return SIGNAL_ALGORITHM_INSUFFICIENT_DATA;
    }
    if ((conversion->adc_max_code == 0U) ||
        (conversion->adc_max_code > (uint32_t)UINT16_MAX) ||
        !isfinite(conversion->reference_voltage_v) ||
        !isfinite(conversion->input_scale) ||
        !isfinite(conversion->offset_voltage_v) ||
        (conversion->reference_voltage_v <= 0.0f) ||
        (conversion->input_scale == 0.0f))
    {
        return SIGNAL_ALGORITHM_INVALID_ARGUMENT;
    }
    for (index = 0U; index < count; ++index)
    {
        float value;
        float corrected;
        float updated;
        if ((uint32_t)raw_codes[index] > conversion->adc_max_code)
        {
            return SIGNAL_ALGORITHM_OUT_OF_RANGE;
        }
        value = (float)raw_codes[index];
        corrected = value - compensation;
        updated = sum + corrected;
        compensation = (updated - sum) - corrected;
        sum = updated;
    }
    temporary.mean_code = sum / (float)count;
    volts_per_code = (conversion->reference_voltage_v *
                      conversion->input_scale) /
                     (float)conversion->adc_max_code;
    temporary.dc_voltage_v = (temporary.mean_code * volts_per_code) +
                             conversion->offset_voltage_v;
    *result = temporary;
    return SIGNAL_ALGORITHM_OK;
}

signal_algorithm_status_t SignalDCMeasure_FromVoltage(
    const float *voltage_v,
    uint32_t count,
    signal_dc_measure_result_t *result)
{
    signal_dc_measure_result_t temporary;
    signal_mean_result_t mean_result;
    signal_algorithm_status_t status;

    if (result == NULL)
    {
        return SIGNAL_ALGORITHM_INVALID_ARGUMENT;
    }
    status = SignalMean_Process(voltage_v, count, &mean_result);
    if (status != SIGNAL_ALGORITHM_OK)
    {
        return status;
    }
    temporary.mean_code = NAN;
    temporary.dc_voltage_v = mean_result.mean_value;
    *result = temporary;
    return SIGNAL_ALGORITHM_OK;
}
