#include "signal_opa_dac_bias.h"

#include <stddef.h>

signal_result_t SignalOPADACBias_Calculate(float input_voltage_v, float gain,
    float dac_bias_v, float *output_voltage_v)
{
    if ((output_voltage_v == NULL) || (dac_bias_v < 0.0f)) {
        return SIGNAL_RESULT_INVALID_ARGUMENT;
    }
    *output_voltage_v = dac_bias_v + gain * input_voltage_v;
    return SIGNAL_RESULT_OK;
}

signal_module_status_t SignalOPADACBias_GetModuleStatus(void)
{ return MODULE_STATUS_BUILD_VERIFIED; }
