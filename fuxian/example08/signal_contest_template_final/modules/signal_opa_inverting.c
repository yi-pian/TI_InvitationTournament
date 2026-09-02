#include "signal_opa_inverting.h"

#include <math.h>
#include <stddef.h>

signal_result_t SignalOPAInverting_MakeConfig(float requested_gain,
    float input_resistor_ohm, float bias_voltage_v,
    signal_opa_config_t *config, float *feedback_resistor_ohm)
{
    if ((config == NULL) || (feedback_resistor_ohm == NULL) ||
        !(input_resistor_ohm > 0.0f) || !(requested_gain < 0.0f) ||
        (bias_voltage_v < 0.0f)) {
        return SIGNAL_RESULT_INVALID_ARGUMENT;
    }
    *feedback_resistor_ohm = fabsf(requested_gain) * input_resistor_ohm;
    config->mode = SIGNAL_OPA_MODE_INVERTING;
    config->resistor_feedback_ohm = *feedback_resistor_ohm;
    config->resistor_input_ohm = input_resistor_ohm;
    config->bias_voltage_v = bias_voltage_v;
    return SIGNAL_RESULT_OK;
}

signal_module_status_t SignalOPAInverting_GetModuleStatus(void)
{ return MODULE_STATUS_BUILD_VERIFIED; }
