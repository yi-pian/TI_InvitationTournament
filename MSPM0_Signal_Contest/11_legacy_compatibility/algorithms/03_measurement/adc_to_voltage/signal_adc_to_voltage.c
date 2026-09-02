#include "signal_adc_to_voltage.h"

#include <math.h>
#include <stddef.h>

signal_algorithm_status_t SignalADCToVoltage_Process(
    const uint16_t *raw_codes,
    float *voltage_v,
    uint32_t count,
    const signal_adc_to_voltage_config_t *config)
{
    uint32_t index;
    float volts_per_code;

    if ((raw_codes == NULL) || (voltage_v == NULL) || (config == NULL))
    {
        return SIGNAL_ALGORITHM_INVALID_ARGUMENT;
    }
    if (count == 0U)
    {
        return SIGNAL_ALGORITHM_INSUFFICIENT_DATA;
    }
    if ((config->adc_max_code == 0U) ||
        (config->adc_max_code > (uint32_t)UINT16_MAX) ||
        !isfinite(config->reference_voltage_v) ||
        !isfinite(config->input_scale) ||
        !isfinite(config->offset_voltage_v) ||
        (config->reference_voltage_v <= 0.0f) ||
        (config->input_scale == 0.0f))
    {
        return SIGNAL_ALGORITHM_INVALID_ARGUMENT;
    }

    for (index = 0U; index < count; ++index)
    {
        if ((uint32_t)raw_codes[index] > config->adc_max_code)
        {
            return SIGNAL_ALGORITHM_OUT_OF_RANGE;
        }
    }

    volts_per_code = (config->reference_voltage_v * config->input_scale) /
                     (float)config->adc_max_code;
    for (index = 0U; index < count; ++index)
    {
        voltage_v[index] = ((float)raw_codes[index] * volts_per_code) +
                           config->offset_voltage_v;
    }
    return SIGNAL_ALGORITHM_OK;
}
